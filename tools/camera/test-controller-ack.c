#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "controller-ack.h"
#define MS 1000000ULL
static void frames(struct quest_controller_ack *s,unsigned seq,unsigned long long t,int broken_camera){
    for(unsigned c=0;c<4;c++)quest_controller_ack_observe(s,c,seq+c,t+c*1000,38,48,128,
        (int)c==broken_camera?1:2,t+MS);
}
int main(void){
    const unsigned long long base=1000*MS;
    struct quest_controller_ack s;quest_controller_ack_start(&s,base);
    frames(&s,10,base+90*MS,-1);assert(!s.camera[0].streak);
    frames(&s,12,base+110*MS,-1);
    assert(quest_controller_ack_poll(&s,base+111*MS,base)==QUEST_CONTROLLER_ACK_NONE);
    frames(&s,12,base+110*MS,-1); /* Same raw buffers cannot count twice. */
    assert(s.camera[0].streak==1);
    for(unsigned c=0;c<4;c++)quest_controller_ack_observe(&s,c,13+c,base+126*MS+c*1000,3990,16,128,1,base+127*MS);
    assert(s.camera[0].streak==1); /* Ordinary intervening scene is ignored. */
    frames(&s,14,base+143*MS,-1);
    assert(quest_controller_ack_poll(&s,base+144*MS,base)==QUEST_CONTROLLER_ACK_CONFIRMED&&s.confirmed);
    assert(quest_controller_ack_poll(&s,base+145*MS,base)==QUEST_CONTROLLER_ACK_NONE);
    struct quest_controller_ack dropped=s;
    frames(&dropped,18,base+210*MS,-1);
    assert(quest_controller_ack_poll(&dropped,base+211*MS,base)==QUEST_CONTROLLER_ACK_LOST);
    /* The previous controller phase now has correctshorttuple butwrongtag. */
    quest_controller_ack_observe(&s,2,18,base+176*MS,38,48,128,1,base+177*MS);
    assert(quest_controller_ack_poll(&s,base+178*MS,base+170*MS)==QUEST_CONTROLLER_ACK_LOST&&!s.confirmed);
    assert(quest_controller_ack_poll(&s,base+500*MS,base+170*MS)==QUEST_CONTROLLER_ACK_NONE);
    assert(quest_controller_ack_poll(&s,base+670*MS,base+170*MS)==QUEST_CONTROLLER_ACK_RETRY);
    assert(quest_controller_ack_poll(&s,base+670*MS,base+670*MS)==QUEST_CONTROLLER_ACK_NONE);
    quest_controller_ack_sent(&s,base+671*MS);
    frames(&s,30,base+780*MS,-1);frames(&s,34,base+847*MS,-1);
    assert(quest_controller_ack_poll(&s,base+848*MS,base+671*MS)==QUEST_CONTROLLER_ACK_NONE); /* +4 skips phase */
    frames(&s,36,base+880*MS,-1);
    assert(quest_controller_ack_poll(&s,base+881*MS,base+671*MS)==QUEST_CONTROLLER_ACK_CONFIRMED);
    /* Silent stream loss is quarantined even without incoming frame callbacks. */
    assert(quest_controller_ack_poll(&s,base+982*MS,base+900*MS)==QUEST_CONTROLLER_ACK_LOST);
    struct quest_controller_ack before=s;
    assert(quest_controller_ack_poll(&s,base+800*MS,base+900*MS)==QUEST_CONTROLLER_ACK_NONE);
    assert(!memcmp(&before,&s,sizeof(s)));
    /* Flags and applied fields all matter; neither tag nor shortexposure alone. */
    quest_controller_ack_start(&s,base);
    for(unsigned c=0;c<4;c++){
        quest_controller_ack_observe(&s,c,20,base+110*MS,38,48,130,2,base+111*MS);
        quest_controller_ack_observe(&s,c,22,base+143*MS,57,48,128,2,base+144*MS);
        quest_controller_ack_observe(&s,c,24,base+176*MS,38,64,128,2,base+177*MS);
        quest_controller_ack_observe(&s,c,26,base+209*MS,38,48,128,1,base+210*MS);
        assert(!s.camera[c].streak);
    }
    /* Partial hardware application plant: cam2nevergetsclass2 untilretransmit.
     * Othersbeingcorrect mustnotpermitbank0; retrybudgetdoesnotreset. */
    quest_controller_ack_start(&s,base);unsigned retries=0,timeouts=0;
    for(unsigned tick=1;tick<=750;tick++){
        unsigned long long t=base+tick*16666667ULL;
        if(tick%2)frames(&s,tick, t,2);
        else for(unsigned c=0;c<4;c++)quest_controller_ack_observe(&s,c,tick+c,t+c*1000,
            c==2?38:3990,c==2?48:16,128,1,t+MS);
        enum quest_controller_ack_action a=quest_controller_ack_poll(&s,t+2*MS,s.sent_ns);
        retries+=a==QUEST_CONTROLLER_ACK_RETRY;timeouts+=a==QUEST_CONTROLLER_ACK_TIMEOUT;
        assert(a!=QUEST_CONTROLLER_ACK_CONFIRMED&&!s.confirmed);
    }
    assert(retries==19&&timeouts==1&&s.timed_out);
    /* Readback may recover after the bounded command attempts have stopped. */
    frames(&s,800,base+12600*MS,-1);frames(&s,802,base+12633*MS,-1);
    assert(quest_controller_ack_poll(&s,base+12634*MS,s.sent_ns)==QUEST_CONTROLLER_ACK_CONFIRMED&&!s.timed_out);
    /* An unfinished +2 pair cannot borrow evidence morethan100ms old. */
    quest_controller_ack_start(&s,base);
    frames(&s,10,base+110*MS,-1);frames(&s,12,base+220*MS,-1);
    assert(quest_controller_ack_poll(&s,base+221*MS,base)==QUEST_CONTROLLER_ACK_NONE);
    puts("controller ack: identity, alternating scene ignored, wrongtag quarantine, distinctseq+2, freshness, global command spacing, boundedretry and late recovery PASS");
}
