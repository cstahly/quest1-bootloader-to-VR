#include "quest-lens-mesh.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static struct quest_lens_mesh mesh;
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
 float x,y;assert(!quest_lens_inverse(&mesh,0,1,NAN,0,&x,&y));assert(!quest_lens_inverse(&mesh,0,1,100,100,&x,&y));
 printf("%d RGB/eye inverse round trips, max error %.6f pixels; mirror symmetry and bounds passed\n",count,worst);
}
