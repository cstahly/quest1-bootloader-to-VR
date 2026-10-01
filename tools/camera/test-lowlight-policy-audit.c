/* Characterize present policy limits, not camera-hardware acceptance.
 * These assertions intentionally document behavior a future policy may change. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "exposure-policy.h"
int main(void) {
    static unsigned char pixels[4][320*240];
    unsigned us[4]={8000,8000,8000,8000};
    unsigned gain[4]={240,16,16,16};
    struct quest_exposure next[4];
    memset(pixels[0],10,sizeof pixels[0]);
    for(unsigned c=1;c<4;c++)memset(pixels[c],255,sizeof pixels[c]);
    for(unsigned i=0;i<20;i++) {
        quest_exposure_group_next(pixels,us,gain,next);
        for(unsigned c=0;c<4;c++) {
            assert(next[c].us==8000);
            assert(next[c].gain==(c==0?240:16));
            us[c]=next[c].us; gain[c]=next[c].gain;
        }
    }
    puts("characterized: dark camera can pin common shutter while bright cameras clip indefinitely");
    memset(pixels[0],80,sizeof pixels[0]);
    unsigned sample=0;
    for(unsigned y=30;y<210;y+=2)for(unsigned x=40;x<280;x+=2)
        if(sample++%100<29)pixels[0][y*320+x]=255;
    struct quest_exposure s=quest_exposure_next(pixels[0],4000,48);
    assert(s.brightness==80 && s.us==4000 && s.gain==48);
    puts("characterized: 29 percent saturated ROI samples do not change p70 target policy");
    memset(pixels[0],255,sizeof pixels[0]);
    s=quest_exposure_next(pixels[0],1007,16);
    assert(s.us==1000 && s.gain==16);
    /* Only arithmetic, NOT proof of the MCU's choice of rounding. */
    assert((s.us/19)*19==988);
    assert(((s.us+18)/19)*19==1007);
    puts("characterized: policy minimum straddles scene classifier under 19 us quantization; MCU rounding unverified");
}
