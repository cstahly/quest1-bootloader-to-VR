/* Optional capture diagnostics only; no camera-feed ABI or exposure decisions. */
#ifndef QUEST_CAMERA_STATISTICS_H
#define QUEST_CAMERA_STATISTICS_H
struct quest_camera_statistics {unsigned p70,clipped,samples;};
static struct quest_camera_statistics quest_camera_statistics(const unsigned char *pixels){
    unsigned histogram[256]={0},samples=0;
    for(unsigned y=30;y<210;y+=2)for(unsigned x=40;x<280;x+=2){
        histogram[pixels[y*320+x]]++;samples++;
    }
    unsigned cumulative=0,level=0;
    for(;level<255;level++){
        cumulative+=histogram[level];if(cumulative*10>=samples*7)break;
    }
    return (struct quest_camera_statistics){level,histogram[255],samples};
}
/* No catch-up bursts after a stall; reset timestamp to the actual emission time. */
static int quest_camera_diagnostics_due(unsigned long long now,unsigned long long *last){
    if(now<*last||now-*last<1000000000ULL)return 0;
    *last=now;return 1;
}
#endif
