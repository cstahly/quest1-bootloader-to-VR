/* Experimental pose mailbox v2. Atomic rename by writer; read only from a
 * separate polling thread, never from the IMU or OpenXR pose callback.
 * Exactly 160 bytes, little endian integers and IEEE754 binary64 fields.
 */
#ifndef MONTEREY_VIO_IO_H
#define MONTEREY_VIO_IO_H
#include "monterey_vio.h"
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#define MV_WIRE_SIZE 160
static inline uint64_t mv_get64(const unsigned char *b){uint64_t v=0;for(int i=7;i>=0;i--)v=(v<<8)|b[i];return v;}
static inline void mv_put64(unsigned char *b,uint64_t v){for(int i=0;i<8;i++){b[i]=(unsigned char)v;v>>=8;}}
static inline double mv_get_double(const unsigned char *b){uint64_t v=mv_get64(b);double d;memcpy(&d,&v,8);return d;}
static inline void mv_put_double(unsigned char *b,double d){uint64_t v;memcpy(&v,&d,8);mv_put64(b,v);}
static inline void mv_encode(unsigned char b[MV_WIRE_SIZE],const struct mv_packet *p){
 memset(b,0,MV_WIRE_SIZE);memcpy(b,"QVIO002",7);
 mv_put64(b+8,p->epoch);mv_put64(b+16,p->timestamp);mv_put64(b+24,p->published);
 const double v[]={p->position.x,p->position.y,p->position.z,p->orientation.x,p->orientation.y,p->orientation.z,p->orientation.w,p->head_offset.x,p->head_offset.y,p->head_offset.z};
 for(unsigned i=0;i<10;i++)mv_put_double(b+32+8*i,v[i]);
 mv_put64(b+112,p->ready);mv_put64(b+120,p->velocity_valid);
 mv_put_double(b+128,p->velocity.x);mv_put_double(b+136,p->velocity.y);mv_put_double(b+144,p->velocity.z);
}
static inline bool mv_decode(const unsigned char b[MV_WIRE_SIZE],struct mv_packet *p){
 if(memcmp(b,"QVIO002\0",8)||mv_get64(b+112)>1||mv_get64(b+120)>1||mv_get64(b+152))return false;
 struct mv_packet v;memset(&v,0,sizeof(v));v.epoch=mv_get64(b+8);v.timestamp=mv_get64(b+16);v.published=mv_get64(b+24);
 v.position=(struct mv_vec){mv_get_double(b+32),mv_get_double(b+40),mv_get_double(b+48)};
 v.orientation=(struct mv_quat){mv_get_double(b+56),mv_get_double(b+64),mv_get_double(b+72),mv_get_double(b+80)};
 v.head_offset=(struct mv_vec){mv_get_double(b+88),mv_get_double(b+96),mv_get_double(b+104)};
 v.velocity_valid=mv_get64(b+120)!=0;v.velocity=(struct mv_vec){mv_get_double(b+128),mv_get_double(b+136),mv_get_double(b+144)};
 v.ready=(unsigned)mv_get64(b+112);*p=v;return true;
}
static inline bool mv_read_packet(const char *path,struct mv_packet *p){
 int fd=open(path,O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return false;
 struct stat st;unsigned char b[MV_WIRE_SIZE+1];bool ok=false;
 if(fstat(fd,&st)==0&&S_ISREG(st.st_mode)&&st.st_size==MV_WIRE_SIZE){
  ssize_t n=read(fd,b,sizeof(b));if(n==MV_WIRE_SIZE)ok=mv_decode(b,p);
 }
 close(fd);return ok;
}
#endif
