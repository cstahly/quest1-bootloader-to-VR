/* Head-locked optical HUD. Cache projected text as sparse pixels at 2 Hz;
 * do not run commands, network requests, or lens inversion per glyph per frame.
 * Included after project(), with framebuffer and OpenXR types available.
 */
#ifndef QUEST_DIAGNOSTIC_HUD_H
#define QUEST_DIAGNOSTIC_HUD_H
#include <arpa/inet.h>
#include <ifaddrs.h>
#define HUD_PIXELS 400000
#ifndef HUD_RECOVERY_STATUS_PATH
#define HUD_RECOVERY_STATUS_PATH "/run/oculus-recovery/status"
#endif
struct hud_pixel { unsigned index,color; };
static struct {
 struct hud_pixel pixels[HUD_PIXELS]; unsigned count;
 double previous,updated,sample_start; unsigned sample_frames;
 double intervals[90],draw_sum,wait_sum,fps,draw,wait; unsigned history;
} hud;
/* Original 5x7 bitmap alphabet; rows use their low five bits, left to right. */
static const unsigned char hud_glyphs[][7]={
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,1,14},
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{15,16,16,16,16,16,15},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {15,16,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,25,21,19,19,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
 {0,0,0,0,0,6,6},{0,6,6,0,6,6,0},{0,0,0,31,0,0,0},{1,2,2,4,8,8,16},
 {14,17,1,2,4,0,4}
};
static const unsigned char *hud_glyph(char c) {
 if(c>='0'&&c<='9')return hud_glyphs[c-'0'];
 if(c>='A'&&c<='Z')return hud_glyphs[10+c-'A'];
 static const char punctuation[]=".:-/?";
 const char *p=strchr(punctuation,c);return p?hud_glyphs[36+(p-punctuation)]:NULL;
}
static void hud_pixel(int eye,int x,int y,unsigned color) {
 if(x<eye*1440||x>=(eye+1)*1440||y<0||y>=1600||hud.count==HUD_PIXELS)return;
 if(fb.swap_rb)color=((color&255)<<16)|(color&0xff00)|((color>>16)&255);
 hud.pixels[hud.count++]=(struct hud_pixel){(unsigned)((1599-y)*fb.stride+2879-x),color};
}
static void hud_stroke(int eye,float ax,float ay,float bx,float by,unsigned color) {
 for(int channel=0;channel<3;channel++) {
  unsigned part=color&(255U<<(16-channel*8));if(!part)continue;
  int x,y,endx,endy;
  if(!project((XrVector3f){ax,ay,-2},eye,channel,1440,1600,&x,&y)||
     !project((XrVector3f){bx,by,-2},eye,channel,1440,1600,&endx,&endy))continue;
  int dx=abs(endx-x),sx=x<endx?1:-1,dy=-abs(endy-y),sy=y<endy?1:-1,err=dx+dy;
  for(;;){
   hud_pixel(eye,x,y,part);hud_pixel(eye,x+1,y,part);hud_pixel(eye,x,y+1,part);hud_pixel(eye,x+1,y+1,part);
   if(x==endx&&y==endy)break;
   int e2=2*err;if(e2>=dy){err+=dy;x+=sx;}if(e2<=dx){err+=dx;y+=sy;}
  }
 }
}
static void hud_text(int eye,float y,const char *text,unsigned color) {
 const float dot=.006f;float x=-.72f;
 for(unsigned i=0;text[i]&&i<40;i++,x+=6*dot){
  const unsigned char *g=hud_glyph(text[i]);if(!g)continue;
  for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(g[row]&(16>>col)){
   int end=col;while(end+1<5&&(g[row]&(16>>(end+1))))end++;
   hud_stroke(eye,x+col*dot,y-row*dot,x+(end+.75f)*dot,y-row*dot,color);col=end;
  }
 }
}
static void hud_wifi(char *text,size_t size) {
 struct ifaddrs *list=NULL;char address[INET_ADDRSTRLEN]="";
 if(getifaddrs(&list)==0){for(struct ifaddrs *p=list;p;p=p->ifa_next){
  if(p->ifa_addr&&!strcmp(p->ifa_name,"wlan0")&&p->ifa_addr->sa_family==AF_INET)
   inet_ntop(AF_INET,&((struct sockaddr_in *)p->ifa_addr)->sin_addr,address,sizeof(address));
  }
  freeifaddrs(list);
 }
 int signal_dbm=0;FILE *f=fopen("/proc/net/wireless","r");char linebuf[256];
 if(f){while(fgets(linebuf,sizeof(linebuf),f)){
  char iface[32];unsigned flags;float quality,level;
  if(sscanf(linebuf," %31[^:]: %x %f %f",iface,&flags,&quality,&level)==4&&!strcmp(iface,"wlan0"))signal_dbm=(int)level;
 }fclose(f);}
 if(address[0]&&signal_dbm<0&&signal_dbm>=-127)snprintf(text,size,"WIFI %s %d DBM",address,signal_dbm);
 else if(address[0])snprintf(text,size,"WIFI %s RSSI N/A",address);
 else snprintf(text,size,"WIFI CONNECTING / NO ADDRESS");
}
static int hud_recovery(double now,double *remaining,unsigned *resets) {
 FILE *f=fopen(HUD_RECOVERY_STATUS_PATH,"r");if(!f)return 0;
 int pid;unsigned long long ticks;double deadline,updated;
 int n=fscanf(f,"%d %llu %lf %lf %u",&pid,&ticks,&deadline,&updated,resets);fclose(f);
 if(n!=5||updated>now+.25||now-updated>2||pid<2||kill(pid,0))return 0;
 *remaining=fmax(0,deadline-now);return 1;
}
#include "camera-panel.h"
#include "spatial-ink.h"
static void hud_draw(double now,float turn) {
 if(!fb.pixels)return;
 if(now-hud.updated>=.5){
  hud.updated=now;hud.count=0;
  char rows[6][80];double remaining=0;unsigned resets=0;int guarded=hud_recovery(now,&remaining,&resets);
  snprintf(rows[0],80,"QUEST / LIVE DIAGNOSTICS");
  snprintf(rows[1],80,"LOOP %.1f FPS  DRAW %.1f MS",hud.fps,hud.draw);
  snprintf(rows[2],80,"WAIT %.1f MS  TURN %.0f DEG/S",hud.wait,turn);
  hud_wifi(rows[3],80);
  if(guarded)snprintf(rows[4],80,"RECOVERY %02d:%02d  RESETS %u",(int)remaining/60,(int)remaining%60,resets);
  else snprintf(rows[4],80,"RECOVERY STATUS UNAVAILABLE");
  snprintf(rows[5],80,"HOLD TRIGGER 2S FOR 5 MIN");
  for(int eye=0;eye<2;eye++){
   char controls[80];
   snprintf(controls,sizeof(controls),"INK %u/2048  DEPTH %.1fM  %s",ink.count,camera_controls.distance,camera_controls.mode==1?"EDGES":camera_controls.mode==2?"TRAILS":"CAMERA");
   hud_text(eye,.23f,controls,0xff88dd);
   hud_text(eye,.153f,"A/X DRAW  STICK DEPTH  CLICK FX",0xff88dd);
   hud_text(eye,.076f,"FLOOR CAMERA / PHYSICAL TRACKING TBD",0xffdd88);
   
   for(int row=0;row<6;row++)hud_text(eye,.98f-row*.077f,rows[row],row==4?(!guarded||remaining<30?0xff9955:0x55ffaa):0xaadfff);
   /* Last 90 loop intervals, 0..40 ms. Reference line is 13.9 ms, not a claim
    * about panel rate; spikes above 25 ms are orange. */
   hud_stroke(eye,-.72f,.36f,.70f,.36f,0x447766);
   hud_stroke(eye,-.72f,.36f+.12f*13.9f/40,.70f,.36f+.12f*13.9f/40,0x447766);
   for(unsigned i=0;i<90;i++){
    double ms=hud.intervals[(hud.history+i)%90];float x=-.72f+i*.016f;
    hud_stroke(eye,x,.36f,x,.36f+(float)fmin(ms,40)*.003f,ms>25?0xff9955:0x55ffaa);
   }
  }
 }
 for(unsigned i=0;i<hud.count;i++)fb.pixels[hud.pixels[i].index]|=hud.pixels[i].color;
}
static void hud_sample(double now,double draw_ms,double wait_ms) {
 if(hud.previous){hud.intervals[hud.history++%90]=(now-hud.previous)*1000;}
 hud.previous=now;
 if(!hud.sample_start)hud.sample_start=now;
 hud.draw_sum+=draw_ms;hud.wait_sum+=wait_ms;hud.sample_frames++;
 if(now-hud.sample_start>=.5){hud.fps=hud.sample_frames/(now-hud.sample_start);
  hud.draw=hud.draw_sum/hud.sample_frames;hud.wait=hud.wait_sum/hud.sample_frames;
  hud.sample_start=now;hud.sample_frames=0;hud.draw_sum=0;hud.wait_sum=0;}
}
#endif
