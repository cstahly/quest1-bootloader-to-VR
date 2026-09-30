/* Host-only recorded-data probe. Does not connect to or control the headset. */
#define VIT_INTERFACE_IMPLEMENTATION
#include <vit_interface.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <stdexcept>
#include <string>
#include <map>

static void check(vit_result_t r) { if(r!=VIT_SUCCESS)throw std::runtime_error("VIT operation failed"); }
static uint64_t le(const unsigned char *p,int n){uint64_t x=0;for(int i=n-1;i>=0;i--)x=(x<<8)|p[i];return x;}
int main(int argc,char **argv){
 if(argc!=4&&argc!=5){fprintf(stderr,"Usage: replay-vit CONFIG EVENTS OUTPUT.csv [FEATURES.csv]\n");return 2;}
 try{
  // Validate the entire file before starting worker threads, and locate the
  // final image. Unbounded trailing IMU blocks the estimator's bounded queue.
  std::ifstream input(argv[2],std::ios::binary);if(!input)throw std::runtime_error("events open failed");
  unsigned char header[13];uint64_t last_camera=0,previous=0;unsigned expected_groups=0;
  std::vector<uint64_t> camera_times;
  while(true){
   input.read((char*)header,sizeof(header));
   if(input.gcount()==0&&input.eof())break;
   if(input.gcount()!=sizeof(header))throw std::runtime_error("truncated event header");
   uint64_t timestamp=le(header+1,8);unsigned size=(unsigned)le(header+9,4);
   if(timestamp<previous)throw std::runtime_error("nonmonotonic event timestamp");
   previous=timestamp;
   if(!((header[0]=='I'&&size==24)||(header[0]=='C'&&size==307200)))throw std::runtime_error("invalid event type/size");
   std::vector<char> data(size);if(!input.read(data.data(),size))throw std::runtime_error("truncated event payload");
   if(header[0]=='C'){last_camera=timestamp;expected_groups++;camera_times.push_back(timestamp);}
  }
  if(!expected_groups)throw std::runtime_error("no camera events");
  input.clear();input.seekg(0);
  vit_config_t config={argv[1],4,1,false};vit_tracker_t *tracker=nullptr;
  check(vit_tracker_create(&config,&tracker));
  if(argc==5)check(vit_tracker_enable_extension(tracker,VIT_TRACKER_EXTENSION_POSE_FEATURES,true));
  check(vit_tracker_start(tracker));
  std::string partial=std::string(argv[3])+".partial";
  FILE *out=fopen(partial.c_str(),"w");if(!out)throw std::runtime_error("pose output open failed");
  setvbuf(out,nullptr,_IOLBF,0);
  FILE *features=nullptr;std::string feature_partial;
  if(argc==5){
   feature_partial=std::string(argv[4])+".partial";features=fopen(feature_partial.c_str(),"w");
   if(!features)throw std::runtime_error("feature output open failed");
   setvbuf(features,nullptr,_IOLBF,0);
   fprintf(features,"timestamp_ns,cam0,cam1,cam2,cam3,unique_landmarks,multicamera_landmarks,positive_inverse_depths\n");
  }
  fprintf(out,"timestamp_ns,x,y,z,qx,qy,qz,qw\n");unsigned poses=0,groups=0;
  auto drain=[&](){while(true){vit_pose_t *pose=nullptr;check(vit_tracker_pop_pose(tracker,&pose));if(!pose)break;
   vit_pose_data_t p;check(vit_pose_get_data(pose,&p));
   if(poses>=camera_times.size()||(uint64_t)p.timestamp!=camera_times[poses]){
    vit_pose_destroy(pose);throw std::runtime_error("missing or mismatched camera pose");
   }
   fprintf(out,"%lld,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",(long long)p.timestamp,p.px,p.py,p.pz,p.ox,p.oy,p.oz,p.ow);
   if(features){
    std::map<int64_t,unsigned> cameras;unsigned counts[4],positive=0,shared=0;
    for(unsigned cam=0;cam<4;cam++){
     vit_pose_features_t f={};check(vit_pose_get_features(pose,cam,&f));counts[cam]=f.count;
     for(unsigned j=0;j<f.count;j++){cameras[f.features[j].id]|=1U<<cam;positive+=f.features[j].depth>0;}
    }
    for(const auto &item:cameras)shared+=(item.second&(item.second-1))!=0;
    fprintf(features,"%lld,%u,%u,%u,%u,%zu,%u,%u\n",(long long)p.timestamp,counts[0],counts[1],counts[2],counts[3],cameras.size(),shared,positive);
   }
   vit_pose_destroy(pose);poses++;}};
  bool final_imu=false;unsigned omitted_imu=0;
  while(input.read((char*)header,sizeof(header))){
   uint64_t timestamp=le(header+1,8);unsigned size=(unsigned)le(header+9,4);
   if(size>307200)throw std::runtime_error("oversize event");
   std::vector<unsigned char> data(size);if(!input.read((char*)data.data(),size))throw std::runtime_error("truncated event");
   if(header[0]=='I'){
    if(size!=24)throw std::runtime_error("invalid IMU event");
    // One sample beyond the last image brackets its integration interval.
    if(final_imu){omitted_imu++;continue;}
    if(timestamp>last_camera)final_imu=true;
    float v[6];memcpy(v,data.data(),24);
    vit_imu_sample_t sample={(int64_t)timestamp,v[0],v[1],v[2],v[3],v[4],v[5]};
    check(vit_tracker_push_imu_sample(tracker,&sample));
    drain();
   }else if(header[0]=='C'){
    if(size!=307200)throw std::runtime_error("invalid camera event");
    for(unsigned i=0;i<4;i++){
     vit_img_sample_t sample={};sample.cam_index=i;sample.timestamp=timestamp;
     sample.data=data.data()+i*76800;sample.width=320;sample.height=240;
     sample.stride=320;sample.size=76800;sample.format=VIT_IMAGE_FORMAT_L8;
     check(vit_tracker_push_img_sample(tracker,&sample));
    }
    groups++;drain();
    // Bound replay throughput so output queues stay drained and timing remains
    // comparable to the 30 Hz producer. This is not a live-device benchmark.
    std::this_thread::sleep_for(std::chrono::milliseconds(33));
   }else throw std::runtime_error("unknown event");
  }
  if(!input.eof())throw std::runtime_error("event read failed");
  check(vit_tracker_stop(tracker));drain();vit_tracker_destroy(tracker);
  if(fclose(out))throw std::runtime_error("pose output close failed");
  if(features&&fclose(features))throw std::runtime_error("feature output close failed");
  if(groups!=expected_groups||poses!=expected_groups)throw std::runtime_error("incomplete replay");
  if(std::rename(partial.c_str(),argv[3]))throw std::runtime_error("pose output rename failed");
  if(features&&std::rename(feature_partial.c_str(),argv[4]))throw std::runtime_error("feature output rename failed");
  fprintf(stderr,"Replay complete: %u/%u groups, %u poses, %u trailing IMU samples omitted\n",groups,expected_groups,poses,omitted_imu);
  return poses?0:3;
 }catch(const std::exception &e){fprintf(stderr,"%s\n",e.what());return 2;}
}
