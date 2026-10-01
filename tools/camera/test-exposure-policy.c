#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "exposure-policy.h"
int main(void){
 unsigned char pixels[320*240];memset(pixels,80,sizeof(pixels));
 struct quest_exposure s=quest_exposure_next(pixels,12000,16);
 assert(s.us==4000&&s.gain==48); /* Same nominal brightness, less motion blur. */
 memset(pixels,10,sizeof(pixels));s=quest_exposure_next(pixels,4000,48);
 assert(s.us==4000&&s.gain>48&&s.gain<=64);
 memset(pixels,255,sizeof(pixels));s=quest_exposure_next(pixels,4000,48);assert(s.gain==36);
 memset(pixels,80,sizeof(pixels));memset(pixels,255,320*20);
 s=quest_exposure_next(pixels,4000,48);assert(s.brightness==80&&s.gain==48);
 for(unsigned v=0;v<256;v++){
  memset(pixels,v,sizeof(pixels));s=(struct quest_exposure){4000,48,0};
  for(unsigned n=0;n<100;n++){s=quest_exposure_next(pixels,s.us,s.gain);assert(s.us>=1000&&s.us<=8000&&s.gain>=16&&s.gain<=240);}
 }
 unsigned char group[4][320*240];unsigned us[4]={12000,12000,12000,12000},gain[4]={16,16,16,16};struct quest_exposure out[4];
 for(unsigned c=0;c<4;c++)memset(group[c],20+c*40,sizeof(group[c]));
 quest_exposure_group_next(group,us,gain,out);
 for(unsigned c=0;c<4;c++)assert(out[c].us==out[0].us&&out[c].gain>=16&&out[c].gain<=240);
 puts("exposure: motion preference, bounded changes, ROI, bright/dark and sustained bounds PASS");
}
