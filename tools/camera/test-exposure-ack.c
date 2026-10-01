#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "exposure-ack.h"
#define MS 1000000ULL
static struct quest_camera_feed cohort(unsigned seq,unsigned long long timestamp,unsigned gain){
    struct quest_camera_feed f={0};
    for(unsigned c=0;c<4;c++){
        f.frames[c]=seq+c; /* Camera parities need not agree. */
        f.timestamp_ns[c]=timestamp+c*1000;f.exposure_us[c]=3990;f.gain_q4[c]=gain;
    }
    return f;
}
static struct quest_exposure_ack start(unsigned long long now,unsigned gain){
    struct quest_exposure_ack s={0};unsigned us[4]={4000,4000,4000,4000},g[4]={gain,gain,gain,gain};
    assert(quest_exposure_ack_target(&s,us,g,now)==QUEST_EXPOSURE_TARGET_CHANGED);
    quest_exposure_ack_sent(&s,now);return s;
}
int main(void){
    const unsigned long long base=1000*MS;
    unsigned us[4]={4000,4000,4000,4000},gain[4]={134,134,134,134};
    struct quest_exposure_ack s=start(base,134);
    struct quest_exposure_ack missing=s;
    struct quest_camera_feed empty={0};
    assert(quest_exposure_ack_poll(&missing,&empty,base+500*MS)==QUEST_EXPOSURE_ACK_RETRY);
    assert(!missing.confirmed&&!missing.streak);
    struct quest_camera_feed f=cohort(10,base+99*MS,134);
    assert(quest_exposure_ack_poll(&s,&f,base+110*MS)==QUEST_EXPOSURE_ACK_NONE&&!s.streak);
    f=cohort(12,base+110*MS,134);
    assert(quest_exposure_ack_poll(&s,&f,base+111*MS)==QUEST_EXPOSURE_ACK_NONE&&s.streak==1);
    assert(quest_exposure_ack_poll(&s,&f,base+112*MS)==QUEST_EXPOSURE_ACK_NONE&&s.streak==1);
    f=cohort(16,base+177*MS,134); /* +4 misses other scene phase: cannot confirm. */
    assert(quest_exposure_ack_poll(&s,&f,base+178*MS)==QUEST_EXPOSURE_ACK_NONE&&s.streak==1);
    f=cohort(18,base+210*MS,134);f.gain_q4[3]=101;
    assert(quest_exposure_ack_poll(&s,&f,base+211*MS)==QUEST_EXPOSURE_ACK_NONE&&!s.streak);
    gain[3]=101;assert(quest_exposure_ack_target(&s,us,gain,base+212*MS)==QUEST_EXPOSURE_TARGET_REFUSED);gain[3]=134;
    /* Partial arrivals don't erase the first match, but cannot confirm it. */
    f=cohort(20,base+244*MS,134);quest_exposure_ack_poll(&s,&f,base+245*MS);assert(s.streak==1);
    f.timestamp_ns[0]+=33*MS;f.frames[0]+=2;
    quest_exposure_ack_poll(&s,&f,base+280*MS);assert(s.streak==1&&!s.confirmed);
    f=cohort(22,base+277*MS,134);quest_exposure_ack_poll(&s,&f,base+281*MS);assert(s.confirmed);
    /* Lost camera input expires confirmation and eventually retries same target. */
    quest_exposure_ack_poll(&s,&f,base+390*MS);assert(!s.confirmed&&!s.streak);
    assert(quest_exposure_ack_poll(&s,&f,base+500*MS)==QUEST_EXPOSURE_ACK_RETRY);
    assert(quest_exposure_ack_poll(&s,&f,base+500*MS)==QUEST_EXPOSURE_ACK_NONE);
    quest_exposure_ack_sent(&s,base+501*MS);
    f=cohort(24,base+599*MS,134);quest_exposure_ack_poll(&s,&f,base+610*MS);assert(!s.streak);
    f=cohort(26,base+611*MS,134);quest_exposure_ack_poll(&s,&f,base+612*MS);assert(s.streak==1);
    f=cohort(28,base+644*MS,134);
    assert(quest_exposure_ack_poll(&s,&f,base+645*MS)==QUEST_EXPOSURE_ACK_EVALUATE&&s.confirmed);
    assert(quest_exposure_ack_target(&s,us,gain,base+645*MS)==QUEST_EXPOSURE_TARGET_UNCHANGED);
    assert(quest_exposure_ack_poll(&s,&f,base+646*MS)==QUEST_EXPOSURE_ACK_NONE);
    unsigned evaluated=0;
    for(unsigned i=1;i<=16;i++){
        f=cohort(28+2*i,base+(644+33*i)*MS,134);
        evaluated+=quest_exposure_ack_poll(&s,&f,base+(645+33*i)*MS)==QUEST_EXPOSURE_ACK_EVALUATE;
        if(i==15){
            /* Evaluation is due, but neither an older still-fresh quartet nor
             * repolling the accepted quartet may supply another policy input. */
            struct quest_camera_feed old=cohort(56,base+1106*MS,134);
            assert(quest_exposure_ack_poll(&s,&old,base+1150*MS)==QUEST_EXPOSURE_ACK_NONE);
            assert(quest_exposure_ack_poll(&s,&f,base+1151*MS)==QUEST_EXPOSURE_ACK_NONE);
        }
    }
    assert(evaluated==1); /* No-op doesn't reevaluate on every camera frame. */
    gain[0]=150;
    assert(quest_exposure_ack_target(&s,us,gain,base+1174*MS)==QUEST_EXPOSURE_TARGET_CHANGED&&!s.confirmed);
    assert(s.desired_gain[0]==150&&s.streak==0);
    struct quest_exposure_ack before=s;
    assert(quest_exposure_ack_poll(&s,&f,base)==QUEST_EXPOSURE_ACK_NONE);
    assert(!memcmp(&before,&s,sizeof(s)));
    /* Permanently mismatching target: bounded retries and one timeout event.
     * Readback recovery is still permitted after writes have stopped. */
    s=start(base,134);unsigned retries=0,timeouts=0;
    for(unsigned i=1;i<=25;i++){
        f=cohort(10+2*i,base+i*500*MS-10*MS,101);
        enum quest_exposure_ack_action a=quest_exposure_ack_poll(&s,&f,base+i*500*MS);
        retries+=a==QUEST_EXPOSURE_ACK_RETRY;timeouts+=a==QUEST_EXPOSURE_ACK_TIMEOUT;
        assert(a!=QUEST_EXPOSURE_ACK_EVALUATE);
    }
    assert(retries==19&&timeouts==1&&s.timed_out&&!s.confirmed);
    f=cohort(62,base+12600*MS,134);quest_exposure_ack_poll(&s,&f,base+12601*MS);assert(s.streak==1);
    f=cohort(64,base+12633*MS,134);quest_exposure_ack_poll(&s,&f,base+12634*MS);assert(s.confirmed&&!s.timed_out);
    /* Alternating plant: one phase sticks at101 until same-target retransmit.
     * Naive readback-driven retargeting would chase101/134; helper may not. */
    s=start(base,134);unsigned resent=0,plant_confirmed=0;
    for(unsigned frame=1;frame<=30;frame++){
        unsigned long long t=base+frame*33333333ULL;
        f=cohort(2*frame,t,(frame%2&&!resent)?101:134);
        enum quest_exposure_ack_action a=quest_exposure_ack_poll(&s,&f,t+MS);
        if(a==QUEST_EXPOSURE_ACK_RETRY){resent++;quest_exposure_ack_sent(&s,t+2*MS);}
        if(s.confirmed){plant_confirmed=1;break;}
        for(unsigned c=0;c<4;c++)assert(s.desired_gain[c]==134);
        assert(a!=QUEST_EXPOSURE_ACK_EVALUATE);
    }
    assert(resent==1&&plant_confirmed);
    puts("exposure acknowledgement: settle/freshness, distinct consecutive phases, partial/stale, target hold, retry, cadence/no-op, timeout/recovery and alternating plant PASS");
}
