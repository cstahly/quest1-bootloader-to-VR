/* Render samples at72Hz from irregular30Hz measurements. No device access. */
#include <assert.h>
#include <stdio.h>
#include "../../patches/monado/runtime-src/monterey_vio.h"
static void close3(struct mv_vec a,struct mv_vec b){assert(mv_norm(mv_sub(a,b))<1e-8);}
int main(void){
 struct mv_state s={0};struct mv_quat q={0,0,0,1};uint64_t origin=1000000000ULL;
 struct mv_packet p={.epoch=1,.timestamp=origin,.published=origin+20000000,.orientation={0,0,0,1},.ready=1,.velocity={1,0,0},.velocity_valid=true};
 assert(mv_update(&s,p,q,0,p.published));
 for(unsigned frame=1;frame<30;frame++){
  p.timestamp=origin+frame*33333333ULL;p.published=p.timestamp+20000000;p.position.x=(p.timestamp-origin)*1e-9;
  assert(mv_update(&s,p,q,0,p.published));
  for(unsigned tick=0;tick<3;tick++){
   uint64_t target=p.published+tick*10000000ULL;
   close3(mv_render_position(&s,target,target),(struct mv_vec){(target-origin)*1e-9,0,0});
  }
 }
 p.timestamp+=33333333;p.published=p.timestamp+20000000;p.ready=0;
 /* HOLD freezes the first loss-update reception-time projection. */
 struct mv_vec last=mv_predict(&s,p.published);
 assert(!mv_update(&s,p,q,1,p.published));
 close3(mv_render_position(&s,p.published+30000000,p.published+30000000),last);
 close3(mv_render_position(&s,p.published+500000000,p.published+500000000),last);
 p.epoch=2;p.ready=1;p.timestamp+=1000000000;p.published=p.timestamp+20000000;p.position.x=50;
 assert(mv_update(&s,p,q,0,p.published));close3(mv_render_position(&s,p.published,p.published),last);
 p.timestamp+=33333333;p.published=p.timestamp+20000000;p.velocity=(struct mv_vec){0,0,0};
 assert(mv_update(&s,p,q,0,p.published));close3(mv_render_position(&s,p.published+30000000,p.published+30000000),last);
 p.timestamp+=33333333;p.published=p.timestamp+20000000;p.velocity.x=NAN;
 assert(mv_update(&s,p,q,0,p.published));assert(!s.can_predict);
 close3(mv_render_position(&s,p.published,p.published),last);
 s=(struct mv_state){.active=true,.can_predict=true,.last_timestamp=origin,.velocity={3,0,0}};
 close3(mv_render_position(&s,s.last_timestamp+120000000,s.last_timestamp+50000000),(struct mv_vec){.15,0,0});
 close3(mv_render_position(&s,s.last_timestamp+300000000,s.last_timestamp+300000000),(struct mv_vec){.15,0,0});
 puts("prediction: continuous motion, sensor-timeline loss/stale hold, restart, stop, invalid velocity and horizon/distance bounds PASS");
}
