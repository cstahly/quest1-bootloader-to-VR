/* Exact LP64 MSM ION ABI from the project's 4.4.205 kernel. The owner's
 * gralloc CPU-read lock uses INVALIDATE, not ION_IOC_SYNC (device direction).
 * Only use on a verified, currently owned HAL buffer, before any CPU reads.
 */
#ifndef QUEST_CAMERA_CPU_ACCESS_H
#define QUEST_CAMERA_CPU_ACCESS_H
#include "raw-capture.h"
#define QUEST_ION_CUSTOM 0xc0104906UL
#define QUEST_ION_INVALIDATE 0xc0184d01U
struct quest_ion_flush { int handle,fd; void *address; unsigned offset,length; };
struct quest_ion_custom { unsigned command,padding; unsigned long argument; };
_Static_assert(sizeof(struct quest_ion_flush)==24,"MSM ION LP64 flush ABI");
_Static_assert(__builtin_offsetof(struct quest_ion_flush,address)==8,"ION address offset");
_Static_assert(__builtin_offsetof(struct quest_ion_flush,length)==20,"ION length offset");
_Static_assert(sizeof(struct quest_ion_custom)==16,"ION custom ABI");
static inline int quest_camera_cpu_request(const unsigned long *raw,
 struct quest_ion_flush *flush,struct quest_ion_custom *custom){
    unsigned capacity=0;
    if(!quest_raw_buffer_valid(raw,&capacity)||raw[7]>0x7fffffffUL||
       (raw[6]&4095UL)||raw[6]+capacity<raw[6])return 0;
    /* A private ION client imports the DMA fd for this call. The verified
     * kernel drops the imported handle on return; HAL ownership is unchanged. */
    *flush=(struct quest_ion_flush){0,(int)raw[7],(void *)raw[6],0,capacity};
    *custom=(struct quest_ion_custom){QUEST_ION_INVALIDATE,0,(unsigned long)flush};
    return 1;
}
#endif
