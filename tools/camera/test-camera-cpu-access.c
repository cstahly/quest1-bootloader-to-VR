#include <assert.h>
#include <stdio.h>
#include "camera-cpu-access.h"
int main(void){
    unsigned long internal[80]={0},wrapper[9]={0},*raw=wrapper+1;
    unsigned char *p=(unsigned char *)internal;
    raw[-1]=(unsigned long)internal;raw[6]=0x100000;raw[7]=15;
    quest_raw_le(p+0x228,15,4);quest_raw_le(p+0x230,raw[6],8);
    quest_raw_le(p+0x238,QUEST_RAW_MAPPED_SIZE,4);
    struct quest_ion_flush f;struct quest_ion_custom c;
    assert(quest_camera_cpu_request(raw,&f,&c));
    assert(f.handle==0&&f.fd==15&&f.address==(void *)0x100000&&f.offset==0);
    assert(f.length==311296&&c.command==0xc0184d01U&&c.padding==0);
    assert(c.argument==(unsigned long)&f&&QUEST_ION_CUSTOM==0xc0104906UL);
    raw[7]=16;assert(!quest_camera_cpu_request(raw,&f,&c));raw[7]=15;
    quest_raw_le(p+0x238,307840,4);assert(!quest_camera_cpu_request(raw,&f,&c));
    quest_raw_le(p+0x238,QUEST_RAW_MAPPED_SIZE,4);
    raw[6]++;quest_raw_le(p+0x230,raw[6],8);assert(!quest_camera_cpu_request(raw,&f,&c));
    raw[6]=0xfffffffffffff000UL;quest_raw_le(p+0x230,raw[6],8);
    assert(!quest_camera_cpu_request(raw,&f,&c));
    raw[6]=0x100000;quest_raw_le(p+0x230,raw[6],8);
    raw[7]=0xffffffffUL;quest_raw_le(p+0x228,raw[7],4);
    assert(!quest_camera_cpu_request(raw,&f,&c));
    puts("CPU access: exact invalidate ABI, fd/length identity, alignment and overflow rejection PASS");
}
