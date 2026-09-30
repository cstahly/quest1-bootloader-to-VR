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

static void check(vit_result_t r) { if(r!=VIT_SUCCESS)throw std::runtime_error("VIT operation failed"); }
static uint64_t le(const unsigned char *p,int n){uint64_t x=0;for(int i=n-1;i>=0;i--)x=(x<<8)|p[i];return x;}
int main(int argc,char **argv){
 if(argc!=4){fprintf(stderr,"Usage: replay-vit CONFIG EVENTS OUTPUT.csv\n");return 2;}
 try{
  vit_config_t config={argv[1],4,1,false};vit_tracker_t *tracker=nullptr;
  check(vit_tracker_create(&config,&tracker));check(vit_tracker_start(tracker));
  std::ifstream input(argv[2],std::ios::binary);if(!input)throw std::runtime_error("events open failed");
  FILE *out=fopen(argv[3],"w");if(!out)throw std::runtime_error("pose output open failed");
  fprintf(out,"timestamp_ns,x,y,z,qx,qy,qz,qw\n");unsigned poses=0,groups=0;
  auto drain=[&](){while(true){vit_pose_t *pose=nullptr;check(vit_tracker_pop_pose(tracker,&pose));if(!pose)break;
   vit_pose_data_t p;check(vit_pose_get_data(pose,&p));
   fprintf(out,"%lld,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",(long long)p.timestamp,p.px,p.py,p.pz,p.ox,p.oy,p.oz,p.ow);
   vit_pose_destroy(pose);poses++;}};
  unsigned char header[13];
  while(input.read((char*)header,sizeof(header))){
   uint64_t timestamp=le(header+1,8);unsigned size=(unsigned)le(header+9,4);
   if(size>307200)throw std::runtime_error("oversize event");
   std::vector<unsigned char> data(size);if(!input.read((char*)data.data(),size))throw std::runtime_error("truncated event");
   if(header[0]=='I'){
    if(size!=24)throw std::runtime_error("invalid IMU event");float v[6];memcpy(v,data.data(),24);
    vit_imu_sample_t sample={(int64_t)timestamp,v[0],v[1],v[2],v[3],v[4],v[5]};
    check(vit_tracker_push_imu_sample(tracker,&sample));
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
  for(int i=0;i<100;i++){drain();std::this_thread::sleep_for(std::chrono::milliseconds(20));}
  fflush(out);fprintf(stderr,"Replay: %u groups, %u poses\n",groups,poses);
  check(vit_tracker_stop(tracker));drain();vit_tracker_destroy(tracker);fclose(out);
  return poses?0:3;
 }catch(const std::exception &e){fprintf(stderr,"%s\n",e.what());return 2;}
}
