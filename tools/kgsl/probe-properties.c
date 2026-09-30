/* Read-only KGSL capability probe. UAPI layouts from Qualcomm msm_kgsl.h. */
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>
struct device_info { unsigned device_id,chip_id,mmu_enabled; unsigned long gmem_gpubaseaddr; unsigned gpu_id; unsigned long gmem_sizebytes; };
struct property { unsigned type; void *value; size_t sizebytes; };
#define GETPROPERTY _IOWR(0x09,0x02,struct property)
static int get(int fd,unsigned type,void *value,size_t size) {
 struct property p={type,value,size}; int r=ioctl(fd,GETPROPERTY,&p);
 if(r)perror("KGSL GETPROPERTY"); return r;
}
int main(void) {
 int fd=open("/dev/kgsl-3d0",O_RDONLY); if(fd<0){perror("KGSL open");return 1;}
 struct device_info info={0}; uint64_t gmem=0;
 int r=get(fd,1,&info,sizeof(info));
 if(!r)printf("chip_id=%#x gpu_id=%u gmem_bytes=%lu mmu=%u\n",info.chip_id,info.gpu_id,info.gmem_sizebytes,info.mmu_enabled);
 int q=get(fd,0x13,&gmem,sizeof(gmem)); if(!q)printf("UCHE_GMEM_VADDR=%#llx\n",(unsigned long long)gmem);
 close(fd);return r||q;
}
