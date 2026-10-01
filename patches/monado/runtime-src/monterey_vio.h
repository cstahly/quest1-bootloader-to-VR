/* Experimental positional adapter. No filesystem/sensor access in this helper.
 * Keeps orientation owned by existing fusion. Positions are local and ephemeral;
 * brief loss holds position and preserves alignment; long loss/restart re-anchors.
 */
#ifndef MONTEREY_VIO_H
#define MONTEREY_VIO_H
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
struct mv_vec { double x,y,z; };
struct mv_quat { double x,y,z,w; };
struct mv_packet {
 uint64_t epoch,timestamp,published;
 struct mv_vec position,head_offset,velocity;
 bool velocity_valid;
 struct mv_quat orientation;
 unsigned ready;
};
struct mv_state {
 bool active,anchored;
 uint64_t epoch,last_timestamp;
 struct mv_quat alignment,last_orientation;
 struct mv_vec head_origin,position_origin,position,last_head,last_offset,velocity,prediction_offset,predicted_hold;
 bool can_predict;
};
static inline struct mv_vec mv_add(struct mv_vec a,struct mv_vec b){return (struct mv_vec){a.x+b.x,a.y+b.y,a.z+b.z};}
static inline struct mv_vec mv_sub(struct mv_vec a,struct mv_vec b){return (struct mv_vec){a.x-b.x,a.y-b.y,a.z-b.z};}
static inline double mv_norm(struct mv_vec a){return sqrt(a.x*a.x+a.y*a.y+a.z*a.z);}
static inline struct mv_quat mv_inverse(struct mv_quat q){return (struct mv_quat){-q.x,-q.y,-q.z,q.w};}
static inline struct mv_quat mv_multiply(struct mv_quat a,struct mv_quat b){return (struct mv_quat){
 a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
 a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
static inline struct mv_vec mv_rotate(struct mv_quat q,struct mv_vec p){struct mv_quat r=mv_multiply(mv_multiply(q,(struct mv_quat){p.x,p.y,p.z,0}),mv_inverse(q));return (struct mv_vec){r.x,r.y,r.z};}
static inline bool mv_finite(struct mv_vec p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static inline bool mv_normalize(struct mv_quat *q){
 double n=q->x*q->x+q->y*q->y+q->z*q->z+q->w*q->w;
 if(!isfinite(n)||fabs(n-1)>0.05)return false;
 n=sqrt(n);q->x/=n;q->y/=n;q->z/=n;q->w/=n;return true;
}
/* Endpoint velocity of the rotated lever arm. The offset chord divided by dt
 * is an interval-average velocity and does not cancel instantaneous IMU velocity
 * during rotation about a stationary head. Relative orientation gives world
 * angular velocity (constant-rate approximation), crossed with the current arm.
 */
static inline struct mv_vec mv_lever_velocity(struct mv_quat previous,struct mv_quat current,struct mv_vec offset,double dt){
 struct mv_quat delta=mv_multiply(current,mv_inverse(previous));
 if(delta.w<0)delta=(struct mv_quat){-delta.x,-delta.y,-delta.z,-delta.w};
 double n=sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
 double factor=n>1e-12?2*atan2(n,delta.w)/(n*dt):2/dt;
 struct mv_vec w={delta.x*factor,delta.y*factor,delta.z*factor};
 return (struct mv_vec){w.y*offset.z-w.z*offset.y,w.z*offset.x-w.x*offset.z,w.x*offset.y-w.y*offset.x};
}
/* Prediction coordinates are separate from the raw measurement path so disabling
 * prediction retains its original alignment/reanchor behavior. */
static inline struct mv_vec mv_predict(const struct mv_state *s,uint64_t target){
 struct mv_vec out=mv_add(s->position,s->prediction_offset);
 if(s->can_predict&&target>s->last_timestamp){
  double horizon=fmin((target-s->last_timestamp)*1e-9,.1);
  double speed=mv_norm(s->velocity);
  if(speed>0)horizon=fmin(horizon,.15/speed);
  out=mv_add(out,(struct mv_vec){s->velocity.x*horizon,s->velocity.y*horizon,s->velocity.z*horizon});
 }
 return out;
}
/* Call on the first observed sensor/update loss, including external IMU or
 * mailbox failure. Repeated polling during an outage must not advance HOLD. */
static inline void mv_deactivate(struct mv_state *s,uint64_t now){
 if(s->active)s->predicted_hold=mv_predict(s,now);
 s->active=false;
}
static inline bool mv_update(struct mv_state *s,struct mv_packet p,struct mv_quat local_head,double angular_speed,uint64_t now){
 if(!p.epoch||!p.timestamp||p.timestamp>p.published||p.published>now||now-p.timestamp>200000000ULL||
    !mv_finite(p.position)||!mv_finite(p.head_offset)||mv_norm(p.head_offset)>.5||
    !isfinite(angular_speed)||!mv_normalize(&p.orientation)||!mv_normalize(&local_head)){
  mv_deactivate(s,now);s->anchored=false;return false;
 }
 if(s->epoch==p.epoch&&p.timestamp<s->last_timestamp){mv_deactivate(s,now);s->anchored=false;return false;}
 if(!p.ready){
  mv_deactivate(s,now);
  if(s->epoch!=p.epoch||p.timestamp-s->last_timestamp>250000000ULL)s->anchored=false;
  return false;
 }
 if(s->active&&s->epoch==p.epoch&&p.timestamp==s->last_timestamp)return true;
 struct mv_vec head=mv_add(p.position,mv_rotate(p.orientation,p.head_offset));
 if(!s->anchored||s->epoch!=p.epoch||p.timestamp-s->last_timestamp>250000000ULL){
  if(angular_speed>.15){mv_deactivate(s,now);s->anchored=false;return false;}
  /* Preserve the update-timeline prediction across a long loss/new epoch;
   * raw position remains untouched for the prediction-disabled path. */
  struct mv_vec held=s->active?mv_predict(s,now):s->predicted_hold;
  s->prediction_offset=mv_sub(held,s->position);
  s->alignment=mv_multiply(local_head,mv_inverse(p.orientation));
  s->can_predict=false;s->last_offset=mv_rotate(p.orientation,p.head_offset);s->last_orientation=p.orientation;
  s->head_origin=head;s->position_origin=s->position;s->last_head=head;
  s->epoch=p.epoch;s->last_timestamp=p.timestamp;s->active=true;s->anchored=true;return true;
 }
 double dt=(p.timestamp-s->last_timestamp)*1e-9;
 // Reject discontinuities before estimating render-time motion.
 if(dt<=0||dt>.25||mv_norm(mv_sub(head,s->last_head))>3.0*dt+.02){mv_deactivate(s,now);s->anchored=false;return false;}
 struct mv_vec offset=mv_rotate(p.orientation,p.head_offset);
 s->can_predict=false;
 if(p.velocity_valid&&mv_finite(p.velocity)&&dt<=.1){
  struct mv_vec lever=mv_lever_velocity(s->last_orientation,p.orientation,offset,dt);
  s->velocity=mv_rotate(s->alignment,mv_add(p.velocity,lever));
  s->can_predict=mv_finite(s->velocity)&&mv_norm(s->velocity)<=3.;
 }
 s->last_offset=offset;s->last_orientation=p.orientation;
 s->position=mv_add(s->position_origin,mv_rotate(s->alignment,mv_sub(head,s->head_origin)));
 s->last_head=head;s->last_timestamp=p.timestamp;s->active=true;return true;
}
/* Pure pose query. Explicit loss freezes at first update reception time. With
 * no new input, stale HOLD is the fixed prediction-horizon endpoint (100ms),
 * already reached before the 150ms freshness expiry. Neither historical nor
 * speculative future queries affect HOLD, raw positions, or reanchor state. */
static inline struct mv_vec mv_render_position(const struct mv_state *s,uint64_t target,uint64_t now){
 if(!s->active)return s->predicted_hold;
 if(now<s->last_timestamp)return mv_predict(s,s->last_timestamp);
 if(now-s->last_timestamp>150000000ULL)return mv_predict(s,now);
 return mv_predict(s,target);
}
#endif
