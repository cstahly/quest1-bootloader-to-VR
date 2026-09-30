/* Independent read-only evdev reader. Never grabs inputs or consumes the
 * recovery guard's stream. Stick changes drawing depth; click cycles camera effects. */
#ifndef QUEST_CAMERA_CONTROLS_H
#define QUEST_CAMERA_CONTROLS_H
#include <dirent.h>
#include <linux/input.h>
static struct {int fd[2],initialized,middle,peek,mode,draw;double scanned;float distance;} camera_controls={.fd={-1,-1},.distance=2.5f};
static void camera_controls_poll(double now){
 if(now-camera_controls.scanned>1){
  camera_controls.scanned=now;
  for(int i=0;i<2;i++)if(camera_controls.fd[i]>=0){close(camera_controls.fd[i]);camera_controls.fd[i]=-1;}
  DIR *d=opendir("/dev/input");int count=0;struct dirent *entry;
  if(d){while(count<2&&(entry=readdir(d))){
   if(strncmp(entry->d_name,"event",5))continue;
   char path[300],name[128]={0};snprintf(path,sizeof(path),"/dev/input/%s",entry->d_name);
   int fd=open(path,O_RDONLY|O_NONBLOCK|O_CLOEXEC);if(fd<0)continue;
   if(ioctl(fd,EVIOCGNAME(sizeof(name)),name)<0||strcmp(name,"Oculus Touch desktop pointer")){close(fd);continue;}
   camera_controls.fd[count++]=fd;
  }closedir(d);}
 }
 int peek=0,middle=0,draw=0;
 for(int i=0;i<2;i++)if(camera_controls.fd[i]>=0){
  unsigned char keys[(KEY_MAX+8)/8]={0};
  if(ioctl(camera_controls.fd[i],EVIOCGKEY(sizeof(keys)),keys)<0){close(camera_controls.fd[i]);camera_controls.fd[i]=-1;continue;}
  int trigger=!!(keys[BTN_TRIGGER/8]&(1U<<(BTN_TRIGGER%8)));
  draw|=!!(keys[BTN_LEFT/8]&(1U<<(BTN_LEFT%8)))&&!trigger;
  struct input_event events[32];ssize_t bytes;
  for(int batch=0;batch<8&&(bytes=read(camera_controls.fd[i],events,sizeof(events)))>0;batch++){
   for(unsigned j=0;j<(unsigned)bytes/sizeof(events[0]);j++)
    if(events[j].type==EV_REL&&events[j].code==REL_Y)camera_controls.distance-=events[j].value*.001f;
  }
  peek|=!!(keys[BTN_RIGHT/8]&(1U<<(BTN_RIGHT%8)));
  middle|=!!(keys[BTN_MIDDLE/8]&(1U<<(BTN_MIDDLE%8)));
 }
 if(camera_controls.initialized&&middle&&!camera_controls.middle)camera_controls.mode=(camera_controls.mode+1)%3;
 camera_controls.initialized=1;camera_controls.middle=middle;camera_controls.peek=peek;camera_controls.draw=draw;
 camera_controls.distance=fminf(8,fmaxf(.4f,camera_controls.distance));
}
#endif
