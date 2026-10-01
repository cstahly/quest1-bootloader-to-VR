/* Opt-in stock-bank diagnostic candidate, not default capture policy.
 * Stock type2/tag1 uses normal bank0; type3/tag2 uses controller bank1.
 * Monterey table controller defaults:38 microseconds, Q4 gain48, tag2.
 */
#ifndef QUEST_CAMERA_SCENE_BANK_H
#define QUEST_CAMERA_SCENE_BANK_H
struct quest_scene_bank_setting {unsigned us,gain,tag;};
static inline struct quest_scene_bank_setting quest_scene_bank_requested(unsigned enabled,unsigned bank,unsigned us,unsigned gain){
    if(enabled&&bank==1)return (struct quest_scene_bank_setting){38,48,2};
    return (struct quest_scene_bank_setting){us,gain,1};
}
static inline void quest_scene_bank_packet(unsigned char out[21],unsigned bank,
 const struct quest_scene_bank_setting settings[4]){
    for(unsigned c=0;c<4;c++){
        out[2*c]=(unsigned char)settings[c].us;out[2*c+1]=(unsigned char)(settings[c].us>>8);
        out[8+2*c]=(unsigned char)settings[c].gain;out[9+2*c]=(unsigned char)(settings[c].gain>>8);
        out[16+c]=(unsigned char)settings[c].tag;
    }
    out[20]=(unsigned char)bank;
}
static inline unsigned quest_camera_metadata_class(unsigned char m50,unsigned char m52){
    return (m50&2U)?0U:(m52&15U);
}
static inline int quest_camera_scene_selected(unsigned char m50,unsigned char m52){
    return quest_camera_metadata_class(m50,m52)==1U;
}
/* Reject impossible scene settings; class1 remains mandatory separately.
 * Minimum1000us quantizes to988us; tolerance19 preserves that valid readback. */
static inline int quest_camera_scene_settings_valid(unsigned us,unsigned gain){
    return us>=981U&&us<=12019U&&gain>=16U&&gain<=240U;
}
/* -1 excludes command transitions/old queued frames; 0 mismatch, 1 match.
 * Compare capture time, not dequeue time, so a delayed old buffer cannot count. */
static inline int quest_scene_readback_match(unsigned long long frame_ns,
 unsigned long long command_ns,unsigned actual_us,unsigned actual_gain,
 unsigned requested_us,unsigned requested_gain){
    if(!command_ns||frame_ns<=command_ns||frame_ns-command_ns<=100000000ULL)return -1;
    unsigned delta=actual_us>requested_us?actual_us-requested_us:requested_us-actual_us;
    return delta<=19U&&actual_gain==requested_gain;
}
#endif
