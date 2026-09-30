/*
 * oculus-canvas — a Wi-Fi vector display server for the Quest 1 (pmOS/Nura).
 *
 * The headset is a dumb glowing-line drawer. Your Mac does all the 3D / stereo / audio
 * and sends line segments in ABSOLUTE panel coordinates (it draws both eyes itself, into
 * the left/right halves of the panel). Each UDP datagram = one full frame: the server
 * clears to black, draws the lines (thick+bright for a cheap Tempest glow), and swaps.
 *
 * Wire format (big-endian), one datagram = one frame:
 *   header:  'O' 'C'  ver=2  nlines(u16)
 *   line (12 bytes):  x1 y1 x2 y2 (u16 each, panel pixels)  r g b (u8)  width(u8)
 *
 * Build (on headset or Kali chroot):  cc -O2 -o oculus-canvas oculus-canvas.c -lX11
 * Run  (on headset, as root):
 *   DISPLAY=:0 XDG_RUNTIME_DIR=/run/user/10000 ./oculus-canvas
 */
#include <X11/Xlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PORT 9999
#define MAXPKT 65536

static Display *dpy;
static Window win;
static Pixmap back;
static GC gc;
static Visual *vis;
static int screen, W, H;
static unsigned long rmask,gmask,bmask; static int rsh,gsh,bsh;

static int shift_of(unsigned long m){int s=0; if(!m)return 0; while(!(m&1)){m>>=1;s++;} return s;}
static unsigned long pix(uint8_t r,uint8_t g,uint8_t b){
    return ((((unsigned long)r<<rsh)&rmask)|(((unsigned long)g<<gsh)&gmask)|(((unsigned long)b<<bsh)&bmask));
}

static void draw_frame(const uint8_t *p,int len){
    if(len<5 || p[0]!='O' || p[1]!='C' || p[2]!=2) return;
    /* header: 'O' 'C' ver(1) nlines(u16) = 5 bytes */
    int n=(p[3]<<8)|p[4];
    const uint8_t *q=p+5;
    /* clear to black */
    XSetForeground(dpy,gc,BlackPixel(dpy,screen));
    XFillRectangle(dpy,back,gc,0,0,W,H);
    for(int i=0;i<n && q+12<=p+len;i++,q+=12){
        int x1=(q[0]<<8)|q[1], y1=(q[2]<<8)|q[3], x2=(q[4]<<8)|q[5], y2=(q[6]<<8)|q[7];
        uint8_t r=q[8],g=q[9],b=q[10],w=q[11];
        /* cheap glow: wide dim halo, then bright core */
        XSetLineAttributes(dpy,gc,(w*3<1?1:w*3),LineSolid,CapRound,JoinRound);
        XSetForeground(dpy,gc,pix(r/3,g/3,b/3));
        XDrawLine(dpy,back,gc,x1,y1,x2,y2);
        XSetLineAttributes(dpy,gc,(w<1?1:w),LineSolid,CapRound,JoinRound);
        XSetForeground(dpy,gc,pix(r,g,b));
        XDrawLine(dpy,back,gc,x1,y1,x2,y2);
    }
    XCopyArea(dpy,back,win,gc,0,0,W,H,0,0);
    XFlush(dpy);
}

int main(void){
    dpy=XOpenDisplay(NULL);
    if(!dpy){fprintf(stderr,"cannot open DISPLAY (set DISPLAY=:0)\n");return 1;}
    screen=DefaultScreen(dpy);
    W=DisplayWidth(dpy,screen); H=DisplayHeight(dpy,screen);
    vis=DefaultVisual(dpy,screen);
    rmask=vis->red_mask; gmask=vis->green_mask; bmask=vis->blue_mask;
    rsh=shift_of(rmask); gsh=shift_of(gmask); bsh=shift_of(bmask);

    XSetWindowAttributes wa; wa.override_redirect=True; wa.background_pixel=BlackPixel(dpy,screen);
    win=XCreateWindow(dpy,RootWindow(dpy,screen),0,0,W,H,0,CopyFromParent,InputOutput,
                      vis,CWOverrideRedirect|CWBackPixel,&wa);
    XMapRaised(dpy,win);
    back=XCreatePixmap(dpy,win,W,H,DefaultDepth(dpy,screen));
    gc=XCreateGC(dpy,back,0,NULL);
    XSetForeground(dpy,gc,BlackPixel(dpy,screen));
    XFillRectangle(dpy,back,gc,0,0,W,H);

    int sock=socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in a; memset(&a,0,sizeof a);
    a.sin_family=AF_INET; a.sin_addr.s_addr=htonl(INADDR_ANY); a.sin_port=htons(PORT);
    if(bind(sock,(struct sockaddr*)&a,sizeof a)<0){perror("bind");return 1;}
    fprintf(stderr,"oculus-canvas: %dx%d TrueColor, listening UDP :%d\n",W,H,PORT);

    uint8_t *buf=malloc(MAXPKT);
    for(;;){ int n=recv(sock,buf,MAXPKT,0); if(n>0) draw_frame(buf,n); }
    return 0;
}
