/* Bounded diagnostic only: passive IMU + existing camera feed to isolated VIT.
 * Does not configure sensors or control device services. Optional mailbox output
 * requires explicit head-origin calibration and quality gating to become ready.
 * The MCU/host clock offset is transport-estimated, not exposure calibrated.
 */
#define VIT_INTERFACE_IMPLEMENTATION
#include <vit_interface.h>
#include "../../patches/monado/runtime-src/monterey_vio_io.h"
#include "vio-quality.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
static uint64_t now(){timespec t{};clock_gettime(CLOCK_MONOTONIC,&t);return uint64_t(t.tv_sec)*1000000000ULL+t.tv_nsec;}
static uint64_t le(const unsigned char *p,unsigned n){uint64_t v=0;for(int i=int(n)-1;i>=0;i--)v=(v<<8)|p[i];return v;}
static float flt(const unsigned char *p){uint32_t v=uint32_t(le(p,4));float x;memcpy(&x,&v,4);return x;}
static void check(vit_result_t r){if(r!=VIT_SUCCESS)throw std::runtime_error("VIT operation failed");}
struct Imu{uint64_t host,tick;std::array<double,6> v;};
struct Map{uint32_t index;unsigned fx,fy;};
int main(int argc,char **argv){
 if(argc!=6){fprintf(stderr,"Usage: live-vit CONFIG MAP FEED OUTPUT.csv SECONDS(1..180)\n");return 2;}
 int fd=-1;vit_tracker_t *tracker=nullptr;FILE *out=nullptr;
 try{
  char *end=nullptr;long seconds=strtol(argv[5],&end,10);if(*end||seconds<1||seconds>180)throw std::runtime_error("invalid duration");
  std::ifstream mf(argv[2],std::ios::binary);std::array<unsigned char,20> mh{};
  if(!mf.read((char*)mh.data(),mh.size()))throw std::runtime_error("map header missing");
  const uint32_t expected[]={0x514d4150,1,320,240};
  for(unsigned i=0;i<4;i++)if(le(mh.data()+4*i,4)!=expected[i])throw std::runtime_error("invalid map header");
  unsigned cameras=le(mh.data()+16,4);if(cameras!=2&&cameras!=4)throw std::runtime_error("invalid map header");
  std::vector<unsigned> ids(cameras);const unsigned allowed[]={0,2,1,3};
  for(unsigned i=0;i<cameras;i++){unsigned char v[4];if(!mf.read((char*)v,4))throw std::runtime_error("map header missing");
   ids[i]=le(v,4);if(ids[i]!=allowed[i])throw std::runtime_error("invalid map camera order");}
  std::vector<Map> maps(cameras*76800);
  for(auto &m:maps){unsigned char b[8];if(!mf.read((char*)b,8))throw std::runtime_error("short map");m={uint32_t(le(b,4)),b[4],b[5]};
   if(m.fx>31||m.fy>31||(m.index!=UINT32_MAX&&(m.index>=76800||(m.index%320==319&&m.fx)||(m.index/320==239&&m.fy))))throw std::runtime_error("invalid map entry");}
  if(mf.peek()!=EOF)throw std::runtime_error("extra map bytes");
  const char *hz_env=getenv("QUEST_VIO_CAMERA_HZ");
  long hz=cameras==4?10:15;
  if(hz_env){char *hz_end;hz=strtol(hz_env,&hz_end,10);if(*hz_end||(hz!=10&&hz!=15&&hz!=30))throw std::runtime_error("camera Hz must be 10, 15 or 30");}
  const uint64_t camera_interval=1000000000ULL/hz-5000000ULL;
  const char *timing_path=getenv("QUEST_VIO_TIMINGS");
  FILE *timings=nullptr;vit_tracker_timing_titles timing_titles{};
  const char *mailbox=getenv("QUEST_VIO_MAILBOX");
  if(mailbox){struct stat st{};if(mailbox[0]!='/'||lstat(mailbox,&st)==0||errno!=ENOENT)throw std::runtime_error("mailbox must be a new absolute path");}
  struct mv_vec head_offset{};bool positional=false;
  const char *offset_file=getenv("QUEST_VIO_HEAD_OFFSET_FILE");
  if(offset_file){
   std::ifstream input(offset_file);std::string extra;
   if(!(input>>head_offset.x>>head_offset.y>>head_offset.z)||(input>>extra)||!mv_finite(head_offset)||mv_norm(head_offset)>.5)
    throw std::runtime_error("invalid head offset calibration");
   if(!mailbox)throw std::runtime_error("head offset requires mailbox");
   positional=true;
  }
  const uint64_t epoch=now();
  unsigned usable=0;struct mq_state quality{};

  auto publish=[&](const vit_pose_data_t &pose,uint64_t host,bool ready){
   if(!mailbox)return;
   struct mv_packet packet{};packet.epoch=epoch;packet.timestamp=uint64_t(pose.timestamp);packet.published=host;
   packet.position={pose.px,pose.py,pose.pz};packet.orientation={pose.ox,pose.oy,pose.oz,pose.ow};
   packet.head_offset=head_offset;packet.ready=positional&&ready;
   packet.velocity={pose.vx,pose.vy,pose.vz};packet.velocity_valid=mv_finite(packet.velocity)&&mv_norm(packet.velocity)<=3.;
   if(packet.ready)usable++;
   unsigned char bytes[MV_WIRE_SIZE];mv_encode(bytes,&packet);
   std::string temporary=std::string(mailbox)+".XXXXXX";
   std::vector<char> name(temporary.begin(),temporary.end());name.push_back(0);
   int output=mkstemp(name.data());if(output<0)throw std::runtime_error("mailbox temporary open failed");
   const ssize_t count=write(output,bytes,sizeof(bytes));const int closed=close(output);
   if(count!=sizeof(bytes)||closed||rename(name.data(),mailbox)){unlink(name.data());throw std::runtime_error("mailbox atomic publish failed");}
  };
  fd=open("/dev/syncboss_stream0",O_RDONLY|O_NONBLOCK);if(fd<0)throw std::runtime_error("passive IMU open failed");
  auto read_imu=[&](){std::vector<Imu> samples;unsigned char b[4096];ssize_t n=read(fd,b,sizeof(b));uint64_t host=now();
   if(n<3||b[0]!=1||b[1]<3||b[1]>n||b[2])return samples;
   for(unsigned off=b[1];off+3<=unsigned(n);){unsigned size=b[off+2];if(off+3+size>unsigned(n))break;
    if(b[off]==0x50&&size>=36){const auto *v=b+off+3;Imu s{host,le(v,8),{}};
     double raw[6];bool valid=true;for(unsigned j=0;j<6;j++){raw[j]=flt(v+8+j*4);valid&=std::isfinite(raw[j]);}
     if(valid){const double rad=3.14159265358979323846/180.;s.v={-raw[1]*9.80665,-raw[0]*9.80665,-raw[2]*9.80665,-raw[4]*rad,-raw[3]*rad,-raw[5]*rad};samples.push_back(s);}}
    off+=3+size;
   }return samples;
  };
  // A short passive clock-offset estimate; no sensor calibration is written.
  uint64_t warm_end=now()+1000000000ULL;int64_t offset=INT64_MAX;unsigned warm_samples=0;
  while(now()<warm_end){pollfd p{fd,POLLIN,0};if(poll(&p,1,20)>0)for(const auto &s:read_imu()){
   if(s.tick>uint64_t(INT64_MAX/1000))throw std::runtime_error("IMU timestamp overflow");
   offset=std::min(offset,int64_t(s.host)-int64_t(s.tick*1000));warm_samples++;}}
  if(warm_samples<500)throw std::runtime_error("insufficient IMU warmup");
  vit_config_t config{argv[1],cameras,1,false};check(vit_tracker_create(&config,&tracker));check(vit_tracker_enable_extension(tracker,VIT_TRACKER_EXTENSION_POSE_FEATURES,true));if(timing_path)check(vit_tracker_enable_extension(tracker,VIT_TRACKER_EXTENSION_POSE_TIMING,true));check(vit_tracker_start(tracker));
  if(timing_path){
   check(vit_tracker_get_timing_titles(tracker,&timing_titles));timings=fopen(timing_path,"wx");if(!timings)throw std::runtime_error("timing output exists or cannot open");
   setvbuf(timings,nullptr,_IOLBF,0);fprintf(timings,"pose_timestamp_ns");
   for(unsigned i=0;i<timing_titles.count;i++)fprintf(timings,",%s",timing_titles.titles[i]);fputc('\n',timings);
  }
  out=fopen(argv[4],"wx");if(!out)throw std::runtime_error("output already exists or cannot open");
  setvbuf(out,nullptr,_IOLBF,0);fprintf(out,"timestamp_ns,x,y,z,qx,qy,qz,qw,driver_timestamp_age_ns,cam0_positive,cam1_positive,shared_positive,quality_ready,slot2_positive,slot3_positive,coverage0,coverage1,coverage2,coverage3,vx,vy,vz\n");
  const uint64_t started=now(),deadline=started+uint64_t(seconds)*1000000000ULL;
  uint64_t last_camera=0,last_imu=0,last_imu_host=started,last_pose=0,next_check=0;
  unsigned images=0,poses=0,imus=0,stale=0,backpressure=0;std::vector<uint64_t> submitted;
  std::vector<unsigned char> feed(307328),pixels(cameras*76800);
  auto drain=[&](){while(true){vit_pose_t *pose=nullptr;check(vit_tracker_pop_pose(tracker,&pose));if(!pose)break;
   vit_pose_data_t p{};check(vit_pose_get_data(pose,&p));const uint64_t host=now();
   if(p.timestamp<=0||uint64_t(p.timestamp)<=last_pose||poses>=submitted.size()||uint64_t(p.timestamp)!=submitted[poses]){vit_pose_destroy(pose);throw std::runtime_error("invalid output timestamp");}
   if(!std::isfinite(p.px)||!std::isfinite(p.py)||!std::isfinite(p.pz)){vit_pose_destroy(pose);throw std::runtime_error("nonfinite pose");}
   if(!std::isfinite(p.ox)||!std::isfinite(p.oy)||!std::isfinite(p.oz)||!std::isfinite(p.ow)){vit_pose_destroy(pose);throw std::runtime_error("nonfinite orientation");}
   last_pose=uint64_t(p.timestamp);
   if(timings){
    vit_pose_timing_t timing{};check(vit_pose_get_timing(pose,&timing));
    if(timing.count!=timing_titles.count){vit_pose_destroy(pose);throw std::runtime_error("timing count mismatch");}
    fprintf(timings,"%lld",(long long)p.timestamp);
    for(unsigned i=0;i<timing.count;i++)fprintf(timings,",%lld",(long long)timing.timestamps[i]);fputc('\n',timings);
   }
   unsigned positive[4]={0},coverage[4]={0},shared=0;
   std::map<int64_t,unsigned> observed;
   for(unsigned cam=0;cam<cameras;cam++){
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
   bool initialized=mq_update_multi(&quality,estimate,positive,coverage,cameras,observed.size(),shared,host);
   try{publish(p,host,initialized);}catch(...){vit_pose_destroy(pose);throw;}
   fprintf(out,"%lld,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%lld,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%.9g,%.9g,%.9g\n",(long long)p.timestamp,p.px,p.py,p.pz,p.ox,p.oy,p.oz,p.ow,(long long)(int64_t(host)-p.timestamp),positive[0],positive[1],shared,unsigned(initialized),positive[2],positive[3],coverage[0],coverage[1],coverage[2],coverage[3],p.vx,p.vy,p.vz);
   vit_pose_destroy(pose);poses++;}};
  while(now()<deadline){
   pollfd p{fd,POLLIN,0};if(poll(&p,1,2)>0)for(const auto &s:read_imu()){
    if(s.tick>uint64_t(INT64_MAX/1000))throw std::runtime_error("IMU timestamp overflow");
    int64_t t=int64_t(s.tick*1000)+offset;if(t<=0||uint64_t(t)<=last_imu)throw std::runtime_error("nonmonotonic IMU");
    last_imu=uint64_t(t);last_imu_host=s.host;
    vit_imu_sample_t sample{t,float(s.v[0]),float(s.v[1]),float(s.v[2]),float(s.v[3]),float(s.v[4]),float(s.v[5])};check(vit_tracker_push_imu_sample(tracker,&sample));imus++;drain();
   }
   const uint64_t host=now();if(host-last_imu_host>100000000ULL)throw std::runtime_error("stale IMU");
   if(host<next_check)continue;
   next_check=host+5000000ULL;
   std::ifstream camera(argv[3],std::ios::binary);
   if(!camera.read((char*)feed.data(),feed.size())||camera.peek()!=EOF)throw std::runtime_error("camera feed size");
   const uint32_t abi[]={0x5143414d,1,320,240,4};for(unsigned i=0;i<5;i++)if(le(feed.data()+4*i,4)!=abi[i])throw std::runtime_error("camera feed ABI");
   const uint64_t camera_now=now();
   uint64_t published=le(feed.data()+120,8),earliest=UINT64_MAX,latest=0,total=0;
   if(published>camera_now||camera_now-published>150000000ULL)throw std::runtime_error("stale camera feed");
   for(unsigned cam=0;cam<cameras;cam++){
    uint64_t stamp=le(feed.data()+24+8*ids[cam],8);
    if(stamp>camera_now||camera_now-stamp>150000000ULL)throw std::runtime_error("stale camera feed");
    earliest=std::min(earliest,stamp);latest=std::max(latest,stamp);total+=stamp;
   }
   if(latest-earliest>1000000ULL){stale++;continue;}
   uint64_t t=total/cameras;
   if(last_camera&&t<last_camera)throw std::runtime_error("camera clock reversed");
   if(last_camera&&t-last_camera<camera_interval)continue;
   // Camera push can block on Basalt's bounded queues. This same thread feeds
   // IMU, so a full image queue can starve the estimator of the IMU it needs to
   // advance. Keep at most two image groups outstanding; continue feeding IMU
   // and select a fresh group when capacity returns rather than aging a backlog.
   if(submitted.size()-poses>=2){backpressure++;continue;}
   for(unsigned cam=0;cam<cameras;cam++){
    const auto *src=feed.data()+128+ids[cam]*76800;auto *dest=pixels.data()+cam*76800;
    for(unsigned j=0;j<76800;j++){const auto &m=maps[cam*76800+j];if(m.index==UINT32_MAX){dest[j]=0;continue;}
     const unsigned x=m.fx,y=m.fy,k=m.index,right=k%320==319?0:1,down=k/320==239?0:320;dest[j]=(src[k]*(32-x)*(32-y)+src[k+right]*x*(32-y)+src[k+down]*(32-x)*y+src[k+down+right]*x*y+512)>>10;}
   }
   submitted.push_back(t);
   for(unsigned cam=0;cam<cameras;cam++){vit_img_sample_t image{};image.cam_index=cam;image.timestamp=int64_t(t);image.data=pixels.data()+cam*76800;image.width=320;image.height=240;image.stride=320;image.size=76800;image.format=VIT_IMAGE_FORMAT_L8;check(vit_tracker_push_img_sample(tracker,&image));}
   last_camera=t;images++;drain();
   if(now()-started>2000000000ULL&&poses==0)throw std::runtime_error("no live poses");
  }
  close(fd);fd=-1;check(vit_tracker_stop(tracker));drain();vit_tracker_destroy(tracker);tracker=nullptr;
  if(timings&&fclose(timings))throw std::runtime_error("timing close failed");
  const int close_result=fclose(out);out=nullptr;if(close_result)throw std::runtime_error("output close failed");
  if(!images||poses!=images)throw std::runtime_error("incomplete live output");
  fprintf(stderr,"Live probe complete: %u images, %u poses, %u IMU samples, %u unsynchronized skips; clock offset %lld ns; %u ready mailbox poses; %u backpressure skips\n",images,poses,imus,stale,(long long)offset,usable,backpressure);
  return 0;
 }catch(const std::exception &e){fprintf(stderr,"Live probe failed: %s\n",e.what());if(fd>=0)close(fd);if(out)fclose(out);
  // External timeout bounds teardown if upstream queues have stalled.
  if(tracker){vit_tracker_stop(tracker);vit_tracker_destroy(tracker);}return 2;}
}
