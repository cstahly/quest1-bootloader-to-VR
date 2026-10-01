/* Offline regression probes for lever geometry and sensor-timeline HOLD.
 * No sensor/device access. Synthetic motion does not establish wearer quality. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../patches/monado/runtime-src/monterey_vio.h"

static struct mv_vec scale(struct mv_vec v,double k){return (struct mv_vec){v.x*k,v.y*k,v.z*k};}
static struct mv_vec cross(struct mv_vec a,struct mv_vec b){return (struct mv_vec){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}

static unsigned rotation_cases(void){
 unsigned failures=0,cases=0;double largest=0,old_largest=0;
 const struct mv_quat identity={0,0,0,1};
 const double rates[]={-3.,0.,1.,3.,8.}, arms[]={.03,.1};
 const struct mv_vec axes[]={{1,0,0},{0,1,0},{0,0,1},{.6,.8,0},{1./3,2./3,2./3}};
 for(unsigned a=0;a<2;a++)for(unsigned b=0;b<5;b++)for(unsigned axis=0;axis<5;axis++)
 for(unsigned initial=0;initial<2;initial++)for(unsigned sign=0;sign<2;sign++){
  const uint64_t t=1000000000ULL,interval=33333333ULL,delay=65000000ULL;
  double dt=interval*1e-9,rate=rates[b],angle=rate*dt;
  struct mv_vec unit=axes[axis],offset=scale((struct mv_vec){.6,0,.8},arms[a]);
  struct mv_quat start=initial?(struct mv_quat){.2,-.3,.1,sqrt(.86)}:identity;
  struct mv_vec previous=mv_rotate(start,offset);
  struct mv_packet p={.epoch=1,.timestamp=t,.published=t+delay,.orientation=start,
   .head_offset=offset,.position=scale(previous,-1),.ready=1,.velocity_valid=true};
  struct mv_state s={0};
  if(!mv_update(&s,p,identity,0,p.published))return 1000;
  p.timestamp+=interval;p.published+=interval;
  struct mv_quat delta={unit.x*sin(angle/2),unit.y*sin(angle/2),unit.z*sin(angle/2),cos(angle/2)};
  p.orientation=mv_multiply(delta,start);
  if(sign)p.orientation=(struct mv_quat){-p.orientation.x,-p.orientation.y,-p.orientation.z,-p.orientation.w};
  struct mv_vec rotated=mv_rotate(p.orientation,offset);
  p.position=scale(rotated,-1); /* head = IMU + rotated offset = zero */
  p.velocity=scale(cross(scale(unit,rate),rotated),-1);
  if(!mv_update(&s,p,p.orientation,fabs(rate),p.published))return 1000;
  double raw_error=mv_norm(s.position);
  double predicted_error=mv_norm(mv_render_position(&s,p.published,p.published));
  double old_error=mv_norm(mv_add(p.velocity,scale(mv_sub(rotated,previous),1/dt)))*delay*1e-9;
  if(raw_error>1e-12||predicted_error>1e-10)failures++;
  if(predicted_error>largest)largest=predicted_error;
  if(old_error>old_largest)old_largest=old_error;
  cases++;
 }
 printf("stationary head: %u/%u failed, maximum predicted error %.12fm (old chord derivative %.9fm)\n",
        failures,cases,largest,old_largest);
 return failures;
}

static void close3(struct mv_vec a,struct mv_vec b){assert(mv_norm(mv_sub(a,b))<1e-10);}
static void query_noise(struct mv_state *s,uint64_t now,unsigned order){
 struct mv_state before=*s;
 const uint64_t target[]={0,s->last_timestamp+10000000,now,UINT64_MAX};
 if(order==1)for(unsigned i=0;i<4;i++)(void)mv_render_position(s,target[i],now);
 if(order==2)for(unsigned i=4;i>0;i--)(void)mv_render_position(s,target[i-1],now);
 assert(!memcmp(&before,s,sizeof(*s))); /* Query purity, including no-query path. */
}
static struct mv_state moving_state(void){
 const struct mv_quat q={0,0,0,1};
 struct mv_state s={0};
 struct mv_packet p={.epoch=1,.timestamp=2000000000ULL,.published=2020000000ULL,
  .ready=1,.velocity_valid=true,.velocity={1,0,0},.orientation=q};
 assert(mv_update(&s,p,q,0,p.published));
 p.timestamp+=33000000;p.published+=33000000;p.position.x=.033;
 assert(mv_update(&s,p,q,0,p.published));
 return s;
}
static void query_order_cases(void){
 const struct mv_quat q={0,0,0,1};
 struct mv_state seed=moving_state();
 for(unsigned order=0;order<3;order++){
  struct mv_state s=seed;
  query_noise(&s,s.last_timestamp+65000000,order);
  uint64_t loss_time=s.last_timestamp+80000000;
  struct mv_packet loss={.epoch=1,.timestamp=s.last_timestamp+33000000,
   .published=loss_time,.orientation=q,.ready=0};
  assert(!mv_update(&s,loss,q,0,loss_time));
  close3(s.position,(struct mv_vec){.033,0,0}); /* Raw baseline remains raw. */
  close3(mv_render_position(&s,0,loss_time),(struct mv_vec){.113,0,0});
  query_noise(&s,loss_time+1000000000,order);
  assert(!mv_update(&s,loss,q,0,loss_time+1000000000));
  close3(mv_render_position(&s,UINT64_MAX,loss_time+1000000000),(struct mv_vec){.113,0,0});
  /* New epoch at unrelated coordinates preserves predicted HOLD but raw anchor. */
  struct mv_packet restart={.epoch=2,.timestamp=loss_time+2000000000,
   .published=loss_time+2020000000,.orientation=q,.ready=1,.position={50,0,0},
   .velocity_valid=true};
  assert(mv_update(&s,restart,q,0,restart.published));
  close3(s.position,(struct mv_vec){.033,0,0});
  close3(mv_render_position(&s,restart.published,restart.published),(struct mv_vec){.113,0,0});
  restart.timestamp+=33000000;restart.published+=33000000;restart.position.x+=.02;
  assert(mv_update(&s,restart,q,0,restart.published));
  close3(s.position,(struct mv_vec){.053,0,0});
  close3(mv_render_position(&s,restart.published,restart.published),(struct mv_vec){.133,0,0});
 }
 /* Silent stale expiry is continuous with the already capped normal now-query.
  * Historic/future query targets cannot change that fixed endpoint. */
 for(unsigned order=0;order<3;order++){
  struct mv_state s=seed;
  query_noise(&s,s.last_timestamp+65000000,order);
  close3(mv_render_position(&s,s.last_timestamp+149000000,s.last_timestamp+149000000),(struct mv_vec){.133,0,0});
  close3(mv_render_position(&s,0,s.last_timestamp+151000000),(struct mv_vec){.133,0,0});
  close3(mv_render_position(&s,UINT64_MAX,s.last_timestamp+2000000000),(struct mv_vec){.133,0,0});
  /* External IMU/mailbox loss must use this helper, not assign active=false. */
  mv_deactivate(&s,s.last_timestamp+180000000);
  mv_deactivate(&s,s.last_timestamp+2000000000);
  close3(mv_render_position(&s,0,s.last_timestamp+3000000000),(struct mv_vec){.133,0,0});
 }
 /* Raw/current-time prediction remains bounded at high speed. */
 struct mv_state s=seed;s.velocity=(struct mv_vec){3,0,0};
 mv_deactivate(&s,s.last_timestamp+90000000);
 close3(mv_render_position(&s,0,s.last_timestamp+90000000),(struct mv_vec){.183,0,0});
 /* Same-epoch long gap reanchors to the capped stale endpoint, even if no
  * render query happened during the gap. Moving startup cannot change HOLD. */
 s=seed;
 struct mv_packet late={.epoch=1,.timestamp=s.last_timestamp+400000000,
  .published=s.last_timestamp+420000000,.orientation=q,.ready=1,.position={10,0,0}};
 assert(!mv_update(&s,late,q,1,late.published));
 close3(mv_render_position(&s,0,late.published),(struct mv_vec){.133,0,0});
 assert(mv_update(&s,late,q,0,late.published));
 close3(mv_render_position(&s,late.published,late.published),(struct mv_vec){.133,0,0});
 close3(s.position,(struct mv_vec){.033,0,0});
 /* Short recovery preserves original coordinates, exposing actual estimator
  * correction; it does not covertly reanchor to hide the difference. */
 s=seed;mv_deactivate(&s,s.last_timestamp+50000000);
 struct mv_packet recovery={.epoch=1,.timestamp=s.last_timestamp+33000000,
  .published=s.last_timestamp+60000000,.orientation=q,.ready=1,.position={.066,0,0},
  .velocity_valid=true};
 assert(mv_update(&s,recovery,q,0,recovery.published));
 close3(mv_render_position(&s,recovery.published,recovery.published),(struct mv_vec){.066,0,0});
 puts("query purity/order, explicit/stale HOLD, repeated loss, reanchor, raw baseline, bounds and short recovery PASS");
}
int main(void){
 unsigned failures=rotation_cases();
 query_order_cases();
 return failures?1:0;
}
