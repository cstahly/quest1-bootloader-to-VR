#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "camera-statistics.h"
int main(void){
    static unsigned char pixels[320*240];
    struct quest_camera_statistics s=quest_camera_statistics(pixels);
    assert(s.p70==0&&s.clipped==0&&s.samples==10800);
    memset(pixels,255,sizeof pixels);s=quest_camera_statistics(pixels);
    assert(s.p70==255&&s.clipped==10800&&s.samples==10800);
    memset(pixels,80,sizeof pixels);
    unsigned i=0;
    for(unsigned y=30;y<210;y+=2)for(unsigned x=40;x<280;x+=2)
        if(i++<3240)pixels[y*320+x]=255;
    s=quest_camera_statistics(pixels);assert(s.p70==80&&s.clipped==3240);
    pixels[84*320+40]=255; /* First sample after the 3240 clipped samples. */
    s=quest_camera_statistics(pixels);assert(s.p70==255&&s.clipped==3241);
    memset(pixels,0,sizeof pixels);
    for(unsigned y=30;y<210;y+=2)for(unsigned x=40;x<280;x+=2)pixels[y*320+x]=80;
    pixels[0]=255;pixels[31*320+41]=255;
    s=quest_camera_statistics(pixels);assert(s.p70==80&&s.clipped==0);
    unsigned long long last=0;
    assert(!quest_camera_diagnostics_due(999999999ULL,&last));
    assert(quest_camera_diagnostics_due(1000000000ULL,&last));
    assert(!quest_camera_diagnostics_due(1000000001ULL,&last));
    assert(quest_camera_diagnostics_due(9999999999ULL,&last));
    assert(!quest_camera_diagnostics_due(9999999999ULL,&last));
    assert(!quest_camera_diagnostics_due(9999999998ULL,&last));
    assert(quest_camera_diagnostics_due(10999999999ULL,&last));
    puts("camera statistics: ROI, p70 boundary, clipping counts and bounded cadence PASS");
}
