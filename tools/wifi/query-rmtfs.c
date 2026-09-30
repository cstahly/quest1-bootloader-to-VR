/* Read-only RMTFS preflight: open NV shadow, inspect allocation metadata, close.
 * Optional --relative-read reads one sector into shared RAM, never writes NV.
 * No NV contents are returned to the caller. */
#include <libqrtr.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static unsigned get16(const unsigned char *p) { return p[0] | p[1]<<8; }
static unsigned get32(const unsigned char *p) { return get16(p) | get16(p+2)<<16; }
static void put32(unsigned char *p, unsigned x) { for(int i=0;i<4;i++)p[i]=x>>(8*i); }
static int call(int fd,unsigned node,unsigned port,unsigned msg,const unsigned char *tlv,int len,unsigned char *reply) {
 unsigned char req[256]={0,1,0,0,0,0,0}; req[3]=msg;req[5]=len;
 memcpy(req+7,tlv,len);
 if(qrtr_sendto(fd,node,port,req,len+7)<0 || qrtr_poll(fd,3000)<=0)return -1;
 struct sockaddr_qrtr from; socklen_t sl=sizeof(from);
 int n=recvfrom(fd,reply,256,0,(void*)&from,&sl);
 if(n<7 || from.sq_node!=node || from.sq_port!=port || reply[0]!=2 || get16(reply+1)!=1 || get16(reply+3)!=msg || get16(reply+5)!=n-7)return -1;
 int ok=0;
 for(int i=7;i+3<=n;) { unsigned size=get16(reply+i+1); if(i+3+size>n)return -1;
  if(reply[i]==2) { if(size!=4 || get16(reply+i+3) || get16(reply+i+5))return -1;ok=1; }
  i+=3+size;
 }
 return ok?n:-1;
}
static const unsigned char *field(const unsigned char *reply,int n,unsigned tag,int size) {
 for(int i=7;i+3<=n;) {unsigned l=get16(reply+i+1); if(i+3+l>n)return NULL;if(reply[i]==tag && l==size)return reply+i+3;i+=3+l;}return NULL;
}
int main(int argc,char **argv) {
 if(argc!=3 && (argc!=4 || strcmp(argv[3],"--relative-read")))return 2;
 unsigned node=strtoul(argv[1],NULL,0),port=strtoul(argv[2],NULL,0);
 const char *paths[]={"/boot/modem_fs1","/boot/modem_fs2","/boot/modem_fsg","/boot/modem_fsc"};
 int fd=qrtr_open(0); if(fd<0)return 1;
 for(int i=0;i<4;i++) {
  unsigned char tlv[128]={1,0,0},reply[256]; int l=strlen(paths[i]);tlv[1]=l;memcpy(tlv+3,paths[i],l);
  int n=call(fd,node,port,1,tlv,l+3,reply); const unsigned char *v=field(reply,n,0x10,4);
  if(!v){fprintf(stderr,"open failed: %s\n",paths[i]);return 1;}unsigned caller=get32(v);
  unsigned char alloc[14]={1,4,0,0,0,0,0,2,4,0,0,0,0,0};put32(alloc+3,caller);put32(alloc+10,0x200000);
  n=call(fd,node,port,4,alloc,sizeof(alloc),reply);v=field(reply,n,0x10,8);
  if(!v || get32(v)!=0xfca00000 || get32(v+4)) {fprintf(stderr,"unexpected memory allocation\n");return 1;}
  if(argc==4) {
   unsigned char io[31]={1,4,0,0,0,0,0,2,1,0,0,3,13,0,1,0,0,0,0,0,0,0,0,0,0,0,0,4,1,0,0};
   put32(io+3,caller);put32(io+15,1);put32(io+19,0x200);put32(io+23,1);
   if(call(fd,node,port,3,io,sizeof(io),reply)<0) {fprintf(stderr,"relative sector read failed: %s\n",paths[i]);return 1;}
  }
  unsigned char close_req[7]={1,4,0};put32(close_req+3,caller);
  if(call(fd,node,port,2,close_req,sizeof(close_req),reply)<0)return 1;
  printf("PASS: %s open, shared buffer 0xfca00000/2MiB, close\n",paths[i]);
 }
 close(fd);return 0;
}
