/* Control-cohort predicate only; no sensor commands, image data or device access. */
#include <assert.h>
#include <stdio.h>
#include "camera-feed.h"
int main(void){
    const unsigned long long request=1000000000ULL,now=request+70000000ULL,age=100000000ULL;
    struct quest_camera_feed f={0};
    assert(!quest_camera_feed_control_ready(&f,now,request,age));
    for(unsigned c=0;c<4;c++){
        f.frames[c]=10;f.timestamp_ns[c]=request+33000000ULL+c*10000ULL;
        if(c<3)assert(!quest_camera_feed_control_ready(&f,now,request,age));
    }
    assert(quest_camera_feed_control_ready(&f,now,request,age));
    /* A camera still on the other 60Hz phase cannot steer a four-camera command. */
    f.timestamp_ns[3]+=16666667ULL;
    assert(!quest_camera_feed_control_ready(&f,now,request,age));
    f.timestamp_ns[3]-=16666667ULL;
    /* Synchronized does not mean fresh: all-old buffers remain in the feed. */
    assert(quest_camera_feed_synchronized(&f));
    assert(!quest_camera_feed_control_ready(&f,now+200000000ULL,request,age));
    /* Driver timestamps in the future must not underflow age subtraction. */
    assert(!quest_camera_feed_control_ready(&f,request+10000000ULL,request,age));
    /* Any camera at/before the command watermark is pre-command input. */
    assert(!quest_camera_feed_control_ready(&f,now,f.timestamp_ns[0],age));
    assert(!quest_camera_feed_control_ready(&f,now,f.timestamp_ns[3],age));
    assert(!quest_camera_feed_control_ready(&f,now,0,age));
    assert(!quest_camera_feed_control_ready(&f,now,request,0));
    /* Coherent new quartet after a subsequent control transaction is allowed. */
    unsigned long long next_request=now;
    for(unsigned c=0;c<4;c++){f.frames[c]+=2;f.timestamp_ns[c]+=66666667ULL;}
    assert(quest_camera_feed_control_ready(&f,now+50000000ULL,next_request,age));
    /* Missing one camera after a restart cannot reuse a zero sequence. */
    f.frames[1]=0;
    assert(!quest_camera_feed_control_ready(&f,now+50000000ULL,next_request,age));
    puts("control cohort: partial, opposite-phase timing, stale, future, pre-command, replay and fresh-after-command PASS");
}
