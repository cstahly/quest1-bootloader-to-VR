/* Low-pass before2x decimation. Kernel is centred at the existing (2x,2y)
 * sample, so factory calibration/remap coordinates do not shift. */
#ifndef QUEST_CAMERA_RESAMPLE_H
#define QUEST_CAMERA_RESAMPLE_H
static unsigned quest_camera_downsample(const unsigned char *src,unsigned char *dst,int denoise){
 unsigned total=0;
 for(unsigned y=0;y<240;y++)for(unsigned x=0;x<320;x++){
  unsigned sx=2*x,sy=2*y,value;
  if(!denoise)value=src[sy*640+sx];
  else{
   unsigned left=sx?sx-1:0,up=sy?sy-1:0;
   unsigned a=src[up*640+left]+2*src[up*640+sx]+src[up*640+sx+1];
   unsigned b=src[sy*640+left]+2*src[sy*640+sx]+src[sy*640+sx+1];
   unsigned c=src[(sy+1)*640+left]+2*src[(sy+1)*640+sx]+src[(sy+1)*640+sx+1];
   value=(a+2*b+c+8)/16;
  }
  dst[y*320+x]=(unsigned char)value;total+=value;
 }
 return total;
}
#endif
