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
#endif
