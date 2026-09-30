/* Stationary, native-pixel stereo diagnostic. No sensors or calibration writes. */
#include <X11/Xlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static volatile sig_atomic_t running=1;
static void stop(int sig){(void)sig;running=0;}
static void label(Display*d,Drawable p,GC g,int x,int y,const char*s){XDrawString(d,p,g,x,y,s,strlen(s));}
static void chart(Display*d,Drawable p,GC g,int cx,int cy){
 XSetForeground(d,g,0x55dd77);
 XSetLineAttributes(d,g,1,LineSolid,CapButt,JoinMiter);
 XDrawRectangle(d,p,g,cx-200,cy-200,400,400);
 XDrawLine(d,p,g,cx-22,cy,cx+22,cy);XDrawLine(d,p,g,cx,cy-22,cx,cy+22);
 label(d,p,g,cx-92,cy-155,"STATIONARY SHARPNESS");
 label(d,p,g,cx-92,cy-125,"0123456789 ABCDEFGH");
 for(int k=0;k<3;k++){
  int thickness=1<<k,x=cx-170+k*125;
  for(int n=0;n<8;n++)XFillRectangle(d,p,g,x+n*thickness*2,cy-75,thickness,48);
  for(int n=0;n<8;n++)XFillRectangle(d,p,g,x,cy+45+n*thickness*2,48,thickness);
  char s[24];snprintf(s,sizeof(s),"%d pixel",thickness);label(d,p,g,x,cy-90,s);
 }
 XSetForeground(d,g,0xbbbbbb);label(d,p,g,cx-92,cy+155,"WHITE 0123456789");
 XDrawRectangle(d,p,g,cx-70,cy-18,140,36);
}
int main(int argc,char**argv){
 int shift=argc>1?atoi(argv[1]):0;
 if(shift < -400 || shift > 400){fprintf(stderr,"Shift outside diagnostic range\n");return 2;}
 Display*d=XOpenDisplay(NULL);if(!d){fprintf(stderr,"Cannot open display\n");return 2;}
 int s=DefaultScreen(d),w=DisplayWidth(d,s),h=DisplayHeight(d,s);
 if(w!=2880||h!=1600){fprintf(stderr,"Unexpected X display %dx%d\n",w,h);XCloseDisplay(d);return 2;}
 XSetWindowAttributes a={.override_redirect=True,.background_pixel=0,.event_mask=ExposureMask|KeyPressMask};
 Window win=XCreateWindow(d,RootWindow(d,s),0,0,w,h,0,CopyFromParent,InputOutput,CopyFromParent,CWOverrideRedirect|CWBackPixel|CWEventMask,&a);
 XStoreName(d,win,"Quest stationary optics check");
 GC g=XCreateGC(d,win,0,NULL);XFontStruct*f=XLoadQueryFont(d,"9x15");if(!f)f=XLoadQueryFont(d,"fixed");if(f)XSetFont(d,g,f->fid);
 Pixmap p=XCreatePixmap(d,win,w,h,DefaultDepth(d,s));XSetForeground(d,g,0);XFillRectangle(d,p,g,0,0,w,h);
 chart(d,p,g,w/4+shift,h/2);chart(d,p,g,3*w/4-shift,h/2);
 XMapRaised(d,win);XSync(d,False);
 signal(SIGINT,stop);signal(SIGTERM,stop);signal(SIGALRM,stop);alarm(180);
 printf("Stationary chart %dx%d, horizontal shift %d per eye, 180 second limit\n",w,h,shift);fflush(stdout);
 while(running){XCopyArea(d,p,win,g,0,0,w,h,0,0);XFlush(d);while(XPending(d)){XEvent e;XNextEvent(d,&e);if(e.type==KeyPress)running=0;}usleep(100000);}
 alarm(0);XFreePixmap(d,p);XFreeGC(d,g);if(f)XFreeFont(d,f);XDestroyWindow(d,win);XCloseDisplay(d);puts("Clean stop");return 0;
}
