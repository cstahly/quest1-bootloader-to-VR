#include <assert.h>
#include <stdio.h>
#include "camera-frame-trace.h"
int main(void){
    struct quest_frame_trace s={0};
    assert(!quest_frame_trace_frame(&s,1));assert(!quest_frame_trace_command(&s,1));
    s.enabled=1;s.start_ns=100;
    assert(!quest_frame_trace_frame(&s,99));
    assert(quest_frame_trace_frame(&s,100));
    assert(quest_frame_trace_frame(&s,100+QUEST_FRAME_TRACE_NS-1));
    assert(!quest_frame_trace_frame(&s,100+QUEST_FRAME_TRACE_NS));
    assert(!quest_frame_trace_command(&s,100+QUEST_FRAME_TRACE_NS));
    s=(struct quest_frame_trace){.enabled=1,.start_ns=100};
    for(unsigned i=0;i<QUEST_FRAME_TRACE_COMMANDS;i++)assert(quest_frame_trace_command(&s,100+i));
    assert(!quest_frame_trace_command(&s,200));
    assert(quest_frame_trace_frame(&s,200)); /* Command cap doesn't hide raw frames. */
    for(unsigned i=1;i<QUEST_FRAME_TRACE_RECORDS;i++)assert(quest_frame_trace_frame(&s,200+i));
    assert(!quest_frame_trace_frame(&s,5000));
    assert(!quest_frame_trace_active(&s,5000));
    assert(s.frames==4096&&s.commands==64);
    puts("frame trace: default off, monotonic bounds, 10-second/4096-frame/64-command caps PASS");
}
