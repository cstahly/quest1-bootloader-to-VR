/* Bounded diagnostic using only the existing, verified command54 control path.
 * Leaves final runtime settings in place; service restart restores config.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "camera-feed.h"
#include "exposure-policy.h"
static uint64_t now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
int main(int argc,char **argv){
 if(argc!=3){fprintf(stderr,"usage: auto-exposure FEED SECONDS(1..120)\n");return 2;}
 char *end;long seconds=strtol(argv[2],&end,10);if(*end||seconds<1||seconds>120)return 2;
 int fd=open("/dev/syncboss0",O_WRONLY|O_CLOEXEC);if(fd<0){perror("syncboss");return 2;}
 uint64_t deadline=now()+(uint64_t)seconds*1000000000;
 while(now()<deadline){
  struct quest_camera_feed feed;FILE *f=fopen(argv[1],"rb");if(!f){perror("feed");close(fd);return 2;}
  int valid=fread(&feed,1,sizeof(feed),f)==sizeof(feed)&&fgetc(f)==EOF;fclose(f);
  uint64_t host=now();
  if(!valid||feed.magic!=QUEST_CAMERA_MAGIC||feed.version!=1||feed.width!=320||feed.height!=240||feed.cameras!=4||feed.published_ns>host||host-feed.published_ns>150000000){fputs("invalid/stale feed\n",stderr);close(fd);return 2;}
  unsigned char packet[24]={54,0,21};
  struct quest_exposure settings[4];
  quest_exposure_group_next(feed.pixels,feed.exposure_us,feed.gain_q4,settings);
  for(unsigned c=0;c<4;c++){
   if(feed.timestamp_ns[c]>host||host-feed.timestamp_ns[c]>150000000){fputs("stale camera\n",stderr);close(fd);return 2;}
  }
  for(unsigned c=0;c<4;c++){
   struct quest_exposure next=settings[c];
   packet[3+2*c]=next.us&255;packet[4+2*c]=next.us>>8;packet[11+2*c]=next.gain&255;packet[12+2*c]=next.gain>>8;packet[19+c]=1;
   printf("%llu,%u,%u,%u,%u,%u,%u\n",(unsigned long long)host,c,feed.exposure_us[c],feed.gain_q4[c],next.brightness,next.us,next.gain);
  }
  for(unsigned mode=0;mode<2;mode++){packet[23]=mode;ssize_t n;do{n=write(fd,packet,sizeof(packet));}while(n<0&&errno==EINTR);if(n!=sizeof(packet)){perror("command54");close(fd);return 2;}}
  fflush(stdout);struct timespec pause={0,500000000};nanosleep(&pause,NULL);
 }
 close(fd);return 0;
}
