#include "quest-lens-mesh.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint32_t le32(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float lefloat(const unsigned char*p){uint32_t u=le32(p);float f;memcpy(&f,&u,4);return f;}
static float clamp(float x,float a,float b){return fmaxf(a,fminf(b,x));}
static void evaluate(const struct quest_lens_mesh*m,int e,int c,float x,float y,float out[2],float jac[4]){
 int ix=(int)floorf(x),iy=(int)floorf(y);if(ix>31)ix=31;if(iy>31)iy=31;
 float u=x-ix,v=y-iy;
 for(int k=0;k<2;k++){
  float a=m->ray[e][iy][ix][c][k],b=m->ray[e][iy][ix+1][c][k];
  float d=m->ray[e][iy+1][ix][c][k],f=m->ray[e][iy+1][ix+1][c][k];
  out[k]=(1-v)*((1-u)*a+u*b)+v*((1-u)*d+u*f);
  jac[k*2]=(1-v)*(b-a)+v*(f-d);
  jac[k*2+1]=(1-u)*(d-a)+u*(f-b);
 }
}
int quest_lens_ray(const struct quest_lens_mesh*m,int e,int c,float x,float y,float out[2]){
 if(e<0||e>1||c<0||c>2||!isfinite(x)||!isfinite(y)||x<0||x>32||y<0||y>32)return 0;
 float j[4];evaluate(m,e,c,x,y,out,j);return 1;
}
int quest_lens_inverse(const struct quest_lens_mesh*m,int e,int c,float tx,float ty,float *gx,float *gy){
 if(e<0||e>1||c<0||c>2||!isfinite(tx)||!isfinite(ty))return 0;
 float x=clamp(16+16*tx,0,32),y=clamp(16+16*ty,0,32);
 for(int i=0;i<24;i++){
  float p[2],j[4];evaluate(m,e,c,x,y,p,j);
  float ex=p[0]-tx,ey=p[1]-ty;
  if(ex*ex+ey*ey<1e-12f){*gx=x;*gy=y;return 1;}
  float det=j[0]*j[3]-j[1]*j[2];if(!isfinite(det)||det<1e-7f)return 0;
  float dx=(j[3]*ex-j[1]*ey)/det,dy=(-j[2]*ex+j[0]*ey)/det;
  float scale=fmaxf(1,fmaxf(fabsf(dx),fabsf(dy))/4);
  x=clamp(x-dx/scale,0,32);y=clamp(y-dy/scale,0,32);
 }
 return 0;
}
int quest_lens_load(struct quest_lens_mesh*m,const char*path){
 unsigned char bytes[52368];FILE*f=fopen(path,"rb");if(!f){perror(path);return 0;}
 size_t n=fread(bytes,1,sizeof(bytes),f);int extra=fgetc(f);fclose(f);
 if(n!=sizeof(bytes)||extra!=EOF||le32(bytes)!=0x56347807||le32(bytes+4)!=0||
    le32(bytes+24)!=32||le32(bytes+28)!=32||le32(bytes+48)!=2880||le32(bytes+52)!=1600){
  fputs("Unsupported or damaged Quest distortion mesh\n",stderr);return 0;
 }
 memset(m,0,sizeof(*m));m->width=2880;m->height=1600;
 /* On-disk order: grid row, eye, column, then three (x,y) tangent-ray pairs. */
 for(int y=0;y<33;y++)for(int e=0;e<2;e++)for(int x=0;x<33;x++)for(int c=0;c<3;c++)for(int k=0;k<2;k++){
  size_t off=96+(((y*2+e)*33+x)*6+c*2+k)*4;
  float a=lefloat(bytes+off);if(!isfinite(a)||fabsf(a)>20)return 0;m->ray[e][y][x][c][k]=a;
 }
 /* Reject folds/singular cells before attempting inversion. */
 for(int e=0;e<2;e++)for(int c=0;c<3;c++)for(int y=0;y<32;y++)for(int x=0;x<32;x++)
  for(int corner=0;corner<4;corner++){
   float ray[2],j[4];evaluate(m,e,c,x+((corner&1)?1.f:0.f),y+((corner&2)?1.f:0.f),ray,j);
   if(j[0]*j[3]-j[1]*j[2]<1e-7f)return 0;
  }
 for(int e=0;e<2;e++){
  float x,y;if(!quest_lens_inverse(m,e,1,0,0,&x,&y))return 0;
  /* Keep the wearer-confirmed horizontal centers from the sharpness test. */
  m->center_shift[e]=(e?620.f:820.f)-x*1440.f/32.f;
  fprintf(stderr,"Eye %d stock ray center %.2f,%.2f; alignment shift %.2f px\n",e,x*45.f,(32-y)*50.f,m->center_shift[e]);
 }
 return 1;
}
