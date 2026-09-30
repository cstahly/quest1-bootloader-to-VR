/* Diagnostic-only scanout: no modesets or persistent device changes. */
#ifndef QUEST_FRAMEBUFFER_H
#define QUEST_FRAMEBUFFER_H
#include <linux/fb.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
struct framebuffer { int fd; size_t bytes; uint32_t *pixels; void *mapping; size_t map_bytes; unsigned stride; int swap_rb; struct fb_var_screeninfo var;
 pthread_t worker; pthread_mutex_t lock; pthread_cond_t changed;
 uint32_t *spare,*ready; int stop,failed,worker_started;
 double intervals[240]; unsigned interval_count; long long last_submit; };
static long long framebuffer_time_ns(void) {
 struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);
 return (long long)ts.tv_sec*1000000000LL+ts.tv_nsec;
}
static int framebuffer_compare_interval(const void *a,const void *b) {
 double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);
}
static void framebuffer_record_submission(struct framebuffer *f) {
 long long now=framebuffer_time_ns();
 if(f->last_submit)f->intervals[f->interval_count++]=(now-f->last_submit)/1e6;
 f->last_submit=now;
 if(f->interval_count==240){
  double sum=0;unsigned stalls=0;
  for(unsigned i=0;i<240;i++){sum+=f->intervals[i];if(f->intervals[i]>25)stalls++;}
  qsort(f->intervals,240,sizeof(double),framebuffer_compare_interval);
  fprintf(stderr,"Framebuffer submission: %.1f fps; interval p50 %.2f p95 %.2f p99 %.2f max %.2f ms; >25ms %u/240\n",
   240000/sum,f->intervals[119],f->intervals[227],f->intervals[237],f->intervals[239],stalls);
  f->interval_count=0;
 }
}
static void *framebuffer_submit_worker(void *arg) {
 struct framebuffer *f=arg;
 pthread_mutex_lock(&f->lock);
 for(;;){
  while(!f->ready&&!f->stop)pthread_cond_wait(&f->changed,&f->lock);
  if(!f->ready&&f->stop)break;
  uint32_t *ready=f->ready;
  pthread_mutex_unlock(&f->lock);
  memcpy(f->mapping,ready,f->bytes);
  int error=ioctl(f->fd,FBIOPAN_DISPLAY,&f->var);
  if(error)perror("submit framebuffer");
  else framebuffer_record_submission(f);
  pthread_mutex_lock(&f->lock);
  if(error)f->failed=1;
  f->spare=ready;f->ready=NULL;
  pthread_cond_broadcast(&f->changed);
 }
 pthread_mutex_unlock(&f->lock);return NULL;
}
static void framebuffer_close(struct framebuffer *f) {
 if(!f->pixels)return;
 if(f->worker_started){
  pthread_mutex_lock(&f->lock);f->stop=1;pthread_cond_broadcast(&f->changed);pthread_mutex_unlock(&f->lock);
  pthread_join(f->worker,NULL);pthread_cond_destroy(&f->changed);pthread_mutex_destroy(&f->lock);
 }
 munmap(f->mapping,f->map_bytes);close(f->fd);free(f->pixels);free(f->spare);f->pixels=NULL;
}
static int framebuffer_open(struct framebuffer *f,const char *path) {
 struct fb_fix_screeninfo fix;struct fb_var_screeninfo var;
 f->fd=open(path,O_RDWR);if(f->fd<0){perror(path);return 0;}
 if(ioctl(f->fd,FBIOGET_FSCREENINFO,&fix)||ioctl(f->fd,FBIOGET_VSCREENINFO,&var)){perror("framebuffer info");close(f->fd);return 0;}
 /* Refuse an unexpected layout rather than guessing or changing display mode. */
 if(var.xres!=2880||var.yres!=1600||var.bits_per_pixel!=32||var.xoffset||var.yoffset||
    var.green.offset!=8||!((var.red.offset==16&&var.blue.offset==0)||(var.red.offset==0&&var.blue.offset==16))||
    var.red.length!=8||var.green.length!=8||var.blue.length!=8||
    fix.line_length!=2880*4||fix.smem_len<fix.line_length*1600){
  fprintf(stderr,"Unsupported fb: %ux%u %ubpp offsets %u,%u RGB %u/%u/%u stride %u\n",var.xres,var.yres,var.bits_per_pixel,var.xoffset,var.yoffset,var.red.offset,var.green.offset,var.blue.offset,fix.line_length);close(f->fd);return 0;
 }
 f->swap_rb=var.red.offset==0;
 f->stride=fix.line_length/4;f->bytes=(size_t)fix.line_length*1600;f->map_bytes=f->bytes;
 f->mapping=mmap(NULL,f->map_bytes,PROT_READ|PROT_WRITE,MAP_SHARED,f->fd,0);
 if(f->mapping==MAP_FAILED){perror("mmap framebuffer");close(f->fd);return 0;}
 f->pixels=calloc(1,f->bytes);if(!f->pixels){munmap(f->mapping,f->map_bytes);close(f->fd);return 0;}
 f->var=var;f->var.activate=FB_ACTIVATE_VBL;
 if(ioctl(f->fd,FBIOBLANK,FB_BLANK_UNBLANK)){perror("unblank framebuffer");framebuffer_close(f);return 0;}
 f->spare=calloc(1,f->bytes);if(!f->spare){framebuffer_close(f);return 0;}
 pthread_mutex_init(&f->lock,NULL);pthread_cond_init(&f->changed,NULL);
 if(pthread_create(&f->worker,NULL,framebuffer_submit_worker,f)){
  pthread_cond_destroy(&f->changed);pthread_mutex_destroy(&f->lock);framebuffer_close(f);return 0;
 }
 f->worker_started=1;
 puts("Direct framebuffer: 2880x1600, software 180-degree rotation, no modeset");return 1;
}
static void framebuffer_dot(struct framebuffer *f,int x,int y,int eye,uint32_t color) {
 /* Match a 3px wire; clip before rotating both axes into native scanout. */
 for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
  int px=x+dx,py=y+dy;
  if(px<eye*1440||px>=(eye+1)*1440||py<0||py>=1600)continue;
  f->pixels[(1599-py)*f->stride+2879-px]|=color;
 }
}
static void framebuffer_line(struct framebuffer *f,int x,int y,int bx,int by,int eye,uint32_t color) {
 if(f->swap_rb)color=((color&0xff)<<16)|(color&0xff00)|((color>>16)&0xff);
 int dx=abs(bx-x),sx=x<bx?1:-1,dy=-abs(by-y),sy=y<by?1:-1,error=dx+dy;
 framebuffer_dot(f,x,y,eye,color);
 while(x!=bx||y!=by){
  int previous_x=x,previous_y=y,e=2*error;
  if(e>=dy){error+=dy;x+=sx;}if(e<=dx){error+=dx;y+=sy;}
  /* Adjacent 3x3 stamps overlap. Write only their newly exposed border,
   * preserving exactly the same pixel coverage and OR blending. */
  if(x!=previous_x){
   int px=x+sx;
   if(px>=eye*1440&&px<(eye+1)*1440)
    for(int py=y-1;py<=y+1;py++)if(py>=0&&py<1600)
     f->pixels[(1599-py)*f->stride+2879-px]|=color;
  }
  if(y!=previous_y){
   int py=y+sy;
   if(py>=0&&py<1600)
    for(int px=x-1;px<=x+1;px++)
     if((x==previous_x||px!=x+sx)&&px>=eye*1440&&px<(eye+1)*1440)
      f->pixels[(1599-py)*f->stride+2879-px]|=color;
  }
 }
}
static int framebuffer_present(struct framebuffer *f) {
 /* Two CPU buffers: render next while MDSS submits previous. Never overwrite
  * a buffer owned by the submission worker, and never queue stale frames. */
 pthread_mutex_lock(&f->lock);
 while(f->ready&&!f->failed)pthread_cond_wait(&f->changed,&f->lock);
 if(f->failed){pthread_mutex_unlock(&f->lock);return 0;}
 f->ready=f->pixels;f->pixels=f->spare;f->spare=NULL;
 pthread_cond_broadcast(&f->changed);pthread_mutex_unlock(&f->lock);return 1;
}
#endif
