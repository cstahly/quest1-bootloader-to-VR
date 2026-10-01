#include <assert.h>
#include <stdio.h>
#include "vio-quality.h"
int main(void){
 struct mq_state s={0};struct mv_packet p={.timestamp=1000000000,.orientation={0,0,0,1}};
 unsigned positive[]={20,12},coverage[]={6,4};
 for(int i=0;i<45;i++){
  assert(!mq_update(&s,p,positive,coverage,24,8,p.timestamp+50000000));p.timestamp+=66666667;
 }
 assert(mq_update(&s,p,positive,coverage,24,8,p.timestamp+50000000));
 p.timestamp+=66666667;p.position.x=.1;
 assert(mq_update(&s,p,positive,coverage,24,8,p.timestamp+50000000));
 positive[1]=1;p.timestamp+=66666667;
 assert(!mq_update(&s,p,positive,coverage,24,1,p.timestamp+50000000));positive[1]=12;
 p.timestamp+=66666667;p.position.x=.15;
 assert(mq_update(&s,p,positive,coverage,24,8,p.timestamp+50000000));
 positive[1]=1;
 for(int i=0;i<5;i++){p.timestamp+=66666667;assert(!mq_update(&s,p,positive,coverage,24,1,p.timestamp+50000000));}
 positive[1]=12;
 for(int i=0;i<15;i++){p.timestamp+=66666667;assert(!mq_update(&s,p,positive,coverage,24,8,p.timestamp+50000000));}
 p.timestamp+=66666667;assert(mq_update(&s,p,positive,coverage,24,8,p.timestamp+50000000));
 /* A one-frame processing stall stays unready, then recovers without requiring
  * the moving wearer to stop again. Host reception times remain monotonic. */
 struct mq_state ready_state=s;struct mv_packet ready_packet=p;
 p.timestamp+=66666667;p.position.x+=.01;
 assert(!mq_update(&s,p,positive,coverage,24,8,p.timestamp+151000000));
 assert(s.initialized);
 p.timestamp+=66666667;p.position.x+=.01;
 assert(mq_update(&s,p,positive,coverage,24,8,p.timestamp+110000000));
 /* Boundary audit: freshness is inclusive at150ms; stale retention is
  * inclusive at250ms since last supported MEASUREMENT, not receipt time. */
 s=ready_state;p=ready_packet;p.timestamp+=99000000;p.position.x+=.01;
 assert(mq_update(&s,p,positive,coverage,24,8,p.timestamp+150000000));
 assert(s.last_supported==p.timestamp);
 s=ready_state;p=ready_packet;p.timestamp+=99000000;p.position.x+=.01;
 assert(!mq_update(&s,p,positive,coverage,24,8,p.timestamp+151000000));
 assert(s.initialized&&s.last_supported==ready_state.last_supported);
 s=ready_state;p=ready_packet;p.timestamp+=99000000;p.position.x+=.01;
 assert(!mq_update(&s,p,positive,coverage,24,8,p.timestamp+151000001));
 assert(!s.initialized&&s.last_supported==ready_state.last_supported);
 /* Late supported results cannot refresh their own retention anchor. */
 s=ready_state;p=ready_packet;p.timestamp+=66000000;p.position.x+=.01;
 assert(!mq_update(&s,p,positive,coverage,24,8,p.timestamp+151000000));
 assert(s.initialized&&s.last_supported==ready_state.last_supported);
 p.timestamp+=34000000;p.position.x+=.01;
 assert(!mq_update(&s,p,positive,coverage,24,8,p.timestamp+151000000));
 assert(!s.initialized&&s.last_supported==ready_state.last_supported);
 /* A sustained backlog exceeds the wall-time retention ceiling. */
 s=ready_state;p=ready_packet;p.timestamp+=100000000;
 assert(!mq_update(&s,p,positive,coverage,24,8,p.timestamp+151000000));
 assert(!s.initialized);
 p.timestamp+=66666667;p.position.x+=.02;
 assert(!mq_update(&s,p,positive,coverage,24,8,p.timestamp+110000000));
 /* Staleness never hides absent visual support or a positional discontinuity. */
 s=ready_state;p=ready_packet;p.timestamp+=66666667;positive[1]=1;
 assert(!mq_update(&s,p,positive,coverage,24,1,p.timestamp+151000000));
 assert(!s.initialized);positive[1]=12;
 s=ready_state;p=ready_packet;p.timestamp+=66666667;p.position.x+=2;
 assert(!mq_update(&s,p,positive,coverage,24,8,p.timestamp+151000000));
 assert(!s.initialized);
 s=ready_state;p=ready_packet;
 p.timestamp+=66666667;p.position.x=5;assert(!mq_update(&s,p,positive,coverage,24,8,p.timestamp));
 p.timestamp--;assert(!mq_update(&s,p,positive,coverage,24,8,p.timestamp));assert(!s.first);
 p.position.x=0;p.orientation.w=2;assert(!mq_update(&s,p,positive,coverage,24,8,p.timestamp));
 struct mq_state four={0};
 p=(struct mv_packet){.timestamp=1000000000,.orientation={0,0,0,1}};
 unsigned fp[]={20,0,12,4},fc[]={6,0,4,2};
 for(int i=0;i<46;i++){
  bool ready=mq_update_multi(&four,p,fp,fc,4,24,8,p.timestamp+50000000);
  assert(ready==(i==45));p.timestamp+=66666667;
 }
 /* A weak lower-right view is acceptable with other supported views. */
 assert(mq_update_multi(&four,p,fp,fc,4,24,8,p.timestamp+50000000));
 p.timestamp+=66666667;fp[2]=fp[3]=0;
 assert(!mq_update_multi(&four,p,fp,fc,4,24,8,p.timestamp+50000000));
 puts("quality: startup delay, supported motion, sparse-camera loss, quiet reacquisition, bounded late-result retention, stale/jump/reversed-time/quaternion rejection PASS");
}
