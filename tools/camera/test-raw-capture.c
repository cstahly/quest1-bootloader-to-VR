/* Synthetic buffers only. Emit a fixture for the independent Python parser. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raw-capture.h"
int main(int argc,char **argv){
    assert(argc==2);
    struct quest_raw_capture *capture=calloc(1,sizeof(*capture));assert(capture);
    capture->requested_ns=1000000000ULL;
    struct quest_camera_feed feed={0};
    unsigned char *buffer=malloc(QUEST_RAW_FRAME);assert(buffer);
    /* Synthetic owner-HAL wrapper. The public low32 word contains fd15, not
     * length; capacity is the 4096-rounded private mapping size311296. */
    unsigned long internal_words[0x248/8]={0},wrapper[10]={0};
    unsigned char *internal=(unsigned char *)internal_words;
    wrapper[0]=(unsigned long)internal;wrapper[7]=(unsigned long)buffer;wrapper[8]=15;
    quest_raw_le(internal+0x228,15,4);quest_raw_le(internal+0x230,(unsigned long)buffer,8);
    quest_raw_le(internal+0x238,QUEST_RAW_MAPPED_SIZE,4);
    unsigned capacity=0;
    assert(quest_raw_buffer_valid(wrapper+1,&capacity)&&capacity==311296);
    quest_raw_le(internal+0x238,QUEST_RAW_FRAME-1,4);
    assert(!quest_raw_buffer_valid(wrapper+1,&capacity));
    quest_raw_le(internal+0x238,QUEST_RAW_FRAME,4);
    assert(!quest_raw_buffer_valid(wrapper+1,&capacity));
    quest_raw_le(internal+0x238,QUEST_RAW_MAPPED_SIZE,4);
    wrapper[8]=16;assert(!quest_raw_buffer_valid(wrapper+1,&capacity));wrapper[8]=15;
    wrapper[7]++;assert(!quest_raw_buffer_valid(wrapper+1,&capacity));wrapper[7]--;
    wrapper[8]|=1ULL<<32;assert(!quest_raw_buffer_valid(wrapper+1,&capacity));wrapper[8]=15;
    wrapper[0]=0;assert(!quest_raw_buffer_valid(wrapper+1,&capacity));wrapper[0]=(unsigned long)internal;
    assert(quest_raw_buffer_valid(wrapper+1,&capacity));
    unsigned requests[4]={12000,12000,12000,12000},gains[4]={16,32,48,64};
    unsigned char header[QUEST_RAW_HEADER];
    assert(!quest_raw_header(header,capture,&feed,1100000000ULL,requests,gains,7));
    for(unsigned c=0;c<4;c++){
        memset(buffer,(int)(c+1),QUEST_RAW_FRAME);
        buffer[3]=(unsigned char)gains[c];buffer[6]=2;buffer[7]=100;
        feed.frames[c]=30+c;feed.timestamp_ns[c]=1010000000ULL+c*100000ULL;
        feed.exposure_us[c]=11628;feed.gain_q4[c]=gains[c];
        quest_raw_stage(capture,c,buffer,&feed);
        memset(buffer,255,QUEST_RAW_FRAME); /* Simulate HAL reusing returned buffer. */
        assert(capture->data[c][640]==c+1);
    }
    assert(quest_raw_header(header,capture,&feed,1100000000ULL,requests,gains,7));
    assert(!quest_raw_header(header,capture,&feed,1005000000ULL,requests,gains,7));
    feed.timestamp_ns[3]+=2000000;
    assert(!quest_raw_header(header,capture,&feed,1100000000ULL,requests,gains,7));
    feed.timestamp_ns[3]-=2000000;
    feed.frames[2]++;
    assert(!quest_raw_header(header,capture,&feed,1100000000ULL,requests,gains,7));
    feed.frames[2]--;
    capture->requested_ns=feed.timestamp_ns[0];
    assert(!quest_raw_header(header,capture,&feed,1100000000ULL,requests,gains,7));
    capture->requested_ns=1000000000ULL;
    assert(quest_raw_header(header,capture,&feed,1100000000ULL,requests,gains,7));
    FILE *out=fopen(argv[1],"wb");assert(out);
    assert(fwrite(header,1,sizeof(header),out)==sizeof(header));
    assert(fwrite(capture->data,1,sizeof(capture->data),out)==sizeof(capture->data));
    assert(!fclose(out));free(buffer);free(capture);
    puts("raw capture: owner HAL fd/length/layout, synchronization/freshness/matching cohort/buffer lifetime PASS");
}
