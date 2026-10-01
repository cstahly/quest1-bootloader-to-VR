/* Deterministic geometry and failure cases, no hardware required. */
#include <assert.h>
#include <stdio.h>
#include "../../patches/monado/runtime-src/monterey_vio.h"
static void close3(struct mv_vec a,struct mv_vec b){assert(mv_norm(mv_sub(a,b))<1e-9);}
static struct mv_packet packet(uint64_t t){return (struct mv_packet){.epoch=1,.timestamp=t,.published=t+1000000,.orientation={0,0,0,1},.ready=1};}
int main(void){
 const struct mv_quat identity={0,0,0,1};struct mv_state s={0};uint64_t t=1000000000;
 struct mv_packet p=packet(t);assert(mv_update(&s,p,identity,0,t+2000000));
 p.timestamp+=100000000;p.published+=100000000;p.position.x=.1;
 assert(mv_update(&s,p,identity,0,p.published));close3(s.position,(struct mv_vec){.1,0,0});
 assert(!mv_update(&s,p,identity,0,p.timestamp+201000000));close3(s.position,(struct mv_vec){.1,0,0});
 p=packet(t+400000000);p.epoch=2;p.position.x=20;
 assert(!mv_update(&s,p,identity,.2,p.published));
 assert(mv_update(&s,p,identity,0,p.published));close3(s.position,(struct mv_vec){.1,0,0});
 p.timestamp+=100000000;p.published+=100000000;p.position.x=20.1;
 assert(mv_update(&s,p,identity,0,p.published));close3(s.position,(struct mv_vec){.2,0,0});
 p.position.x=NAN;assert(!mv_update(&s,p,identity,0,p.published));close3(s.position,(struct mv_vec){.2,0,0});
 // One unsupported sample holds untracked, then resumes original coordinates
 // while rotating. It must not quietly re-anchor and discard the displacement.
 s=(struct mv_state){0};p=packet(t);assert(mv_update(&s,p,identity,0,p.published));
 p.timestamp+=100000000;p.published+=100000000;p.position.x=.1;p.ready=0;
 assert(!mv_update(&s,p,identity,1,p.published));assert(!s.active&&s.anchored);
 close3(s.position,(struct mv_vec){0,0,0});
 p.timestamp+=100000000;p.published+=100000000;p.position.x=.2;p.ready=1;
 assert(mv_update(&s,p,identity,1,p.published));close3(s.position,(struct mv_vec){.2,0,0});
 p.timestamp+=300000000;p.published+=300000000;p.ready=0;
 assert(!mv_update(&s,p,identity,1,p.published));assert(!s.anchored);
 p.timestamp+=100000000;p.published+=100000000;p.ready=1;
 assert(!mv_update(&s,p,identity,1,p.published));
 assert(mv_update(&s,p,identity,0,p.published));close3(s.position,(struct mv_vec){.2,0,0});
 // VIO world Z up -> local Y up, without changing the orientation provider.
 s=(struct mv_state){0};p=packet(t);double h=sqrt(.5);struct mv_quat align={-h,0,0,h};
 assert(mv_update(&s,p,align,0,p.published));p.timestamp+=100000000;p.published+=100000000;p.position.z=.1;
 assert(mv_update(&s,p,align,0,p.published));close3(s.position,(struct mv_vec){0,.1,0});
 // Rotating about the head origin must not look like translation of that origin.
 s=(struct mv_state){0};p=packet(t);p.head_offset=(struct mv_vec){.1,0,0};p.position=(struct mv_vec){-.1,0,0};
 assert(mv_update(&s,p,identity,0,p.published));p.timestamp+=100000000;p.published+=100000000;
 p.orientation=(struct mv_quat){0,0,h,h};p.position=(struct mv_vec){0,-.1,0};
 assert(mv_update(&s,p,p.orientation,1,p.published));close3(s.position,(struct mv_vec){0,0,0});
 p.timestamp+=100000000;p.published+=100000000;p.position.x=10;
 assert(!mv_update(&s,p,identity,0,p.published));close3(s.position,(struct mv_vec){0,0,0});
 p=packet(t);p.orientation.w=2;assert(!mv_update(&s,p,identity,0,p.published));
 puts("translation, stale hold, epoch restart, quiet anchor, world axes, lever arm, nonfinite/quaternion/jump rejection PASS");
}
