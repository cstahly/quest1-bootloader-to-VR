#include "framebuffer.h"
#include <assert.h>
static void reference_line(struct framebuffer *f,int x,int y,int bx,int by,int eye,uint32_t color) {
 if(f->swap_rb)color=((color&0xff)<<16)|(color&0xff00)|((color>>16)&0xff);
 int dx=abs(bx-x),sx=x<bx?1:-1,dy=-abs(by-y),sy=y<by?1:-1,error=dx+dy;
 for(;;){framebuffer_dot(f,x,y,eye,color);if(x==bx&&y==by)break;int e=2*error;if(e>=dy){error+=dy;x+=sx;}if(e<=dx){error+=dx;y+=sy;}}
}
int main(void) {
 struct framebuffer f={.stride=2880,.bytes=2880*1600*4};
 uint32_t *allocation=calloc(2880*1600+2,4);assert(allocation);
 allocation[0]=0x12345678;allocation[2880*1600+1]=0x87654321;f.pixels=allocation+1;
 framebuffer_line(&f,0,0,0,0,0,0xff0000);
 assert(f.pixels[1599*2880+2879]==0xff0000); /* native 180 degree rotation */
 assert(f.pixels[0]==0);
 memset(f.pixels,0,f.bytes);f.swap_rb=1;
 framebuffer_line(&f,1440,100,1440,100,0,0xff0000);
 assert(f.pixels[(1599-100)*2880+1440]==0xff); /* left eye x1439 */
 assert(f.pixels[(1599-100)*2880+1439]==0); /* clipped right-eye x1440 */
 framebuffer_line(&f,-28,-10,2910,1610,1,0x00ff00);
 assert(allocation[0]==0x12345678&&allocation[2880*1600+1]==0x87654321);
 struct framebuffer reference=f;reference.pixels=calloc(1,f.bytes);assert(reference.pixels);
 memset(f.pixels,0,f.bytes);
 const int lines[][4]={{0,0,100,0},{100,0,0,0},{0,0,0,100},{0,100,0,0},
  {20,20,150,80},{150,80,20,20},{20,80,150,20},{150,20,20,80},
  {20,20,80,150},{80,150,20,20},{20,150,80,20},{80,20,20,150},
  {-10,-10,2910,1610},{1450,1610,1430,-10},{-10,1599,2910,1599},{2880,20,2879,20}};
 for(unsigned i=0;i<sizeof(lines)/sizeof(lines[0]);i++){
  unsigned color=0x13579b^(i*0x119);
  for(int eye=0;eye<2;eye++){
   framebuffer_line(&f,lines[i][0],lines[i][1],lines[i][2],lines[i][3],eye,color);
   reference_line(&reference,lines[i][0],lines[i][1],lines[i][2],lines[i][3],eye,color);
   assert(memcmp(f.pixels,reference.pixels,f.bytes)==0);
  }
 }
 free(reference.pixels);
 free(allocation);puts("Framebuffer rotation, RGB ordering, stereo clip and bounds pass");return 0;
}
