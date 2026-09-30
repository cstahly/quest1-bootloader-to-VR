#include "sharedmem.c"
#include <assert.h>
int main(void) {
 unsigned char ram[2048]={0},input[512],output[512];memset(input,0x5a,sizeof(input));
 struct rmtfs_mem r={.address=0xfca00000,.size=sizeof(ram),.base=ram,.fd=-1,.relative_offsets=true};
 assert(rmtfs_mem_write(&r,0x200,input,512)==512);
 assert(rmtfs_mem_read(&r,0x200,output,512)==512 && !memcmp(input,output,512));
 assert(ram[511]==0 && ram[1024]==0);
 assert(rmtfs_mem_write(&r,1536,input,512)==512);
 assert(rmtfs_mem_write(&r,1537,input,512)==-EINVAL);
 assert(rmtfs_mem_write(&r,0xfca00200,input,512)==-EINVAL);
 assert(rmtfs_mem_write(&r,0,input,-1)==-EINVAL);
 assert(rmtfs_mem_write(&r,~0UL,input,512)==-EINVAL);
 r.relative_offsets=false;
 assert(rmtfs_mem_read(&r,0xfca00200,output,512)==512 && !memcmp(input,output,512));
 assert(rmtfs_mem_read(&r,0x200,output,512)==-EINVAL);
 assert(rmtfs_mem_read(&r,0xfca00601,output,512)==-EINVAL);
 puts("PASS: relative/absolute offsets, both edges, overflow and negative lengths");return 0;
}
