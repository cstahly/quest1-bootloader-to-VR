/* Optional metadata-only diagnostic limits. No exposure/frame policy decisions. */
#ifndef QUEST_CAMERA_FRAME_TRACE_H
#define QUEST_CAMERA_FRAME_TRACE_H
#define QUEST_FRAME_TRACE_NS 10000000000ULL
#define QUEST_FRAME_TRACE_RECORDS 4096U
#define QUEST_FRAME_TRACE_COMMANDS 64U
struct quest_frame_trace {
    unsigned enabled,frames,commands,request_id,reported;
    unsigned long long start_ns;
};
static inline int quest_frame_trace_active(const struct quest_frame_trace *s,unsigned long long now){
    return s->enabled&&now>=s->start_ns&&now-s->start_ns<QUEST_FRAME_TRACE_NS&&
           s->frames<QUEST_FRAME_TRACE_RECORDS;
}
static inline int quest_frame_trace_frame(struct quest_frame_trace *s,unsigned long long now){
    if(!quest_frame_trace_active(s,now))return 0;
    s->frames++;return 1;
}
static inline int quest_frame_trace_command(struct quest_frame_trace *s,unsigned long long now){
    if(!quest_frame_trace_active(s,now)||s->commands>=QUEST_FRAME_TRACE_COMMANDS)return 0;
    s->commands++;return 1;
}
#endif
