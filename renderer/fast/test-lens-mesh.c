#include "quest-lens-mesh.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
static struct quest_lens_mesh mesh;
static double seconds(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
int main(int argc,char**argv){
 assert(argc==2);assert(quest_lens_load(&mesh,argv[1]));float worst=0;int count=0;
 for(int e=0;e<2;e++)for(int c=0;c<3;c++)for(int y=0;y<32;y++)for(int x=0;x<32;x++){
  float ray[2],rx,ry;assert(quest_lens_ray(&mesh,e,c,x+.37f,y+.63f,ray));
  assert(quest_lens_inverse(&mesh,e,c,ray[0],ray[1],&rx,&ry));
  float err=hypotf((rx-x-.37f)*45,(ry-y-.63f)*50);if(err>worst)worst=err;count++;assert(err<.01f);
 }
 for(int c=0;c<3;c++)for(int y=0;y<33;y++)for(int x=0;x<33;x++){
  assert(fabsf(mesh.ray[0][y][x][c][0]+mesh.ray[1][y][32-x][c][0])<5e-5f);
  assert(fabsf(mesh.ray[0][y][x][c][1]-mesh.ray[1][y][32-x][c][1])<5e-5f);
 }
 double begin=seconds();quest_lens_prepare_fast(&mesh);printf("Cache startup %.3fs\n",seconds()-begin);
 float cache_worst=0;
 for(int e=0;e<2;e++)for(int c=0;c<3;c++)for(int y=0;y<32;y++)for(int x=0;x<32;x++){
  float ray[2],rx,ry;assert(quest_lens_ray(&mesh,e,c,x+.37f,y+.63f,ray));
  assert(quest_lens_project_fast(&mesh,e,c,ray[0],ray[1],&rx,&ry));
  float err=hypotf((rx-x-.37f)*45,(ry-y-.63f)*50);if(err>cache_worst)cache_worst=err;assert(err<.25f);
 }
 printf("Cached mapping worst deviation %.4f pixels\n",cache_worst);
 float tx,ty;double exact_start=seconds();
 for(int i=0;i<100000;i++)quest_lens_inverse(&mesh,i%2,i%3,(i%170-85)/100.f,((i/170)%170-85)/100.f,&tx,&ty);
 double exact_time=seconds()-exact_start,fast_start=seconds();
 for(int i=0;i<100000;i++)quest_lens_project_fast(&mesh,i%2,i%3,(i%170-85)/100.f,((i/170)%170-85)/100.f,&tx,&ty);
 printf("100k lookups: exact %.4fs cached %.4fs\n",exact_time,seconds()-fast_start);
 float x,y;assert(!quest_lens_inverse(&mesh,0,1,NAN,0,&x,&y));assert(!quest_lens_inverse(&mesh,0,1,100,100,&x,&y));
 printf("%d RGB/eye inverse round trips, max error %.6f pixels; mirror symmetry and bounds passed\n",count,worst);
}
