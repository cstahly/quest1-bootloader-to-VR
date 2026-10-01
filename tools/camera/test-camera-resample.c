#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "camera-resample.h"
static unsigned char src[640*480],out[320*240];
int main(void){
 memset(src,137,sizeof(src));assert(quest_camera_downsample(src,out,1)==137*320*240);
 for(unsigned i=0;i<sizeof(out);i++)assert(out[i]==137);
 for(unsigned y=0;y<480;y++)for(unsigned x=0;x<640;x++)src[y*640+x]=((x+y)&1)?255:0;
 quest_camera_downsample(src,out,0);assert(out[1000]==0);
 quest_camera_downsample(src,out,1);for(unsigned y=1;y<240;y++)for(unsigned x=1;x<320;x++)assert(out[y*320+x]==128);
 for(unsigned y=0;y<480;y++)for(unsigned x=0;x<640;x++)src[y*640+x]=(x%200);
 quest_camera_downsample(src,out,1);
 for(unsigned y=1;y<240;y++)for(unsigned x=1;x<320;x++)if(2*x%200>0&&2*x%200<199)assert(out[y*320+x]==(2*x%200));
 memset(src,0,sizeof(src));src[200*640+200]=160;quest_camera_downsample(src,out,1);assert(out[100*320+100]==40);
 puts("resample: constant image, alias suppression, unchanged sample centres and impulse weights PASS");
}
