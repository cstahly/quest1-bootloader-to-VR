#include <math.h>
#include <assert.h>
#include <stdio.h>
typedef struct {float x,y,z,w;} XrQuaternionf;
typedef struct {float x,y,z;} XrVector3f;
static XrQuaternionf conjugate(XrQuaternionf q) { return (XrQuaternionf){-q.x,-q.y,-q.z,q.w}; }
static XrQuaternionf mul(XrQuaternionf a,XrQuaternionf b) {
 return (XrQuaternionf){a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
static XrVector3f rotate(XrQuaternionf q,XrVector3f p) {
 XrQuaternionf r=mul(mul(q,(XrQuaternionf){p.x,p.y,p.z,0}),conjugate(q));
 return (XrVector3f){r.x,r.y,r.z};
}
/* Recenter heading only: gravity remains world up, regardless of head tilt. */
static int recenter_heading(XrQuaternionf q,XrQuaternionf *origin) {
 XrVector3f f=rotate(q,(XrVector3f){0,0,-1});
 if(f.x*f.x+f.z*f.z<0.04f)return 0; /* Yaw is undefined near vertical. */
 float yaw=atan2f(-f.x,-f.z);
 *origin=(XrQuaternionf){0,sinf(yaw/2),0,cosf(yaw/2)};return 1;
}

int main(void){
 float a=(float)M_PI/4;
 XrQuaternionf yaw={0,sinf(a),0,cosf(a)};
 XrQuaternionf pitch={sinf(a/2),0,0,cosf(a/2)};
 XrQuaternionf q=mul(yaw,pitch),origin={0,0,0,1};
 assert(recenter_heading(q,&origin));
 assert(fabsf(origin.x)<1e-5 && fabsf(origin.z)<1e-5);
 assert(fabsf(origin.y-yaw.y)<1e-5 && fabsf(origin.w-yaw.w)<1e-5);
 XrQuaternionf camera=conjugate(mul(conjugate(origin),q));
 XrVector3f up=rotate(camera,(XrVector3f){0,1,0});
 assert(fabsf(up.x)<1e-5 && fabsf(up.y-cosf(a))<1e-5 && fabsf(up.z+sinf(a))<1e-5);
 XrQuaternionf vertical={sinf(a),0,0,cosf(a)};
 assert(!recenter_heading(vertical,&origin));
 assert(fabsf(origin.y-yaw.y)<1e-5);
 puts("Heading recenter preserves gravity tilt and rejects undefined vertical heading");
}
