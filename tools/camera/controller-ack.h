/* Opt-in acknowledgement of the owner's stock bank1 tuple. No sensor IO.
 * Observe every raw frame before scene filtering. Controller identity requires
 * m50 bit1 clear AND metadata class2 AND exact38us/Q4gain48; brightness or global
 * parity never identifies a bank. Per-camera seq+2 covers consecutive sightings.
 * This first candidate uses strict readiness: a +4 gap drops confirmation even
 * if the last valid pair is under100ms old. That can quarantine publication on
 * an isolated dropped frame; no grace policy is silently assumed. A future grace
 * design must age the last FULL two-phase confirmation, not individual matches.
 */
#ifndef QUEST_CONTROLLER_ACK_H
#define QUEST_CONTROLLER_ACK_H
#define QUEST_CONTROLLER_SETTLE_NS 100000000ULL
#define QUEST_CONTROLLER_AGE_NS 100000000ULL
#define QUEST_CONTROLLER_ACK_RETRY_NS 500000000ULL
#define QUEST_CONTROLLER_ACK_TIMEOUT_NS 10000000000ULL
enum quest_controller_ack_action {
    QUEST_CONTROLLER_ACK_NONE,QUEST_CONTROLLER_ACK_RETRY,QUEST_CONTROLLER_ACK_CONFIRMED,
    QUEST_CONTROLLER_ACK_LOST,QUEST_CONTROLLER_ACK_TIMEOUT
};
struct quest_controller_camera {
    unsigned streak,sequence;
    unsigned long long match_ns,raw_ns;
};
struct quest_controller_ack {
    unsigned initialized,confirmed,timed_out;
    unsigned long long sent_ns,settle_after_ns,pending_since_ns,polled_ns;
    struct quest_controller_camera camera[4];
};
static inline void quest_controller_ack_start(struct quest_controller_ack *s,unsigned long long now){
    *s=(struct quest_controller_ack){0};
    if(now){s->initialized=1;s->sent_ns=s->settle_after_ns=s->pending_since_ns=s->polled_ns=now;}
}
static inline void quest_controller_ack_sent(struct quest_controller_ack *s,unsigned long long now){
    if(!s->initialized||now<s->polled_ns||now<s->sent_ns)return;
    s->sent_ns=s->settle_after_ns=now;s->confirmed=0;
    for(unsigned c=0;c<4;c++)s->camera[c]=(struct quest_controller_camera){0};
}
static inline void quest_controller_ack_observe(struct quest_controller_ack *s,unsigned camera,
 unsigned sequence,unsigned long long timestamp,unsigned exposure_us,unsigned gain,
 unsigned metadata50,unsigned metadata52,unsigned long long now){
    if(!s->initialized||camera>=4||!sequence||now<s->polled_ns||timestamp>now||
       now-timestamp>QUEST_CONTROLLER_AGE_NS||timestamp<=s->settle_after_ns||
       timestamp-s->settle_after_ns<=QUEST_CONTROLLER_SETTLE_NS)return;
    struct quest_controller_camera *c=&s->camera[camera];
    if(timestamp<=c->raw_ns)return;
    c->raw_ns=timestamp;
    unsigned kind=metadata52&15U;
    int matches=!(metadata50&2U)&&kind==2&&exposure_us==38&&gain==48;
    unsigned delta=(unsigned)(sequence-c->sequence);
    if(matches){
        if(c->match_ns&&!delta)return;
        int consecutive=c->match_ns&&timestamp>c->match_ns&&
            timestamp-c->match_ns<=QUEST_CONTROLLER_AGE_NS&&delta==2;
        c->streak=consecutive&&c->streak?2:1;
        c->sequence=sequence;c->match_ns=timestamp;
    }else if(kind==2||(c->match_ns&&delta&&!(delta&1U))){
        /* Ignore the ordinary intervening scene phase. A bad controller tuple
         * or wrong class on the established controller phase breaks support. */
        c->streak=0;
    }
}
static inline enum quest_controller_ack_action quest_controller_ack_poll(struct quest_controller_ack *s,
 unsigned long long now,unsigned long long last_any_bank_command_ns){
    if(!s->initialized||now<s->polled_ns||now<s->sent_ns||now<last_any_bank_command_ns)return QUEST_CONTROLLER_ACK_NONE;
    s->polled_ns=now;
    unsigned all=1;
    for(unsigned c=0;c<4;c++){
        struct quest_controller_camera *p=&s->camera[c];
        if(!p->match_ns||p->match_ns>now||now-p->match_ns>QUEST_CONTROLLER_AGE_NS)p->streak=0;
        if(p->streak<2)all=0;
    }
    if(all){
        s->pending_since_ns=0;s->timed_out=0;
        if(!s->confirmed){s->confirmed=1;return QUEST_CONTROLLER_ACK_CONFIRMED;}
        return QUEST_CONTROLLER_ACK_NONE;
    }
    if(s->confirmed){
        s->confirmed=0;s->pending_since_ns=s->settle_after_ns=now;s->timed_out=0;
        for(unsigned c=0;c<4;c++)s->camera[c]=(struct quest_controller_camera){0};
        return QUEST_CONTROLLER_ACK_LOST;
    }
    if(!s->pending_since_ns)s->pending_since_ns=now;
    if(now-s->pending_since_ns>=QUEST_CONTROLLER_ACK_TIMEOUT_NS){
        if(!s->timed_out){s->timed_out=1;return QUEST_CONTROLLER_ACK_TIMEOUT;}
        return QUEST_CONTROLLER_ACK_NONE;
    }
    if(now-s->sent_ns>=QUEST_CONTROLLER_ACK_RETRY_NS&&now-last_any_bank_command_ns>=QUEST_CONTROLLER_ACK_RETRY_NS){
        quest_controller_ack_sent(s,now); /* Reserve this retry; caller notes completion. */
        return QUEST_CONTROLLER_ACK_RETRY;
    }
    return QUEST_CONTROLLER_ACK_NONE;
}
#endif
