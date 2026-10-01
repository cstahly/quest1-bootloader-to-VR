/* Optional one-shot full-resolution capture. Separate from camera-feed ABI.
 * Wire integers are little-endian; payload retains 640 metadata + 640x480 mono
 * bytes per physical camera, in physical order 0,1,2,3. No proprietary code. */
#ifndef QUEST_RAW_CAPTURE_H
#define QUEST_RAW_CAPTURE_H
#include "camera-feed.h"
#define QUEST_RAW_HEADER 256U
#define QUEST_RAW_METADATA 640U
#define QUEST_RAW_PIXELS (640U*480U)
#define QUEST_RAW_FRAME (QUEST_RAW_METADATA+QUEST_RAW_PIXELS)
#define QUEST_RAW_SIZE (QUEST_RAW_HEADER+4U*QUEST_RAW_FRAME)
#define QUEST_RAW_MAPPED_SIZE ((QUEST_RAW_FRAME+4095U)&~4095U)
/* Owner HAL SHA256 109b418cec3182c069d20bda7e659666ce9202d731d1253c3302c4d5ff30dd86.
 * Public raw[7] is an fd, NOT a length. Its private wrapper prefix points to
 * internal frame metadata: +0x228 fd, +0x230 buffer, +0x238 mapped allocation
 * length. This verified private ABI is valid only while the HAL frame is owned.
 * Require the known rounded allocation and cross-check fd/pointer before copy.
 */
static inline unsigned long long quest_raw_get_le(const unsigned char *p,unsigned bytes){
    unsigned long long value=0;
    for(unsigned j=0;j<bytes;j++)value|=(unsigned long long)p[j]<<(8*j);
    return value;
}
static inline int quest_raw_buffer_valid(const unsigned long *raw,unsigned *capacity){
    _Static_assert(sizeof(unsigned long)==8,"owner HAL requires LP64 ABI");
    *capacity=0;
    const unsigned char *internal=(const unsigned char *)raw[-1];
    if(!internal||!raw[6]||(raw[-1]&7U)||(raw[7]>>32))return 0;
    *capacity=(unsigned)quest_raw_get_le(internal+0x238,4);
    return *capacity==QUEST_RAW_MAPPED_SIZE&&
        quest_raw_get_le(internal+0x230,8)==raw[6]&&
        quest_raw_get_le(internal+0x228,4)==(unsigned)raw[7];
}
struct quest_raw_capture {
    unsigned long long requested_ns;
    unsigned frames[4];
    unsigned long long timestamps[4];
    unsigned char data[4][QUEST_RAW_FRAME];
};
static inline void quest_raw_stage(struct quest_raw_capture *capture,unsigned camera,
                                  const unsigned char *buffer,const struct quest_camera_feed *feed){
    for(unsigned j=0;j<QUEST_RAW_FRAME;j++)capture->data[camera][j]=buffer[j];
    capture->frames[camera]=feed->frames[camera];
    capture->timestamps[camera]=feed->timestamp_ns[camera];
}
static inline void quest_raw_le(unsigned char *out,unsigned long long value,unsigned bytes){
    for(unsigned j=0;j<bytes;j++){out[j]=(unsigned char)value;value>>=8;}
}
static inline int quest_raw_header(unsigned char out[QUEST_RAW_HEADER],const struct quest_raw_capture *capture,
                                   const struct quest_camera_feed *feed,unsigned long long captured_ns,
                                   const unsigned requested_us[4],const unsigned requested_gain[4],unsigned flags){
    if(!capture->requested_ns||captured_ns<capture->requested_ns||(flags&~7U)||!quest_camera_feed_synchronized(feed))return 0;
    for(unsigned c=0;c<4;c++)if(capture->frames[c]!=feed->frames[c]||
        capture->timestamps[c]!=feed->timestamp_ns[c]||feed->timestamp_ns[c]<=capture->requested_ns||
        feed->timestamp_ns[c]>captured_ns||feed->exposure_us[c]<1000)return 0;
    for(unsigned j=0;j<QUEST_RAW_HEADER;j++)out[j]=0;
    const unsigned char magic[8]={'Q','R','A','W','0','0','1',0};
    for(unsigned j=0;j<8;j++)out[j]=magic[j];
    quest_raw_le(out+8,QUEST_RAW_HEADER,4);quest_raw_le(out+12,4,4);
    quest_raw_le(out+16,640,4);quest_raw_le(out+20,480,4);quest_raw_le(out+24,640,4);
    quest_raw_le(out+28,QUEST_RAW_METADATA,4);quest_raw_le(out+32,QUEST_RAW_FRAME,4);
    quest_raw_le(out+36,flags,4);quest_raw_le(out+40,captured_ns,8);quest_raw_le(out+48,capture->requested_ns,8);
    for(unsigned c=0;c<4;c++){
        unsigned char *record=out+64+c*48;
        quest_raw_le(record,c,4);quest_raw_le(record+4,feed->frames[c],4);
        quest_raw_le(record+8,feed->timestamp_ns[c],8);
        quest_raw_le(record+16,feed->exposure_us[c],4);quest_raw_le(record+20,feed->gain_q4[c],4);
        quest_raw_le(record+24,QUEST_RAW_HEADER+c*QUEST_RAW_FRAME,8);
        quest_raw_le(record+32,QUEST_RAW_FRAME,4);
        quest_raw_le(record+36,requested_us[c],4);quest_raw_le(record+40,requested_gain[c],4);
    }
    return 1;
}
#endif
