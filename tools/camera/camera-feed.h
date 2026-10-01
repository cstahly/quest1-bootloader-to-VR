/* Local little-endian ABI shared by Bionic capture and native playground.
 * Atomic rename publishes a complete file. Consumers reject stale frames.
 * Timestamps are driver CLOCK_MONOTONIC buffer timestamps, NOT photon latency.
 */
#ifndef QUEST_CAMERA_FEED_H
#define QUEST_CAMERA_FEED_H
#define QUEST_CAMERA_MAGIC 0x5143414dU
#define QUEST_CAMERA_WIDTH 320U
#define QUEST_CAMERA_HEIGHT 240U
#define QUEST_CAMERA_PIXELS (QUEST_CAMERA_WIDTH*QUEST_CAMERA_HEIGHT)
struct quest_camera_feed {
    unsigned magic,version,width,height,cameras,publication;
    unsigned long long timestamp_ns[4];
    unsigned frames[4],exposure_us[4],gain_q4[4],mean[4];
    unsigned long long published_ns;
    unsigned char pixels[4][QUEST_CAMERA_PIXELS];
};
_Static_assert(sizeof(unsigned)==4,"32-bit word required");
_Static_assert(sizeof(struct quest_camera_feed)==128+4*QUEST_CAMERA_PIXELS,"feed ABI size");
/* A publication must contain one near-synchronous cohort, not whichever
 * camera buffers happened to be dequeued when camera zero advanced. */
static inline int quest_camera_feed_synchronized(const struct quest_camera_feed *f){
    unsigned long long low=~0ULL,high=0;
    for(unsigned c=0;c<4;c++){
        if(!f->frames[c]||!f->timestamp_ns[c])return 0;
        if(f->timestamp_ns[c]<low)low=f->timestamp_ns[c];
        if(f->timestamp_ns[c]>high)high=f->timestamp_ns[c];
    }
    return high-low<=1000000ULL;
}
/* Exposure control must not consume a partial, stale, or pre-command quartet.
 * This is a pure predicate: caller supplies its command/control watermark and
 * age bound. Same-bank identity remains the capture selector's responsibility;
 * timestamp coherence alone does not identify scene vs controller frames.
 * No feed ABI fields or publication rules are changed. */
static inline int quest_camera_feed_control_ready(const struct quest_camera_feed *f,
 unsigned long long now,unsigned long long after_ns,unsigned long long max_age_ns){
    if(!after_ns||!max_age_ns||!quest_camera_feed_synchronized(f))return 0;
    for(unsigned c=0;c<4;c++){
        unsigned long long timestamp=f->timestamp_ns[c];
        if(timestamp<=after_ns||timestamp>now||now-timestamp>max_age_ns)return 0;
    }
    return 1;
}
#endif
