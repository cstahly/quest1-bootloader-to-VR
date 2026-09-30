from pathlib import Path
s=Path('quest-pmos-bringup/monterey-head-tracking.c').read_text()
s=s.replace('#include <X11/keysym.h>','#include <X11/keysym.h>\n#include <limits.h>\n#include "quest-lens-mesh.h"')
s=s.replace('static volatile sig_atomic_t running=1;','static volatile sig_atomic_t running=1;\nstatic struct quest_lens_mesh lens;\nstatic unsigned long line_color=0xffffff;')
a=s.index('static int project(');b=s.index('int main(',a)
s=s[:a]+'''static int project(XrQuaternionf q,XrVector3f p,int eye,int channel,int w,int h,int *x,int *y) {
 p=rotate(q,p);p.x-=(eye?0.03175f:-0.03175f);
 if(p.z>-.1f)return 0;
 float gx,gy;
 if(!quest_lens_inverse(&lens,eye,channel,p.x/-p.z,p.y/-p.z,&gx,&gy))return 0;
 *x=eye*w+(int)lroundf(gx*w/32.f+lens.center_shift[eye]);
 /* Stock mesh rows increase from bottom to top; X11 pixels increase downward. */
 *y=(int)lroundf((32.f-gy)*h/32.f);return 1;
}
static void line(Display*d,Drawable p,GC gc,XrQuaternionf q,int eye,int w,int h,XrVector3f a,XrVector3f b) {
 for(int channel=0;channel<3;channel++){
  unsigned long color=line_color&(0xffUL<<(16-channel*8));if(!color)continue;
  XSetForeground(d,gc,color);XPoint points[25];int count=0;
  /* A straight scene edge must become a curve on the bare panel. */
  for(int n=0;n<=24;n++){
   float t=n/24.f;XrVector3f v={a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};
   int x,y;
   if(project(q,v,eye,channel,w,h,&x,&y)){points[count++]=(XPoint){(short)x,(short)y};}
   else {if(count>1)XDrawLines(d,p,gc,points,count,CoordModeOrigin);count=0;}
  }
  if(count>1)XDrawLines(d,p,gc,points,count,CoordModeOrigin);
 }
}
''' +s[b:]
s=s.replace('int main(int argc,char **argv) {','''int main(int argc,char **argv) {
 char mesh_path[PATH_MAX];const char *override=getenv("MONTEREY_DISTORTION_MESH");
 if(override)snprintf(mesh_path,sizeof(mesh_path),"%s",override);
 else {const char *slash=strrchr(argv[0],'/');int length=slash?(int)(slash-argv[0]):1;
       snprintf(mesh_path,sizeof(mesh_path),"%.*s/distortion-mesh.bin",length,slash?argv[0]:".");}
 if(!quest_lens_load(&lens,mesh_path))return 2;
 puts("Using stock Quest distortion mesh with RGB correction; no guessed FOV scale");fflush(stdout);''')
s=s.replace('height=DisplayHeight(d,screen);','height=DisplayHeight(d,screen);\n  if(width!=(int)lens.width || height!=(int)lens.height){fputs("Display does not match mesh\\n",stderr);XCloseDisplay(d);return 2;}')
s=s.replace('XSetForeground(d,gc,0);XFillRectangle','XSetFunction(d,gc,GXcopy);XSetForeground(d,gc,0);XFillRectangle')
s=s.replace('for(int eye=0;eye<2;eye++){','XSetFunction(d,gc,GXor);\n   for(int eye=0;eye<2;eye++){')
s=s.replace('XSetForeground(d,gc,0x55d9ae);','line_color=0x55d9ae;').replace('XSetForeground(d,gc,0xffffff);','line_color=0xffffff;')
s=s.replace('XSetClipMask(d,gc,None);','XSetClipMask(d,gc,None);XSetFunction(d,gc,GXcopy);')
Path('quest-pmos-bringup/monterey-head-mesh.c').write_text(s)
