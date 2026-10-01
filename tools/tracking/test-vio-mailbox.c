#define _GNU_SOURCE
#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../patches/monado/runtime-src/monterey_vio_io.h"
static void put(const char *path,const unsigned char *b,size_t n){int fd=open(path,O_CREAT|O_TRUNC|O_WRONLY,0600);assert(fd>=0);assert(write(fd,b,n)==(ssize_t)n);assert(close(fd)==0);}
int main(void){
 char dir[]="/tmp/quest-vio-test-XXXXXX";assert(mkdtemp(dir));char path[256],link[256];snprintf(path,sizeof(path),"%s/pose",dir);snprintf(link,sizeof(link),"%s/link",dir);
 struct mv_packet a={.epoch=17,.timestamp=100,.published=101,.position={.1,-.2,.3},.orientation={0,0,0,1},.head_offset={-.03,.01,0},.ready=1,.velocity={.2,-.3,.4},.velocity_valid=true},b={0};
 unsigned char wire[MV_WIRE_SIZE+1];mv_encode(wire,&a);wire[MV_WIRE_SIZE]=0;
 assert(wire[8]==17&&wire[16]==100);put(path,wire,MV_WIRE_SIZE);assert(mv_read_packet(path,&b));assert(b.epoch==a.epoch&&b.timestamp==100&&b.published==101&&b.ready==1);assert(b.position.y==a.position.y&&b.head_offset.x==a.head_offset.x&&b.orientation.w==1);
 assert(b.velocity_valid&&b.velocity.y==-.3);
 assert(symlink(path,link)==0);assert(!mv_read_packet(link,&b));assert(unlink(link)==0);
 put(path,wire,MV_WIRE_SIZE-1);assert(!mv_read_packet(path,&b));put(path,wire,MV_WIRE_SIZE+1);assert(!mv_read_packet(path,&b));
 wire[152]=1;put(path,wire,MV_WIRE_SIZE);assert(!mv_read_packet(path,&b));wire[152]=0;
 wire[112]=2;put(path,wire,MV_WIRE_SIZE);assert(!mv_read_packet(path,&b));wire[112]=1;
 wire[0]='!';put(path,wire,MV_WIRE_SIZE);assert(!mv_read_packet(path,&b));
 assert(unlink(path)==0);assert(mkfifo(path,0600)==0);assert(!mv_read_packet(path,&b));assert(unlink(path)==0);assert(!mv_read_packet(path,&b));assert(!mv_read_packet(dir,&b));assert(rmdir(dir)==0);
 puts("mailbox: round trip, size/version/reserved checks, symlink/FIFO/directory/missing rejection PASS");
}
