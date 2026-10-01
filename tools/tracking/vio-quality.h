/* Experimental acceptance gate, not a calibrated estimator confidence metric. */
#ifndef QUEST_VIO_QUALITY_H
#define QUEST_VIO_QUALITY_H
#include "../../patches/monado/runtime-src/monterey_vio.h"
#include <string.h>
struct mq_state {
 uint64_t first,stable,previous,last_supported;
 struct mv_vec position;
 struct mv_quat orientation;
 bool initialized;
};
static inline bool mq_update_multi(struct mq_state *s,struct mv_packet p,
 const unsigned *positive,const unsigned *coverage,unsigned cameras,unsigned unique,unsigned shared,uint64_t now){
 if(cameras<2||cameras>4||!p.timestamp||p.timestamp>now||!mv_finite(p.position)||!mv_normalize(&p.orientation)||
    (s->previous&&p.timestamp<=s->previous)){
  memset(s,0,sizeof(*s));return false;
 }
 double dt=s->previous?(p.timestamp-s->previous)*1e-9:0;
 double step=mv_norm(mv_sub(p.position,s->position));
 double dot=fabs(p.orientation.x*s->orientation.x+p.orientation.y*s->orientation.y+
                 p.orientation.z*s->orientation.z+p.orientation.w*s->orientation.w);
 double angle=2*acos(fmin(1,dot));
 unsigned usable=0,strong=0;
 for(unsigned i=0;i<cameras;i++){
  usable+=positive[i]>=3&&coverage[i]>=2;
  strong+=positive[i]>=6&&coverage[i]>=3;
 }
 bool support=cameras>=2&&cameras<=4&&usable>=2&&strong>=1&&unique>=12&&shared>=1;
 bool fresh=now-p.timestamp<=150000000;
 bool continuous=dt>0&&dt<=.2&&step<=3*dt+.02;
 /* One late result does not uninitialize a supported, continuous estimator.
  * It is still NOT ready. Bound this retention by wall time since the last
  * fresh supported measurement; sustained backlog must require reacquisition. */
 bool brief_stale=s->initialized&&!fresh&&support&&continuous&&s->last_supported&&
                  now-s->last_supported<=250000000ULL;
 if(!s->first)s->first=p.timestamp;
 if((!fresh&&!brief_stale)||!continuous||(!support&&(!s->initialized||p.timestamp-s->last_supported>250000000ULL))){s->initialized=false;s->stable=0;}
 /* A brief unsupported frame is never published as ready, but does not erase
  * established initialization. Long losses still require quiet reacquisition. */
 else if(support&&!s->initialized){
  if(step<=.1*dt+.001&&angle<=.15*dt+.001){if(!s->stable)s->stable=p.timestamp;}
  else s->stable=0;
  s->initialized=s->stable&&p.timestamp-s->stable>=1000000000ULL&&p.timestamp-s->first>=3000000000ULL;
 }
 if(support&&fresh&&continuous)s->last_supported=p.timestamp;
 s->previous=p.timestamp;s->position=p.position;s->orientation=p.orientation;
 return s->initialized&&support&&fresh&&continuous;
}
static inline bool mq_update(struct mq_state *s,struct mv_packet p,
 const unsigned positive[2],const unsigned coverage[2],unsigned unique,unsigned shared,uint64_t now){
 return mq_update_multi(s,p,positive,coverage,2,unique,shared,now);
}
#endif
