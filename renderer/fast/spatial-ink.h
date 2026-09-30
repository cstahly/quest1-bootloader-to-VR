/* Rotation-tracked laser pen: head aim, A/X to draw, stick changes depth.
 * Does not claim tracked hand position. Saves on release, never every frame. */
#ifndef QUEST_SPATIAL_INK_H
#define QUEST_SPATIAL_INK_H
#ifndef QUEST_INK_PATH
#define QUEST_INK_PATH "/var/lib/monado/monterey/spatial-ink.bin"
#endif
#define QUEST_INK_POINTS 2048
struct ink_point {XrVector3f point;unsigned start;};
static struct {struct ink_point points[QUEST_INK_POINTS];unsigned count;int loaded,down,dirty;double sampled;} ink;
static int ink_save(void){
 if(!ink.dirty)return 1;
 char path[PATH_MAX];snprintf(path,sizeof(path),"%s.new",QUEST_INK_PATH);
 FILE *f=fopen(path,"wb");if(!f)return 0;
 unsigned header[3]={0x51494e4b,1,ink.count};
 int ok=fwrite(header,sizeof(header),1,f)==1&&fwrite(ink.points,sizeof(ink.points[0]),ink.count,f)==ink.count;
 if(fclose(f))ok=0;
 if(ok&&rename(path,QUEST_INK_PATH)==0){ink.dirty=0;return 1;}return 0;
}
static void ink_load(void){
 if(ink.loaded)return;
 ink.loaded=1;
 FILE *f=fopen(QUEST_INK_PATH,"rb");if(!f)return;
 unsigned h[3];struct ink_point *points=malloc(sizeof(ink.points));
 int ok=points&&fread(h,sizeof(h),1,f)==1&&h[0]==0x51494e4b&&h[1]==1&&h[2]<=QUEST_INK_POINTS;
 ok=ok&&fread(points,sizeof(*points),h[2],f)==h[2]&&fgetc(f)==EOF;fclose(f);
 if(ok)for(unsigned i=0;i<h[2];i++){
  XrVector3f p=points[i].point;
  if(!isfinite(p.x)||!isfinite(p.y)||!isfinite(p.z)||fabsf(p.x)>9||fabsf(p.y)>9||fabsf(p.z)>9||points[i].start>1){ok=0;break;}
 }
 if(ok){memcpy(ink.points,points,h[2]*sizeof(*points));ink.count=h[2];}free(points);
}
static void ink_segment(XrQuaternionf camera,int eye,XrVector3f a,XrVector3f b,unsigned color){
 a=rotate(camera,subtract(a,scene_position));b=rotate(camera,subtract(b,scene_position));
 for(int c=0;c<3;c++){
  int ax,ay,bx,by;
  if(project(a,eye,c,1440,1600,&ax,&ay)&&project(b,eye,c,1440,1600,&bx,&by))
   framebuffer_line(&fb,ax,ay,bx,by,eye,color&(255U<<(16-c*8)));
 }
}
static void ink_draw(double now,XrQuaternionf camera){
 if(!fb.pixels)return;
 ink_load();
 XrVector3f tip=add(scene_position,rotate(conjugate(camera),(XrVector3f){0,0,-camera_controls.distance}));
 int down=camera_controls.draw;
 if(down&&ink.count<QUEST_INK_POINTS&&(!ink.down||now-ink.sampled>=.025)){
  XrVector3f previous=ink.count?ink.points[ink.count-1].point:tip;
  float dx=tip.x-previous.x,dy=tip.y-previous.y,dz=tip.z-previous.z;
  if(!ink.down||dx*dx+dy*dy+dz*dz>.0001f){
   ink.points[ink.count++]=(struct ink_point){tip,!ink.down};ink.dirty=1;ink.sampled=now;
  }
 }
 if(!down&&ink.down&&!ink_save())perror("save spatial ink");
 ink.down=down;
 project_clip_fast=1;
 for(int eye=0;eye<2;eye++){
  for(unsigned i=1;i<ink.count;i++)if(!ink.points[i].start)ink_segment(camera,eye,ink.points[i-1].point,ink.points[i].point,0xff55dd);
  float radius=camera_controls.distance*.006f;
  XrVector3f left=add(scene_position,rotate(conjugate(camera),(XrVector3f){-radius,0,-camera_controls.distance}));
  XrVector3f right=add(scene_position,rotate(conjugate(camera),(XrVector3f){radius,0,-camera_controls.distance}));
  XrVector3f up=add(scene_position,rotate(conjugate(camera),(XrVector3f){0,radius,-camera_controls.distance}));
  XrVector3f bottom=add(scene_position,rotate(conjugate(camera),(XrVector3f){0,-radius,-camera_controls.distance}));
  ink_segment(camera,eye,left,right,down?0xff55dd:0x88ccff);ink_segment(camera,eye,up,bottom,down?0xff55dd:0x88ccff);
 }
 project_clip_fast=0;
}
#endif
