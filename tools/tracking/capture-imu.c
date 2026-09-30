/* Read-only SyncBoss recorder: no stream configuration, calibration or firmware writes.
 * Existing Monado keeps the IMU running. Record host arrival and original timestamps
 * separately; do not invent camera/IMU synchronization. */
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
static uint64_t now_ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000ULL+t.tv_nsec;}
static uint64_t le64(const unsigned char *p){uint64_t v=0;for(int i=7;i>=0;i--)v=(v<<8)|p[i];return v;}
static float lefloat(const unsigned char *p){uint32_t v=(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;float f;memcpy(&f,&v,4);return f;}
int main(int argc,char **argv){
 unsigned seconds=argc==2?(unsigned)atoi(argv[1]):15;if(seconds<1||seconds>120)return 2;
 int fd=open("/dev/syncboss_stream0",O_RDONLY|O_NONBLOCK);if(fd<0){perror("IMU reader");return 2;}
 puts("host_monotonic_ns,device_timestamp,sequence,ax_g,ay_g,az_g,gx_deg_s,gy_deg_s,gz_deg_s");
 uint64_t end=now_ns()+(uint64_t)seconds*1000000000ULL;unsigned samples=0;
 while(now_ns()<end){
  struct pollfd p={fd,POLLIN,0};if(poll(&p,1,100)<=0)continue;
  unsigned char b[4096];ssize_t n=read(fd,b,sizeof(b));uint64_t host=now_ns();
  if(n<3||b[0]!=1||b[1]<3||b[1]>n||b[2])continue;
  for(unsigned off=b[1];off+3<=(unsigned)n;){
   unsigned count=b[off+2];if(off+3+count>(unsigned)n)break;
   if(b[off]==0x50&&count>=36){
    unsigned char *v=b+off+3;printf("%llu,%llu,%u",(unsigned long long)host,(unsigned long long)le64(v),b[off+1]);
    for(int j=0;j<6;j++)printf(",%.9g",lefloat(v+8+j*4));
    putchar('\n');samples++;
   }
   off+=3+count;
  }
 }
 close(fd);fprintf(stderr,"Captured %u IMU samples\n",samples);return samples?0:3;
}
