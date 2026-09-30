/* Four world-anchored camera stations, with accepted per-channel lens warp.
 * Raw sensor views, not calibrated stereo passthrough.
 */
#ifndef QUEST_CAMERA_PANEL_H
#define QUEST_CAMERA_PANEL_H
#include "../../tools/camera/camera-feed.h"
#include <sys/stat.h>
#include "camera-controls.h"
#ifndef CAMERA_FEED_PATH
#define CAMERA_FEED_PATH "/run/quest-camera-root/tmp/camera-feed"
#endif
static struct {
 struct quest_camera_feed feed;
 unsigned char display[4][QUEST_CAMERA_PIXELS],previous[4][QUEST_CAMERA_PIXELS],trail[4][QUEST_CAMERA_PIXELS];
 unsigned previous_frames,last_publication;
 double checked,rate_start,fps;
 int seen,valid;
} camera_panel;
static void camera_panel_read(double now){
 if(now-camera_panel.checked<.025)return;
 camera_panel.checked=now;
 FILE *f=fopen(CAMERA_FEED_PATH,"rb");
 if(!f){camera_panel.valid=0;return;}
 struct quest_camera_feed *next=malloc(sizeof(*next));
 if(!next){fclose(f);camera_panel.valid=0;return;}
 int ok=fread(next,1,sizeof(*next),f)==sizeof(*next)&&fgetc(f)==EOF;
 fclose(f);
 ok=ok&&next->magic==QUEST_CAMERA_MAGIC&&next->version==1&&next->width==320&&next->height==240&&next->cameras==4;
 if(ok)for(int i=0;i<4;i++){
  double age=now-next->timestamp_ns[i]/1e9;
  if(age<-.25||age>.5||!next->frames[i])ok=0;
 }
 camera_panel.valid=ok;
 if(!ok){free(next);return;}
 camera_panel.seen=1;

 if(next->publication!=camera_panel.last_publication){
  camera_panel.feed=*next;camera_panel.last_publication=next->publication;
  if(!camera_panel.rate_start){camera_panel.rate_start=now;camera_panel.previous_frames=next->publication;}
  else if(now-camera_panel.rate_start>=.5){
   camera_panel.fps=next->publication>=camera_panel.previous_frames?(next->publication-camera_panel.previous_frames)/(now-camera_panel.rate_start):0;
   camera_panel.rate_start=now;camera_panel.previous_frames=next->publication;
  }
  unsigned char tone[256];
  for(unsigned v=0;v<256;v++)tone[v]=(unsigned char)lroundf(powf(fmaxf(0,(v-4.f)/251.f),1.f/2.2f)*255);
  for(unsigned cam=0;cam<4;cam++)for(unsigned i=0;i<QUEST_CAMERA_PIXELS;i++){
   unsigned value=next->pixels[cam][i];
   unsigned motion=(unsigned)abs((int)value-camera_panel.previous[cam][i])*10;
   unsigned trail=camera_panel.trail[cam][i]*235U/256;if(motion>trail)trail=motion;if(trail>255)trail=255;
   camera_panel.trail[cam][i]=(unsigned char)trail;camera_panel.previous[cam][i]=(unsigned char)value;
   unsigned display=tone[value];
   if(camera_controls.mode==1){
    unsigned left=i%320?next->pixels[cam][i-1]:value,up=i>=320?next->pixels[cam][i-320]:value;
    display=4U*(abs((int)value-(int)left)+abs((int)value-(int)up));if(display>255)display=255;
   } else if(camera_controls.mode==2){display=display/4+trail;if(display>255)display=255;}
   camera_panel.display[cam][i]=(unsigned char)display;
  }

 }
 free(next);
}
#include "camera-peek.h"
/* Small tessellated planes retain the stock per-channel lens distortion.
 * Rasterize at 2x2 pixels for this software prototype; texture remains raw mono.
 * Four stations sit on a 3m arc left of the cube, in the same 3DoF world. */
struct camera_vertex {float x,y,u,v,iz;int valid;};
static float camera_edge(struct camera_vertex a,struct camera_vertex b,float x,float y){
 return (x-a.x)*(b.y-a.y)-(y-a.y)*(b.x-a.x);
}
static void camera_triangle(struct camera_vertex a,struct camera_vertex b,struct camera_vertex c,int eye,int channel,int cam){
 if(!a.valid||!b.valid||!c.valid)return;
 float area=camera_edge(a,b,c.x,c.y);if(fabsf(area)<.01f)return;
 int minx=(int)floorf(fminf(a.x,fminf(b.x,c.x))/2)*2;
 int maxx=(int)ceilf(fmaxf(a.x,fmaxf(b.x,c.x)));
 int miny=(int)floorf(fminf(a.y,fminf(b.y,c.y))/2)*2;
 int maxy=(int)ceilf(fmaxf(a.y,fmaxf(b.y,c.y)));
 if(minx<eye*1440)minx=eye*1440;
 if(maxx>eye*1440+1439)maxx=eye*1440+1439;
 if(miny<0)miny=0;
 if(maxy>1599)maxy=1599;
 unsigned shift=fb.swap_rb?channel*8:16-channel*8,mask=255U<<shift;
 float inverse_area=1/area;
 float wax=(c.y-b.y)*inverse_area*2,way=-(c.x-b.x)*inverse_area*2;
 float wbx=(a.y-c.y)*inverse_area*2,wby=-(a.x-c.x)*inverse_area*2;
 float rowa=camera_edge(b,c,minx+.5f,miny+.5f)*inverse_area;
 float rowb=camera_edge(c,a,minx+.5f,miny+.5f)*inverse_area;
 float za=a.iz-c.iz,zb=b.iz-c.iz,ua=a.u-c.u,ub=b.u-c.u,va=a.v-c.v,vb=b.v-c.v;
 for(int y=miny;y<=maxy;y+=2,rowa+=way,rowb+=wby){
  float wa=rowa,wb=rowb;
  for(int x=minx;x<=maxx;x+=2,wa+=wax,wb+=wbx){
   if(wa<-.00001f||wb<-.00001f||wa+wb>1.00001f)continue;
   float iz=c.iz+wa*za+wb*zb;if(iz<=0)continue;
   float depth=1/iz;
   int u=(int)((c.u+wa*ua+wb*ub)*depth),v=(int)((c.v+wa*va+wb*vb)*depth);
   if(u<0)u=0;
   if(u>319)u=319;
   if(v<0)v=0;
   if(v>239)v=239;
   unsigned value=(cam==4?(camera_peek.valid?camera_peek.pixels[v*320+u]:24):(camera_panel.valid?camera_panel.display[cam][v*320+u]:24))<<shift;
   /* Coordinates are even and clipped to each eye: all four stores are valid. */
   unsigned *out=&fb.pixels[(1599-y)*fb.stride+2879-x];
   out[0]=(out[0]&~mask)|value;out[-1]=(out[-1]&~mask)|value;
   out[-(int)fb.stride]=(out[-(int)fb.stride]&~mask)|value;
   out[-(int)fb.stride-1]=(out[-(int)fb.stride-1]&~mask)|value;
  }
 }

}
static XrVector3f camera_world_point(int cam,float u,float v){
 if(cam==4)return (XrVector3f){(u-.5f)*1.5f,-1.48f,-2.3f+v*1.1f};
 float angle=(-35.f-32.f*cam)*.01745329252f;
 return (XrVector3f){3*sinf(angle)+(u-.5f)*1.45f*cosf(angle),(.5f-v)*1.0875f,-3*cosf(angle)+(u-.5f)*1.45f*sinf(angle)};
}
struct camera_eye_job {XrQuaternionf rotation;int eye;long long geometry,raster;};
static void *camera_panel_eye(void *arg){
 struct camera_eye_job *job=arg;XrQuaternionf rotation=job->rotation;int eye=job->eye;
 long long geometry=0,raster=0;
 project_clip_fast=1;
 for(int cam=0;cam<(camera_peek.map?5:4);cam++){
  for(int channel=0;channel<3;channel++){
   long long vertex_start=now_ns();
   struct camera_vertex vertices[9][13];
   for(int y=0;y<=8;y++)for(int x=0;x<=12;x++){
    XrVector3f p=rotate(rotation,subtract(camera_world_point(cam,x/12.f,y/8.f),scene_position));p.x-=eye?.03175f:-.03175f;
    struct camera_vertex *v=&vertices[y][x];*v=(struct camera_vertex){0};
    float gx,gy;if(p.z>=-.15f||!quest_lens_project_clipped(&lens,eye,channel,p.x/-p.z,p.y/-p.z,&gx,&gy))continue;
    v->x=eye*1440+gx*45+lens.center_shift[eye];v->y=(32-gy)*50;v->iz=1/-p.z;
    v->u=x*(320.f/12)*v->iz;v->v=y*30.f*v->iz;v->valid=1;
   }
   long long vertex_done=now_ns();geometry+=vertex_done-vertex_start;
   for(int y=0;y<8;y++)for(int x=0;x<12;x++){
    camera_triangle(vertices[y][x],vertices[y][x+1],vertices[y+1][x],eye,channel,cam);
    camera_triangle(vertices[y][x+1],vertices[y+1][x+1],vertices[y+1][x],eye,channel,cam);
   }
   raster+=now_ns()-vertex_done;
  }
  line_color=camera_panel.valid?0xffdd88:0xff4444;
  for(int side=0;side<4;side++){
   const float uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
   line(NULL,0,0,rotation,eye,1440,1600,camera_world_point(cam,uv[side][0],uv[side][1]),camera_world_point(cam,uv[(side+1)%4][0],uv[(side+1)%4][1]));
  }
  if(cam==4)continue;
  /* Two legs ground each panel; camera number is one to four top-edge ticks. */
  for(int leg=0;leg<2;leg++){
   XrVector3f a=camera_world_point(cam,leg?.85f:.15f,1),b=a;b.y=-1.5f;
   line(NULL,0,0,rotation,eye,1440,1600,a,b);
  }
  for(int tick=0;tick<=cam;tick++){
   XrVector3f a=camera_world_point(cam,.42f+tick*.05f,0),b=a;b.y+=.09f;
   line(NULL,0,0,rotation,eye,1440,1600,a,b);
  }
 }
 project_clip_fast=0;job->geometry=geometry;job->raster=raster;return NULL;
}
static void camera_panel_draw(double now,XrQuaternionf rotation){
 if(!fb.pixels)return;
 long long begin=now_ns();camera_panel_read(now);if(!camera_panel.seen)return;
 long long read_done=now_ns();
 /* Keep the CPU renderer on the Quest's four performance cores. Newly
  * created eye workers inherit this per-process affinity; no governor changes. */
 static int affinity_set;
 if(!affinity_set&&sysconf(_SC_NPROCESSORS_ONLN)>=8){
  cpu_set_t cores;CPU_ZERO(&cores);for(int c=4;c<8;c++)CPU_SET(c,&cores);
  if(pthread_setaffinity_np(pthread_self(),sizeof(cores),&cores))perror("camera renderer affinity");
  affinity_set=1;
 }
 /* Disjoint eye columns; feed/framebuffer pointer stay fixed until join. */
 struct camera_eye_job jobs[2]={{.rotation=rotation,.eye=0},{.rotation=rotation,.eye=1}};
 pthread_t worker;int spawned=pthread_create(&worker,NULL,camera_panel_eye,&jobs[1])==0;
 camera_panel_eye(&jobs[0]);
 if(spawned)pthread_join(worker,NULL);else camera_panel_eye(&jobs[1]);
 long long geometry=jobs[0].geometry+jobs[1].geometry,raster=jobs[0].raster+jobs[1].raster;
 static unsigned samples;static long long totals[4];
 totals[0]+=read_done-begin;totals[1]+=geometry;totals[2]+=raster;totals[3]+=now_ns()-begin;
 if(++samples==240){fprintf(stderr,"Camera draw ms: read %.2f vertex %.2f raster %.2f total %.2f\n",totals[0]/240e6,totals[1]/240e6,totals[2]/240e6,totals[3]/240e6);samples=0;memset(totals,0,sizeof(totals));}
}
#endif
