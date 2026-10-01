/* Opt-in scene-bank target acknowledgement. Pure host-testable controller;
 * no sensor commands. Caller writes only on TARGET_CHANGED or RETRY, and calls
 * quest_exposure_ack_sent after completing the write attempt. Default capture
 * policy is unchanged until a caller explicitly uses this helper. */
#ifndef QUEST_EXPOSURE_ACK_H
#define QUEST_EXPOSURE_ACK_H
#include "camera-feed.h"
#define QUEST_EXPOSURE_ACK_SETTLE_NS 100000000ULL
#define QUEST_EXPOSURE_ACK_AGE_NS 100000000ULL
#define QUEST_EXPOSURE_ACK_CADENCE_NS 500000000ULL
#define QUEST_EXPOSURE_ACK_TIMEOUT_NS 10000000000ULL
#define QUEST_EXPOSURE_TARGET_CHANGED 1
#define QUEST_EXPOSURE_TARGET_UNCHANGED 0
#define QUEST_EXPOSURE_TARGET_REFUSED (-1)
enum quest_exposure_ack_action {
    QUEST_EXPOSURE_ACK_NONE, QUEST_EXPOSURE_ACK_RETRY,
    QUEST_EXPOSURE_ACK_EVALUATE, QUEST_EXPOSURE_ACK_TIMEOUT
};
struct quest_exposure_ack {
    unsigned initialized,confirmed,streak,timed_out;
    unsigned desired_us[4],desired_gain[4],observed_frames[4];
    unsigned long long observed_times[4],observed_ns;
    unsigned long long sent_ns,evaluated_ns,pending_since_ns,polled_ns;
};
static inline void quest_exposure_ack_sent(struct quest_exposure_ack *s,unsigned long long now){
    if(!s->initialized||now<s->sent_ns||now<s->polled_ns)return;
    s->sent_ns=now;s->streak=0;s->confirmed=0;s->observed_ns=0;
    /* Neither retries nor their completion timestamps reset the deadline. */
}
static inline int quest_exposure_ack_target(struct quest_exposure_ack *s,
 const unsigned us[4],const unsigned gain[4],unsigned long long now){
    if(!now||(s->initialized&&now<s->polled_ns))return QUEST_EXPOSURE_TARGET_REFUSED;
    unsigned same=s->initialized;
    for(unsigned c=0;c<4;c++){
        if(!us[c]||!gain[c])return QUEST_EXPOSURE_TARGET_REFUSED;
        if(us[c]!=s->desired_us[c]||gain[c]!=s->desired_gain[c])same=0;
    }
    if(same)return QUEST_EXPOSURE_TARGET_UNCHANGED;
    if(s->initialized&&(!s->confirmed||now<s->sent_ns))return QUEST_EXPOSURE_TARGET_REFUSED;
    *s=(struct quest_exposure_ack){0};s->initialized=1;
    for(unsigned c=0;c<4;c++){s->desired_us[c]=us[c];s->desired_gain[c]=gain[c];}
    s->sent_ns=s->evaluated_ns=s->pending_since_ns=s->polled_ns=now;
    return QUEST_EXPOSURE_TARGET_CHANGED;
}
static inline enum quest_exposure_ack_action quest_exposure_ack_poll(struct quest_exposure_ack *s,
 const struct quest_camera_feed *f,unsigned long long now){
    if(!s->initialized||now<s->sent_ns||now<s->evaluated_ns||now<s->polled_ns)return QUEST_EXPOSURE_ACK_NONE;
    s->polled_ns=now;
    if(s->observed_ns&&now>=s->observed_ns&&now-s->observed_ns>QUEST_EXPOSURE_ACK_AGE_NS){
        s->confirmed=0;s->streak=0;
    }
    /* Check relative difference before adding the settling interval. */
    int ready=now-s->sent_ns>QUEST_EXPOSURE_ACK_SETTLE_NS&&
        quest_camera_feed_control_ready(f,now,s->sent_ns+QUEST_EXPOSURE_ACK_SETTLE_NS,QUEST_EXPOSURE_ACK_AGE_NS);
    unsigned matches=ready,accepted=0;
    if(ready){
        unsigned distinct=1,consecutive=s->observed_ns!=0;
        for(unsigned c=0;c<4;c++){
            unsigned actual=f->exposure_us[c],desired=s->desired_us[c];
            if((actual>desired?actual-desired:desired-actual)>19||f->gain_q4[c]!=s->desired_gain[c])matches=0;
            if(s->observed_ns&&(f->timestamp_ns[c]<=s->observed_times[c]||f->frames[c]==s->observed_frames[c]))distinct=0;
            if((unsigned)(f->frames[c]-s->observed_frames[c])!=2)consecutive=0;
        }
        if(distinct){
            accepted=1;
            s->streak=matches?(consecutive&&s->streak?2:1):0;
            s->confirmed=s->streak==2;
            s->observed_ns=0;
            for(unsigned c=0;c<4;c++){
                s->observed_times[c]=f->timestamp_ns[c];s->observed_frames[c]=f->frames[c];
                if(f->timestamp_ns[c]>s->observed_ns)s->observed_ns=f->timestamp_ns[c];
            }
        }
    }
    if(s->confirmed){
        s->pending_since_ns=0;s->timed_out=0;
        if(accepted&&matches&&now-s->evaluated_ns>=QUEST_EXPOSURE_ACK_CADENCE_NS){
            s->evaluated_ns=now;return QUEST_EXPOSURE_ACK_EVALUATE;
        }
        return QUEST_EXPOSURE_ACK_NONE;
    }
    if(!s->pending_since_ns)s->pending_since_ns=now;
    if(now-s->pending_since_ns>=QUEST_EXPOSURE_ACK_TIMEOUT_NS){
        if(!s->timed_out){s->timed_out=1;return QUEST_EXPOSURE_ACK_TIMEOUT;}
        return QUEST_EXPOSURE_ACK_NONE;
    }
    if(now-s->sent_ns>=QUEST_EXPOSURE_ACK_CADENCE_NS){
        quest_exposure_ack_sent(s,now); /* Prevent repeated retry actions per poll. */
        return QUEST_EXPOSURE_ACK_RETRY;
    }
    return QUEST_EXPOSURE_ACK_NONE;
}
#endif
