/* Synthetic mailbox source for OFFSCREEN runtime integration tests only.
 * Never use with a visible renderer. Writes 10cm with a brief unsupported interval, then goes stale.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../../patches/monado/runtime-src/monterey_vio_io.h"
static uint64_t clock_ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
int main(int argc,char **argv){
 if(argc!=2||argv[1][0]!='/')return 2;
 struct stat st;if(lstat(argv[1],&st)==0)return 2;
 struct mv_packet p={.epoch=clock_ns(),.orientation={0,0,0,1},.ready=1};
 struct timespec initial={2,0};nanosleep(&initial,NULL);
 for(unsigned i=0;i<110;i++){
  p.ready=!(i==35||i==36);
  p.published=clock_ns();p.timestamp=p.published-50000000;p.velocity_valid=true;
  p.velocity.x=i>=20&&i<50?.1/1.5:0;
  p.position.x=i<20?0:(i<50?(i-20)*.1/30:.1);
  unsigned char bytes[MV_WIRE_SIZE];mv_encode(bytes,&p);char path[4096];
  if(snprintf(path,sizeof(path),"%s.XXXXXX",argv[1])>=(int)sizeof(path))return 2;
  int fd=mkstemp(path);if(fd<0)return 2;
  ssize_t n=write(fd,bytes,sizeof(bytes));int closed=close(fd);
  if(n!=sizeof(bytes)||closed||rename(path,argv[1])){unlink(path);return 2;}
  struct timespec wait={0,50000000};nanosleep(&wait,NULL);
 }
 return 0;
}
