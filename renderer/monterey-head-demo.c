/* Local diagnostic: Monado Monterey pose, X11 wireframe, no persistent writes. */
#define XR_USE_TIMESPEC
#include <time.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <X11/Xlib.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
static volatile sig_atomic_t running=1;
static void stop(int sig) { (void)sig; running=0; }
static long long now_ns(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return (long long)t.tv_sec*1000000000LL+t.tv_nsec; }
static XrQuaternionf conjugate(XrQuaternionf q) { return (XrQuaternionf){-q.x,-q.y,-q.z,q.w}; }
static XrQuaternionf mul(XrQuaternionf a,XrQuaternionf b) {
 return (XrQuaternionf){a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
static XrVector3f rotate(XrQuaternionf q,XrVector3f p) {
 XrQuaternionf r=mul(mul(q,(XrQuaternionf){p.x,p.y,p.z,0}),conjugate(q));
 return (XrVector3f){r.x,r.y,r.z};
}
static int project(XrQuaternionf q,XrVector3f p,int eye,int w,int h,int *x,int *y) {
 p=rotate(q,p); p.x-=(eye?0.03175f:-0.03175f);
 if(p.z>-0.1f)return 0;
 float px=p.x/-p.z,py=p.y/-p.z;
 if(fabsf(px)>1.1f||fabsf(py)>1.1f)return 0;
 *x=eye*w+w/2+(int)(px*w/2);*y=h/2-(int)(py*w/2);return 1;
}
static void line(Display*d,Drawable p,GC gc,XrQuaternionf q,int eye,int w,int h,XrVector3f a,XrVector3f b) {
 int x1,y1,x2,y2;if(project(q,a,eye,w,h,&x1,&y1)&&project(q,b,eye,w,h,&x2,&y2))XDrawLine(d,p,gc,x1,y1,x2,y2);
}
int main(int argc,char **argv) {
 int pose_only=argc>1&&!strcmp(argv[1],"--pose-only");
 Display*d=NULL;Window win=0;Pixmap back=0;GC gc=0;int width=2880,height=1600;
 if(!pose_only){
  d=XOpenDisplay(NULL);if(!d){fprintf(stderr,"Cannot open X display\n");return 2;}
  int screen=DefaultScreen(d);width=DisplayWidth(d,screen);height=DisplayHeight(d,screen);
  XSetWindowAttributes a={.override_redirect=True,.background_pixel=0,.event_mask=KeyPressMask|ExposureMask};
  win=XCreateWindow(d,RootWindow(d,screen),0,0,width,height,0,CopyFromParent,InputOutput,CopyFromParent,CWOverrideRedirect|CWBackPixel|CWEventMask,&a);
  XStoreName(d,win,"Monterey head rotation diagnostic");XMapRaised(d,win);XSync(d,False);
  gc=XCreateGC(d,win,0,NULL);back=XCreatePixmap(d,win,width,height,DefaultDepth(d,screen));
  XSetLineAttributes(d,gc,3,LineSolid,CapRound,JoinRound);
 }
 signal(SIGINT,stop);signal(SIGTERM,stop);signal(SIGALRM,stop);alarm(pose_only?15:60);
 XrInstance instance=XR_NULL_HANDLE; XrSession session=XR_NULL_HANDLE;
 XrSpace local=XR_NULL_HANDLE,view=XR_NULL_HANDLE; XrSystemId system;
 #define CHECK_XR(expr) do { XrResult er=(expr); if(XR_FAILED(er)){fprintf(stderr,"%s failed: %d\n",#expr,er);return 3;} } while(0)
 const char *extensions[]={XR_MND_HEADLESS_EXTENSION_NAME,XR_KHR_CONVERT_TIMESPEC_TIME_EXTENSION_NAME};
 XrInstanceCreateInfo ici={.type=XR_TYPE_INSTANCE_CREATE_INFO,.enabledExtensionCount=2,.enabledExtensionNames=extensions};
 strcpy(ici.applicationInfo.applicationName,"Monterey rotation diagnostic");ici.applicationInfo.apiVersion=XR_MAKE_VERSION(1,0,0);
 CHECK_XR(xrCreateInstance(&ici,&instance));
 XrSystemGetInfo sgi={.type=XR_TYPE_SYSTEM_GET_INFO,.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
 CHECK_XR(xrGetSystem(instance,&sgi,&system));
 XrSystemProperties properties={.type=XR_TYPE_SYSTEM_PROPERTIES};CHECK_XR(xrGetSystemProperties(instance,system,&properties));
 printf("OpenXR system: %s\n",properties.systemName);fflush(stdout);
 if(!strstr(properties.systemName,"Quest 1")){fprintf(stderr,"Expected real Quest 1 HMD\n");return 3;}
 XrSessionCreateInfo sci={.type=XR_TYPE_SESSION_CREATE_INFO,.systemId=system};CHECK_XR(xrCreateSession(instance,&sci,&session));
 PFN_xrConvertTimespecTimeToTimeKHR to_time=NULL;
 CHECK_XR(xrGetInstanceProcAddr(instance,"xrConvertTimespecTimeToTimeKHR",(PFN_xrVoidFunction*)&to_time));
 XrReferenceSpaceCreateInfo rci={.type=XR_TYPE_REFERENCE_SPACE_CREATE_INFO,.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_LOCAL,.poseInReferenceSpace.orientation.w=1};
 CHECK_XR(xrCreateReferenceSpace(session,&rci,&local));rci.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_VIEW;CHECK_XR(xrCreateReferenceSpace(session,&rci,&view));
 int begun=0;
 for(int tries=0;tries<300&&!begun&&running;tries++){
  XrEventDataBuffer event={.type=XR_TYPE_EVENT_DATA_BUFFER};
  while(xrPollEvent(instance,&event)==XR_SUCCESS){
   if(event.type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED&&((XrEventDataSessionStateChanged*)&event)->state==XR_SESSION_STATE_READY){
    XrSessionBeginInfo bi={.type=XR_TYPE_SESSION_BEGIN_INFO,.primaryViewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};CHECK_XR(xrBeginSession(session,&bi));begun=1;
   }
   event.type=XR_TYPE_EVENT_DATA_BUFFER;
  }
  struct timespec pause={0,10000000};nanosleep(&pause,NULL);
 }
 if(!begun){fprintf(stderr,"Session did not become ready\n");return 3;}
 struct timespec settle={0,500000000};nanosleep(&settle,NULL);
 XrQuaternionf origin={0,0,0,1};int frames=0;
 long long start=now_ns();
 while(running){
  struct timespec timestamp;clock_gettime(CLOCK_MONOTONIC,&timestamp);XrTime time;
  CHECK_XR(to_time(instance,&timestamp,&time));
  XrSpaceVelocity velocity={.type=XR_TYPE_SPACE_VELOCITY};XrSpaceLocation location={.type=XR_TYPE_SPACE_LOCATION,.next=&velocity};
  CHECK_XR(xrLocateSpace(view,local,time,&location));
  if(!(location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT)){fprintf(stderr,"Tracking unavailable\n");break;}
  XrQuaternionf q=location.pose.orientation;float norm=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
  if(!isfinite(norm)||fabsf(norm-1)>0.05f){fprintf(stderr,"Invalid quaternion\n");break;}
  if(frames==0)origin=q;
  if(frames%10==0){printf("t=%.3f q=%.6f,%.6f,%.6f,%.6f gyro=%.4f,%.4f,%.4f\n",(now_ns()-start)/1e9,q.x,q.y,q.z,q.w,velocity.angularVelocity.x,velocity.angularVelocity.y,velocity.angularVelocity.z);fflush(stdout);}
  if(d){
   XrQuaternionf camera=conjugate(mul(conjugate(origin),q));
   XSetForeground(d,gc,0);XFillRectangle(d,back,gc,0,0,width,height);
   for(int eye=0;eye<2;eye++){
    XSetForeground(d,gc,0x55d9ae);
    for(int n=-4;n<=4;n++){
     for(int z=-8;z<-1;z++)line(d,back,gc,camera,eye,width/2,height,(XrVector3f){n,-1.5f,z},(XrVector3f){n,-1.5f,z+1});
     for(int x=-4;x<4;x++)line(d,back,gc,camera,eye,width/2,height,(XrVector3f){x,n,-5},(XrVector3f){x+1,n,-5});
    }
    XSetForeground(d,gc,0xffffff);
    XrVector3f v[8];for(int i=0;i<8;i++)v[i]=(XrVector3f){(i&1)?0.6f:-0.6f,(i&2)?0.6f:-0.6f,(i&4)?-2.4f:-3.6f};
    for(int i=0;i<8;i++)for(int bit=1;bit<=4;bit*=2)if(!(i&bit))line(d,back,gc,camera,eye,width/2,height,v[i],v[i|bit]);
   }
   XCopyArea(d,back,win,gc,0,0,width,height,0,0);XFlush(d);
   while(XPending(d)){XEvent e;XNextEvent(d,&e);if(e.type==KeyPress)running=0;}
  }
  frames++;struct timespec pause={0,33333333};nanosleep(&pause,NULL);
 }
 xrRequestExitSession(session);xrDestroySpace(view);xrDestroySpace(local);xrDestroySession(session);xrDestroyInstance(instance);alarm(0);
 if(d){XFreePixmap(d,back);XFreeGC(d,gc);XDestroyWindow(d,win);XCloseDisplay(d);}
 printf("Clean stop after %d frames\n",frames);return frames>1?0:4;
}
