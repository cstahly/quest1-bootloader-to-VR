/* Temporary scene/controller exposure using the same verified command54 as
 * stock-probe.c. No firmware/calibration writes. A camera-service restart
 * restores its configured startup settings. Capture old feed metadata first.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(int argc,char **argv){
 if(argc!=3){fprintf(stderr,"usage: set-exposure MICROSECONDS(1000..12000) GAIN_Q4(16..240)\n");return 2;}
 char *end;long us=strtol(argv[1],&end,10);if(*end||us<1000||us>12000)return 2;
 long gain=strtol(argv[2],&end,10);if(*end||gain<16||gain>240)return 2;
 unsigned char packet[24]={54,0,21};
 for(int i=0;i<4;i++){packet[3+2*i]=us&255;packet[4+2*i]=(us>>8)&255;packet[11+2*i]=gain&255;packet[12+2*i]=(gain>>8)&255;packet[19+i]=1;}
 int fd=open("/dev/syncboss0",O_WRONLY|O_CLOEXEC);if(fd<0){perror("open syncboss");return 2;}
 for(unsigned mode=0;mode<2;mode++){
  packet[23]=mode;ssize_t n;do{n=write(fd,packet,sizeof(packet));}while(n<0&&errno==EINTR);
  if(n!=sizeof(packet)){perror("exposure command");close(fd);return 2;}
 }
 close(fd);puts("Exposure requested; verify published camera metadata and imagery.");return 0;
}
