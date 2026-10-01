/* Recorded-data probe. Does not read live sensors or control device services. */
#define VIT_INTERFACE_IMPLEMENTATION
#include <vit_interface.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <utility>
#include <stdexcept>
#include <string>
#include <map>
#include <set>
#include "vio-quality.h"

static void check(vit_result_t r) { if(r!=VIT_SUCCESS)throw std::runtime_error("VIT operation failed"); }
static uint64_t le(const unsigned char *p,int n){uint64_t x=0;for(int i=n-1;i>=0;i--)x=(x<<8)|p[i];return x;}
int main(int argc,char **argv){
 if(argc<4||argc>7){fprintf(stderr,"Usage: replay-vit CONFIG EVENTS OUTPUT.csv [FEATURES.csv [CAM_COUNT [MASKS.txt]]]\n");return 2;}
 try{
  unsigned cam_count=argc>=6?(unsigned)std::atoi(argv[5]):4;
  if(cam_count<2||cam_count>4)throw std::runtime_error("camera count must be 2..4");
  const char *bracket_env=std::getenv("QUEST_REPLAY_BRACKET_IMU");
  if(bracket_env&&std::strcmp(bracket_env,"1"))throw std::runtime_error("QUEST_REPLAY_BRACKET_IMU must be 1 or unset");
  const bool bracket_imu=bracket_env!=nullptr;
  const char *quality_path=std::getenv("QUEST_REPLAY_QUALITY");
  struct mq_state quality{};FILE *quality_out=nullptr;
  std::vector<std::vector<vit_mask_t>> masks(cam_count);
  if(argc==7){
   std::ifstream mask_input(argv[6]);if(!mask_input)throw std::runtime_error("mask file open failed");
   int cam,x,y,w,h;
   while(mask_input>>cam){
    if(!(mask_input>>x>>y>>w>>h)||cam<0||(unsigned)cam>=cam_count||x<0||y<0||w<=0||h<=0||x>=320||y>=240||w>320-x||h>240-y)
     throw std::runtime_error("invalid mask rectangle");
    if(masks[cam].size()>=76800)throw std::runtime_error("too many masks");
    masks[cam].push_back({(float)x,(float)y,(float)w,(float)h});
   }
   if(!mask_input.eof())throw std::runtime_error("invalid mask data");
  }
  unsigned image_bytes=cam_count*76800;
  // Validate the entire file before starting worker threads, and locate the
  // final image. Unbounded trailing IMU blocks the estimator's bounded queue.
  std::ifstream input(argv[2],std::ios::binary);if(!input)throw std::runtime_error("events open failed");
  unsigned char header[13];uint64_t last_camera=0,previous=0;unsigned expected_groups=0;
  std::vector<uint64_t> camera_times;
  bool awaiting_bracket=false;
  while(true){
   input.read((char*)header,sizeof(header));
   if(input.gcount()==0&&input.eof())break;
   if(input.gcount()!=sizeof(header))throw std::runtime_error("truncated event header");
   uint64_t timestamp=le(header+1,8);unsigned size=(unsigned)le(header+9,4);
   if(timestamp<previous)throw std::runtime_error("nonmonotonic event timestamp");
   previous=timestamp;
   if(!((header[0]=='I'&&size==24)||(header[0]=='C'&&size==image_bytes)))throw std::runtime_error("invalid event type/size");
   std::vector<char> data(size);if(!input.read(data.data(),size))throw std::runtime_error("truncated event payload");
   if(header[0]=='C'){
    if(bracket_imu&&awaiting_bracket)throw std::runtime_error("camera lacks future IMU bracket before next camera");
    last_camera=timestamp;expected_groups++;camera_times.push_back(timestamp);awaiting_bracket=true;
   }else if(timestamp>last_camera)awaiting_bracket=false;
  }
  if(!expected_groups)throw std::runtime_error("no camera events");
  if(bracket_imu&&awaiting_bracket)throw std::runtime_error("final camera lacks future IMU bracket");
  input.clear();input.seekg(0);
  vit_config_t config={argv[1],cam_count,1,false};vit_tracker_t *tracker=nullptr;
  check(vit_tracker_create(&config,&tracker));
  if(argc>=5||quality_path)check(vit_tracker_enable_extension(tracker,VIT_TRACKER_EXTENSION_POSE_FEATURES,true));
  check(vit_tracker_start(tracker));
  std::string partial=std::string(argv[3])+".partial";
  FILE *out=fopen(partial.c_str(),"w");if(!out)throw std::runtime_error("pose output open failed");
  setvbuf(out,nullptr,_IOLBF,0);
  FILE *features=nullptr;std::string feature_partial;
  if(argc>=5){
   feature_partial=std::string(argv[4])+".partial";features=fopen(feature_partial.c_str(),"w");
   if(!features)throw std::runtime_error("feature output open failed");
   setvbuf(features,nullptr,_IOLBF,0);
   fprintf(features,"timestamp_ns,cam0,cam1,cam2,cam3,unique_landmarks,multicamera_landmarks,positive_inverse_depths\n");
  }
  // Optional replay submission-to-poll age, not sensor/display latency.
  const char *timing_path=std::getenv("QUEST_REPLAY_TIMING");
  FILE *timing=nullptr;std::string timing_partial;
  if(timing_path){
   if(!*timing_path)throw std::runtime_error("empty timing path");
   timing_partial=std::string(timing_path)+".partial";
   timing=fopen(timing_partial.c_str(),"w");if(!timing)throw std::runtime_error("timing output open failed");
   setvbuf(timing,nullptr,_IOLBF,0);
   fprintf(timing,"timestamp_ns,submission_to_poll_ns,outstanding_camera_groups\n");
  }
  if(quality_path){
   quality_out=fopen(quality_path,"wx");if(!quality_out)throw std::runtime_error("quality output exists or cannot open");
   setvbuf(quality_out,nullptr,_IOLBF,0);
   fprintf(quality_out,"timestamp_ns,cam0_positive,cam1_positive,coverage0,coverage1,shared,ready,slot2_positive,slot3_positive,coverage2,coverage3\n");
  }
  using Clock=std::chrono::steady_clock;
  std::vector<Clock::time_point> submitted;
  fprintf(out,"timestamp_ns,x,y,z,qx,qy,qz,qw,vx,vy,vz\n");unsigned poses=0,groups=0;
  auto drain=[&](){while(true){vit_pose_t *pose=nullptr;check(vit_tracker_pop_pose(tracker,&pose));if(!pose)break;
   vit_pose_data_t p;check(vit_pose_get_data(pose,&p));
   if(poses>=camera_times.size()||(uint64_t)p.timestamp!=camera_times[poses]){
    vit_pose_destroy(pose);throw std::runtime_error("missing or mismatched camera pose");
   }
   if(timing){
    if(poses>=submitted.size())throw std::runtime_error("pose before camera submission");
    auto age=std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-submitted[poses]).count();
    fprintf(timing,"%lld,%lld,%zu\n",(long long)p.timestamp,(long long)age,submitted.size()-poses-1);
   }
   fprintf(out,"%lld,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",(long long)p.timestamp,p.px,p.py,p.pz,p.ox,p.oy,p.oz,p.ow,p.vx,p.vy,p.vz);
   if(features){
    std::map<int64_t,unsigned> cameras;unsigned counts[4]={0},positive=0,shared=0;
    for(unsigned cam=0;cam<cam_count;cam++){
     vit_pose_features_t f={};check(vit_pose_get_features(pose,cam,&f));counts[cam]=f.count;
     for(unsigned j=0;j<f.count;j++){cameras[f.features[j].id]|=1U<<cam;positive+=f.features[j].depth>0;}
    }
    for(const auto &item:cameras)shared+=(item.second&(item.second-1))!=0;
    fprintf(features,"%lld,%u,%u,%u,%u,%zu,%u,%u\n",(long long)p.timestamp,counts[0],counts[1],counts[2],counts[3],cameras.size(),shared,positive);
   }
   if(quality_out){
   unsigned positive[4]={0},coverage[4]={0},shared=0;
   std::map<int64_t,unsigned> observed;
   for(unsigned cam=0;cam<cam_count;cam++){
    vit_pose_features_t features{};check(vit_pose_get_features(pose,cam,&features));std::set<unsigned> cells;
    for(unsigned j=0;j<features.count;j++){
     const auto &f=features.features[j];
     if(!std::isfinite(f.depth)||f.depth<=0||!std::isfinite(f.u)||!std::isfinite(f.v)||f.u<0||f.u>=320||f.v<0||f.v>=240)continue;
     positive[cam]++;observed[f.id]|=1U<<cam;cells.insert(unsigned(f.u/80)+4*unsigned(f.v/60));
    }
    coverage[cam]=cells.size();
   }
   for(const auto &entry:observed)shared+=(entry.second&(entry.second-1))!=0;
   struct mv_packet estimate{};estimate.timestamp=p.timestamp;
   estimate.position={p.px,p.py,p.pz};estimate.orientation={p.ox,p.oy,p.oz,p.ow};
   bool initialized=mq_update_multi(&quality,estimate,positive,coverage,cam_count,observed.size(),shared,uint64_t(p.timestamp));
    // Replay gate evaluates geometry/support only, assuming fresh input. It
    // cannot validate live processing age from recorded timestamps.
    fprintf(quality_out,"%lld,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",(long long)p.timestamp,positive[0],positive[1],coverage[0],coverage[1],shared,unsigned(initialized),positive[2],positive[3],coverage[2],coverage[3]);
   }
   vit_pose_destroy(pose);poses++;}};
  const char *realtime_env=std::getenv("QUEST_REPLAY_REALTIME");
  if(realtime_env&&std::strcmp(realtime_env,"1"))throw std::runtime_error("QUEST_REPLAY_REALTIME must be 1 or unset");
  const bool realtime=realtime_env!=nullptr;
  const auto wall_origin=Clock::now();uint64_t event_origin=0;bool first_event=true;
  bool final_imu=false;unsigned omitted_imu=0;
  // Deterministic Basalt blocks the final camera push until its pose is ready.
  // Supply one strictly later IMU sample first, without changing either stream's
  // timestamps/order or feeding an unbounded IMU lookahead.
  auto submit_camera=[&](uint64_t timestamp,std::vector<unsigned char> &data){
    submitted.push_back(Clock::now());
    for(unsigned i=0;i<cam_count;i++){
     vit_img_sample_t sample={};sample.cam_index=i;sample.timestamp=timestamp;
     sample.data=data.data()+i*76800;sample.width=320;sample.height=240;
     sample.stride=320;sample.size=76800;sample.format=VIT_IMAGE_FORMAT_L8;
     sample.mask_count=(uint32_t)masks[i].size();sample.masks=masks[i].data();
     check(vit_tracker_push_img_sample(tracker,&sample));
    }
    groups++;drain();
    // Bound replay throughput so output queues stay drained and timing remains
    // comparable to the 30 Hz producer. This is not a live-device benchmark.
    if(!realtime)std::this_thread::sleep_for(std::chrono::milliseconds(33));
  };
  std::vector<unsigned char> pending_camera;uint64_t pending_timestamp=0;
  while(input.read((char*)header,sizeof(header))){
   uint64_t timestamp=le(header+1,8);unsigned size=(unsigned)le(header+9,4);
   if(size>307200)throw std::runtime_error("oversize event");
   std::vector<unsigned char> data(size);if(!input.read((char*)data.data(),size))throw std::runtime_error("truncated event");
   if(realtime){
    if(first_event){event_origin=timestamp;first_event=false;}
    const uint64_t elapsed=timestamp-event_origin;
    if(elapsed>86400000000000ULL)throw std::runtime_error("realtime replay exceeds one day");
    std::this_thread::sleep_until(wall_origin+std::chrono::nanoseconds(elapsed));
   }
   if(header[0]=='I'){
    if(size!=24)throw std::runtime_error("invalid IMU event");
    // One sample beyond the last image brackets its integration interval.
    if(final_imu){omitted_imu++;continue;}
    if(timestamp>last_camera)final_imu=true;
    float v[6];memcpy(v,data.data(),24);
    vit_imu_sample_t sample={(int64_t)timestamp,v[0],v[1],v[2],v[3],v[4],v[5]};
    check(vit_tracker_push_imu_sample(tracker,&sample));
    drain();
    if(bracket_imu&&!pending_camera.empty()&&timestamp>pending_timestamp){
     submit_camera(pending_timestamp,pending_camera);pending_camera.clear();
    }
   }else if(header[0]=='C'){
    if(size!=image_bytes)throw std::runtime_error("invalid camera event");
    if(bracket_imu){pending_camera=std::move(data);pending_timestamp=timestamp;}
    else submit_camera(timestamp,data);
   }else throw std::runtime_error("unknown event");
  }
  if(!input.eof())throw std::runtime_error("event read failed");
  check(vit_tracker_stop(tracker));drain();vit_tracker_destroy(tracker);
  if(fclose(out))throw std::runtime_error("pose output close failed");
  if(features&&fclose(features))throw std::runtime_error("feature output close failed");
  if(quality_out&&fclose(quality_out))throw std::runtime_error("quality output close failed");
  if(timing&&fclose(timing))throw std::runtime_error("timing output close failed");
  if(groups!=expected_groups||poses!=expected_groups)throw std::runtime_error("incomplete replay");
  if(std::rename(partial.c_str(),argv[3]))throw std::runtime_error("pose output rename failed");
  if(features&&std::rename(feature_partial.c_str(),argv[4]))throw std::runtime_error("feature output rename failed");
  if(timing&&std::rename(timing_partial.c_str(),timing_path))throw std::runtime_error("timing output rename failed");
  fprintf(stderr,"Replay complete: %u/%u groups, %u poses, %u trailing IMU samples omitted\n",groups,expected_groups,poses,omitted_imu);
  return poses?0:3;
 }catch(const std::exception &e){fprintf(stderr,"%s\n",e.what());return 2;}
}
