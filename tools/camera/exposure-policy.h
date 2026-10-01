/* Bounded scene-exposure policy. Header-only for native tests and Bionic probe.
 * Prefer short exposure for head motion; increase gain before exposure duration.
 * Per-camera central percentile avoids fisheye borders and isolated bright lamps.
 */
#ifndef QUEST_EXPOSURE_POLICY_H
#define QUEST_EXPOSURE_POLICY_H
struct quest_exposure {unsigned us,gain,brightness;};
static struct quest_exposure quest_exposure_next(const unsigned char *pixels,unsigned us,unsigned gain){
 unsigned hist[256]={0},n=0;
 for(unsigned y=30;y<210;y+=2)for(unsigned x=40;x<280;x+=2){hist[pixels[y*320+x]]++;n++;}
 unsigned sum=0,level=0;
 for(;level<255;level++){sum+=hist[level];if(sum*10>=n*7)break;}
 if(us<1000||us>12020||gain<16||gain>240)return (struct quest_exposure){4000,48,level};
 double ratio=level?80./level:1.33;
 if(level>=68&&level<=92)ratio=1;
 if(ratio<.75)ratio=.75;if(ratio>1.33)ratio=1.33;
 double product=(double)us*gain*ratio;
 if(product<16000)product=16000;if(product>1920000)product=1920000;
 unsigned next_us=4000,next_gain=(unsigned)(product/4000+.5);
 if(next_gain<16){next_gain=16;next_us=(unsigned)(product/16+.5);}
 if(next_gain>240){next_gain=240;next_us=(unsigned)(product/240+.5);}
 if(next_us<1000)next_us=1000;if(next_us>8000)next_us=8000;
 return (struct quest_exposure){next_us,next_gain,level};
}
static void quest_exposure_group_next(const unsigned char pixels[4][320*240],
 const unsigned us[4],const unsigned gain[4],struct quest_exposure settings[4]){
 unsigned common_us=1000;
 for(unsigned c=0;c<4;c++){
  settings[c]=quest_exposure_next(pixels[c],us[c],gain[c]);
  if(settings[c].us>common_us)common_us=settings[c].us;
 }
 for(unsigned c=0;c<4;c++){
  unsigned next_gain=(settings[c].us*settings[c].gain+common_us/2)/common_us;
  if(next_gain<16)next_gain=16;if(next_gain>240)next_gain=240;
  settings[c].us=common_us;settings[c].gain=next_gain;
 }
}
#endif
