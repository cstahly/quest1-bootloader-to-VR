/* Bionic-only diagnostic preload. No stock proprietary code is included.
 * Compile with clang --target=aarch64-linux-android29 -fuse-ld=lld -nostdlib
 * -shared -fPIC -O2 -Wall -Wextra -Werror -LOWNER_BIONIC -lc -ldl.
 * Explicit libc dependency is required for constructor initialization order.
 * Run only in the restricted RAM root.
 * Initial milestone: load the owner's HAL and open/query/close sensors, no stream.
 */
#include "camera-feed.h"
#include "exposure-policy.h"
#include "camera-resample.h"
#include "camera-statistics.h"
#include "camera-frame-trace.h"
#include "camera-bank-diagnostic.h"
#include "camera-scene-bank.h"
#include "exposure-ack.h"
#include "controller-ack.h"
#include "raw-capture.h"
#include "camera-cpu-access.h"
typedef unsigned long size_t;
extern void *dlopen(const char *, int);
extern void *dlsym(void *, const char *);
extern char *dlerror(void);
extern int dprintf(int, const char *, ...);
extern int vdprintf(int, const char *, __builtin_va_list);
extern int *__errno(void);
extern unsigned alarm(unsigned);
extern char *getenv(const char *);
extern long strtol(const char *,char **,int);
extern int usleep(unsigned);
extern int open(const char *,int,...);
extern long read(int,void *,size_t);
extern long write(int,const void *,size_t);
extern int close(int);
extern int ioctl(int,unsigned long,...);
extern int mkstemp(char *);
extern int fchmod(int,unsigned);
extern void *calloc(size_t,size_t);
extern void free(void *);
extern int rename(const char *,const char *);
extern int unlink(const char *);
extern void (*signal(int,void (*)(int)))(int);
struct timespec {long tv_sec,tv_nsec;};
extern int clock_gettime(int,struct timespec *);
static volatile int keep_running=1;
static void interrupted(int sig){(void)sig;keep_running=0;}
static unsigned long long monotonic_ns(void){struct timespec t;clock_gettime(1,&t);return (unsigned long long)t.tv_sec*1000000000+t.tv_nsec;}
static struct quest_camera_feed feed;
static struct quest_frame_trace frame_trace;
static struct quest_bank_diagnostic bank_diagnostic;
static unsigned scene_bank_only,scene_control_failed;
static unsigned requested_tags[2][4];
static struct quest_exposure requested_bank[2][4];
extern int snprintf(char *,size_t,const char *,...);
struct pollfd {int fd;short events,revents;};
extern int poll(struct pollfd *,unsigned long,int);
extern void _exit(int) __attribute__((noreturn));
int __android_log_print(int priority,const char *tag,const char *format,...){
    /* Expected EAGAIN from nonblocking dequeue is too noisy at camera cadence. */
    if(priority==5&&format[0]=='c')return 0;
    __builtin_va_list args;__builtin_va_start(args,format);
    dprintf(2,"HAL[%d] %s: ",priority,tag);int r=vdprintf(2,format,args);
    __builtin_va_end(args);dprintf(2,"\n");return r;
}
static void *symbol(void *lib, const char *name) {
    void *p=dlsym(lib,name);
    if(!p){dprintf(2,"missing %s: %s\n",name,dlerror());_exit(2);}
    return p;
}
/* Commands verified against owner's v50 libsyncboss and sensor-service caller.
 * This path does not initialize the broad stock SyncBoss service or update MCU
 * firmware. Camera init/start/stop take the CAMERA COUNT, not a bit mask.
 */
static int command_fd=-1,control_fd=-1;
static unsigned char command_sequence=200;
static unsigned char response_bytes[256];
static int camera_command(unsigned char type,const unsigned char *payload,unsigned len,int response){
    unsigned char packet[64]={type,response?++command_sequence:0,(unsigned char)len};
    if(len>sizeof(packet)-3)return -1;
    for(unsigned i=0;i<len;i++)packet[i+3]=payload[i];
    if(write(command_fd,packet,len+3)!=(long)len+3)return -1;
    if(!response)return 0;
    unsigned long long deadline=monotonic_ns()+1000000000ULL;
    while(monotonic_ns()<deadline){
        struct pollfd p={control_fd,1,0};
        if(poll(&p,1,100)<=0)continue;
        unsigned char record[512];long n=read(control_fd,record,sizeof(record));
        if(n<6||record[0]!=1||record[2]!=0)continue;
        for(unsigned off=record[1];off+3<=(unsigned)n;){
            unsigned count=record[off+2];
            if(off+3+count>(unsigned)n)break;
            if(record[off+1]==command_sequence&&record[off]==response){
                dprintf(2,"MCU command %u response %u:",type,response);
                for(unsigned j=0;j<count;j++){response_bytes[j]=record[off+3+j];dprintf(2," %02x",record[off+3+j]);}
                dprintf(2,"\n");
                if(response==74&&count==51)return 0;
                if(response==70&&count==1)return record[off+3];
                if(response==125&&count==4){
                    for(unsigned j=0;j<4;j++)if(record[off+3+j])return -1;
                    return 0;
                }
                return -1;
            }
            off+=3+count;
        }
    }
    dprintf(2,"MCU command %u timed out\n",type);return -1;
}
static void request_exposure(const struct quest_exposure settings[4],unsigned bank_mask){
    if(bank_diagnostic.enabled&&!bank_diagnostic.first_request_ns)bank_diagnostic.first_request_ns=monotonic_ns();
    if(frame_trace.enabled)frame_trace.request_id++;
    for(unsigned bank=0;bank<2;bank++){
        if(!(bank_mask&(1U<<bank)))continue;
        unsigned char exposure[21];
        struct quest_scene_bank_setting packet_settings[4];
        for(unsigned i=0;i<4;i++){
            struct quest_exposure next=quest_bank_requested(&bank_diagnostic,bank,settings[i]);
            packet_settings[i]=quest_scene_bank_requested(scene_bank_only,bank,next.us,next.gain);
            requested_bank[bank][i]=(struct quest_exposure){packet_settings[i].us,packet_settings[i].gain,0};
            requested_tags[bank][i]=packet_settings[i].tag;
        }
        quest_scene_bank_packet(exposure,bank,packet_settings);
        unsigned long long before=frame_trace.enabled?monotonic_ns():0;
        int rc=camera_command(54,exposure,sizeof(exposure),0);
        if(scene_bank_only&&rc){
            scene_control_failed=1;
            dprintf(2,"SCENE_BANK_ONLY command54 bank%u write failed rc=%d; capture will stop\n",bank,rc);
        }
        unsigned long long after=frame_trace.enabled?monotonic_ns():0;
        if(quest_frame_trace_command(&frame_trace,after)){
            dprintf(2,"CAMERA_TRACE_CMD request=%u bank=%u before_ns=%llu after_ns=%llu rc=%d us=%u,%u,%u,%u gain=%u,%u,%u,%u tags=%u,%u,%u,%u\n",
                frame_trace.request_id,bank,before,after,rc,
                requested_bank[bank][0].us,requested_bank[bank][1].us,requested_bank[bank][2].us,requested_bank[bank][3].us,
                requested_bank[bank][0].gain,requested_bank[bank][1].gain,requested_bank[bank][2].gain,requested_bank[bank][3].gain,
                requested_tags[bank][0],requested_tags[bank][1],requested_tags[bank][2],requested_tags[bank][3]);
        }
    }
}
static unsigned camera_setting(const char *name,unsigned fallback,unsigned minimum,unsigned maximum){
    const char *value=getenv(name);if(!value)return fallback;
    char *end;long parsed=strtol(value,&end,10);
    if(!*value||*end||parsed<(long)minimum||parsed>(long)maximum){dprintf(2,"invalid %s\n",name);_exit(2);}
    return (unsigned)parsed;
}
/* One atomic private artifact, one attempt per process. Offscreen diagnostics
 * only: the synchronous ~1.23MB write can transiently delay camera dequeue. */
static int raw_write_all(int fd,const unsigned char *data,size_t bytes){
    size_t done=0;
    while(done<bytes){
        long n=write(fd,data+done,bytes-done);
        if(n<0&&*__errno()==4)continue;
        if(n<=0)return -1;
        done+=(size_t)n;
    }
    return 0;
}
static int raw_capture_publish(const struct quest_raw_capture *capture,const struct quest_camera_feed *current,
                               const struct quest_exposure settings[4],unsigned flags){
    unsigned requested_us[4],requested_gain[4];
    for(unsigned c=0;c<4;c++){requested_us[c]=settings[c].us;requested_gain[c]=settings[c].gain;}
    unsigned char header[QUEST_RAW_HEADER];
    if(!quest_raw_header(header,capture,current,monotonic_ns(),requested_us,requested_gain,flags))return -1;
    char temporary[]="/tmp/quest-camera-raw-v1.XXXXXX";
    int fd=mkstemp(temporary);if(fd<0)return -1;
    int failed=fchmod(fd,0600)||raw_write_all(fd,header,sizeof(header));
    for(unsigned c=0;c<4&&!failed;c++)failed=raw_write_all(fd,capture->data[c],QUEST_RAW_FRAME);
    if(close(fd))failed=1;
    if(!failed&&rename(temporary,"/tmp/quest-camera-raw-v1.bin"))failed=1;
    if(failed)unlink(temporary);
    return failed?-1:0;
}
__attribute__((constructor)) static void probe(void) {
    unsigned setting_us=camera_setting("CAMERA_EXPOSURE_US",12000,1000,12000);
    unsigned setting_gain=camera_setting("CAMERA_GAIN_Q4",240,16,240);
    unsigned auto_exposure=camera_setting("CAMERA_AUTO_EXPOSURE",0,0,1);
    unsigned denoise=camera_setting("CAMERA_DENOISE",0,0,1);
    unsigned diagnostics=camera_setting("CAMERA_DIAGNOSTICS",0,0,1);
    unsigned cpu_invalidate=camera_setting("CAMERA_CPU_INVALIDATE",0,0,1);
    int cpu_ion_fd=-1;
    unsigned raw_enabled=camera_setting("CAMERA_RAW_CAPTURE",0,0,1);
    unsigned trace_enabled=camera_setting("CAMERA_FRAME_TRACE",0,0,1);
    unsigned bank_step=camera_setting("CAMERA_DIAGNOSTIC_BANK_STEP",0,0,1);
    scene_bank_only=camera_setting("CAMERA_SCENE_BANK_ONLY",0,0,1);
    unsigned bank0_present=getenv("CAMERA_DIAGNOSTIC_BANK0_US")!=0;
    unsigned bank1_present=getenv("CAMERA_DIAGNOSTIC_BANK1_US")!=0;
    bank_diagnostic.us[0]=camera_setting("CAMERA_DIAGNOSTIC_BANK0_US",0,2000,4000);
    bank_diagnostic.us[1]=camera_setting("CAMERA_DIAGNOSTIC_BANK1_US",0,2000,4000);
    unsigned control=getenv("CAMERA_MCU")&&getenv("CAMERA_EXPOSURE")&&getenv("CAMERA_STREAM")&&getenv("CAMERA_ALL");
    if(!quest_bank_diagnostic_valid(bank0_present,bank1_present,bank_diagnostic.us[0],bank_diagnostic.us[1],trace_enabled,auto_exposure,control)){
        dprintf(2,"BANK_DIAGNOSTIC requires both distinct 2000..4000us values, FRAME_TRACE=1, AUTO_EXPOSURE=0 and camera control; refusing\n");
        _exit(2);
    }
    bank_diagnostic.enabled=bank0_present&&bank1_present;
    if(bank_step&&!bank_diagnostic.enabled){
        dprintf(2,"BANK_DIAGNOSTIC step requires valid distinct-bank diagnostic; refusing\n");_exit(2);
    }
    bank_diagnostic.step_enabled=bank_step;
    if(scene_bank_only&&(bank_diagnostic.enabled||bank_step||raw_enabled||!control)){
        dprintf(2,"SCENE_BANK_ONLY incompatible with bank override/step/raw capture or missing camera control; refusing\n");_exit(2);
    }
    if(scene_bank_only)dprintf(2,"SCENE_BANK_ONLY candidate: bank0 scene/tag1, bank1 fixed38us/Q4gain48/tag2; controller-first readback and class-based selection\n");
    if(bank_diagnostic.enabled)dprintf(2,"BANK_DIAGNOSTIC bank0_us=%u bank1_us=%u gain=16 step=%u; temporary identification only\n",bank_diagnostic.us[0],bank_diagnostic.us[1],bank_diagnostic.step_enabled);
    struct quest_raw_capture *raw_capture=0;
    unsigned long long raw_check_ns=0;
    if(raw_enabled){
        /* Discard stale requests; caller creates a NEW marker after startup. */
        unlink("/tmp/quest-camera-raw-v1.request");
        dprintf(2,"RAW_CAPTURE waiting for new /tmp/quest-camera-raw-v1.request marker\n");
    }
    int live=getenv("CAMERA_LIVE")!=0;
    if(live){signal(2,interrupted);signal(15,interrupted);signal(14,interrupted);}
    alarm(live?0:15);
    int mcu=getenv("CAMERA_MCU")!=0;
    dprintf(2,"probe v3 MCU=%d stream=%s\n",mcu,getenv("CAMERA_STREAM"));
    void *lib=dlopen("libqcameraoculushal.so",2);
    if(!lib){dprintf(2,"dlopen: %s\n",dlerror());_exit(2);}
    void *(*open_hal)(int)=symbol(lib,"qcamera_open");
    void (*close_hal)(void *)=symbol(lib,"qcamera_close");
    int (*num)(void)=symbol(lib,"qcamera_num_sensors");
    void *(*get)(void *,int)=symbol(lib,"qcamera_get_sensor");
    void (*release)(void *)=symbol(lib,"qcamera_release_sensor");
    int (*info)(void *,void *)=symbol(lib,"qcamera_query_sensor_info");
    int (*start)(void *,void *,void *,int,void *,int)=symbol(lib,"qcamera_start_sensor");
    int (*stop)(void *)=symbol(lib,"qcamera_stop_sensor");
    void *(*dequeue)(void *)=symbol(lib,"qcamera_dequeue_nonblocking");
    int (*enqueue)(void *,void *)=symbol(lib,"qcamera_enqueue");
    const unsigned char count=4;
    if(mcu){
        control_fd=open("/dev/syncboss_control0",0x800);
        command_fd=open("/dev/syncboss0",1);
        if(control_fd<0||command_fd<0)_exit(4);
        if(camera_command(40,0,0,70)!=15){camera_command(41,0,0,125);_exit(4);}
        const unsigned char bpp[]={140,8};
        if(camera_command(3,bpp,2,125)||camera_command(46,&count,1,0)){
            camera_command(41,0,0,125);_exit(4);
        }
        usleep(250000);
        const unsigned char parameter=74;camera_command(2,&parameter,1,74);
    }
    dprintf(2,"camera count=%d\n",num());
    void *hal=open_hal(0);
    dprintf(2,"camera open=%p errno=%d\n",hal,*__errno());
    if(!hal){
        if(mcu){camera_command(47,&count,1,125);camera_command(41,0,0,125);}
        _exit(3);
    }
    void *sensors[4]={0};unsigned dimensions[4][64]={{0}};
    int started[4]={0},frames[4]={0};
    int selected=getenv("CAMERA_ALL")?4:1;
    for(int i=0;i<4;i++){
        sensors[i]=get(hal,i);
        dprintf(2,"sensor %d=%p errno=%d\n",i,sensors[i],*__errno());
        if(!sensors[i])continue;
        unsigned *words=dimensions[i];int rc=info(sensors[i],words);
        dprintf(2,"sensor %d info rc=%d words:",i,rc);
        for(int j=0;j<16;j++)dprintf(2," %08x",words[j]);
        dprintf(2,"\n");
        if(i<selected&&rc==0&&getenv("CAMERA_STREAM")&&
           words[0]==640&&words[1]==480&&words[2]==112&&words[3]==8){
            rc=start(sensors[i],words,words+2,4,(void *)0,1);
            dprintf(2,"start sensor%d rc=%d errno=%d\n",i,rc,*__errno());
            started[i]=rc==0;
        }
    }
    int stream_started=0,failed=0;
    if(cpu_invalidate){
        cpu_ion_fd=open("/dev/ion",0x80000); /* private read-only/CLOEXEC client */
        if(cpu_ion_fd<0){dprintf(2,"CPU_INVALIDATE ion open failed errno=%d\n",*__errno());failed=1;goto cleanup;}
        dprintf(2,"CPU_INVALIDATE verified MSM CPU-read operation enabled\n");
    }
    if(live&&!(started[0]&&started[1]&&started[2]&&started[3])){failed=1;goto cleanup;}
    if(mcu&&started[0]){
        if(getenv("CAMERA_EXPOSURE")){
            const unsigned char mode[]={141,1};camera_command(3,mode,2,125);
        }
        if(camera_command(44,&count,1,0)){failed=1;goto cleanup;}
        stream_started=1;
    }
    unsigned last_published=0,exposure_verified=0;
    unsigned bank_seen[4]={0};
    struct quest_exposure_ack exposure_ack={0};
    struct quest_controller_ack controller_ack={0};
    unsigned long long last_any_bank_command=0,quarantined_scene[4]={0},rejected_scene_metadata[4]={0};
    unsigned long long last_exposure_request=0;
    unsigned long long scene_match[4]={0},scene_mismatch[4]={0},scene_transition[4]={0};
    unsigned last_bad_seq[4]={0},last_bad_us[4]={0},last_bad_gain[4]={0};
    unsigned long long last_diagnostic=diagnostics?monotonic_ns():0;
    unsigned long long short_frames[4]={0},published_ok=0;
    unsigned long long retained_scene[4]={0},rejected_class2[4]={0},rejected_class4[4]={0},rejected_other[4]={0},controller_stock_frames[4]={0};
    unsigned last_us[4]={0},last_gain[4]={0};
    unsigned long long cpu_reads[4]={0},metadata_match[4]={0},metadata_mismatch[4]={0};
    struct quest_exposure settings[4];
    for(unsigned i=0;i<4;i++)settings[i]=(struct quest_exposure){setting_us,setting_gain,0};
    if(trace_enabled){
        frame_trace.enabled=1;frame_trace.start_ns=monotonic_ns();
        dprintf(2,"CAMERA_TRACE_START ns=%llu max_ns=%llu max_frames=%u max_commands=%u\n",
            frame_trace.start_ns,QUEST_FRAME_TRACE_NS,QUEST_FRAME_TRACE_RECORDS,QUEST_FRAME_TRACE_COMMANDS);
    }
    if(scene_bank_only)unlink("/tmp/camera-feed"); /* Withdraw stale service artifact during quarantine. */
    feed.magic=QUEST_CAMERA_MAGIC;feed.version=1;feed.width=320;feed.height=240;feed.cameras=4;
    for(unsigned attempt=0;keep_running&&(live||attempt<1500)&&getenv("CAMERA_STREAM");attempt++){
        if(attempt==10&&mcu&&getenv("CAMERA_EXPOSURE")){
            /* Current stock command54: u16 exposure-us[4], Q4 gain[4],
             * tags[4], controller-exposure flag. Sent after stream starts. */
            if(!bank_diagnostic.enabled)dprintf(2,"Exposure request %u us, Q4 gain %u\n",setting_us,setting_gain);
            if(scene_bank_only){
                quest_controller_ack_start(&controller_ack,monotonic_ns());
                request_exposure(settings,2U); /* Restore controller bank first. */
                if(scene_control_failed){failed=1;goto cleanup;}
                last_any_bank_command=monotonic_ns();
                quest_controller_ack_sent(&controller_ack,last_any_bank_command);
            }else{
                request_exposure(settings,3U);
                last_exposure_request=monotonic_ns();
            }
        }
        if(raw_enabled){
            unsigned long long now=monotonic_ns();
            if(now-raw_check_ns>=250000000ULL){
                raw_check_ns=now;
                /* Nonblocking + no-follow: a stray FIFO/symlink cannot stall us. */
                int request=open("/tmp/quest-camera-raw-v1.request",0x20800);
                if(request>=0){
                    close(request);unlink("/tmp/quest-camera-raw-v1.request");
                    raw_enabled=0; /* One request/attempt, including allocation failure. */
                    raw_capture=calloc(1,sizeof(*raw_capture));
                    if(raw_capture){raw_capture->requested_ns=monotonic_ns();dprintf(2,"RAW_CAPTURE armed ns=%llu\n",raw_capture->requested_ns);}
                    else dprintf(2,"RAW_CAPTURE allocation failed; disabled\n");
                }
            }
        }
        if(raw_capture&&monotonic_ns()-raw_capture->requested_ns>5000000000ULL){
            dprintf(2,"RAW_CAPTURE timed out waiting for fresh synchronized cohort; disabled\n");
            free(raw_capture);raw_capture=0;
        }
        for(int i=0;i<4;i++)if(started[i]){
            void *frame=dequeue(sensors[i]);if(!frame)continue;
            unsigned long long dequeue_ns=frame_trace.enabled?monotonic_ns():0;
            unsigned long *raw=frame;
            if(frames[i]<3)dprintf(2,"camera%d frame%d timestamp=%lu.%09lu seq=%lu data=%lx\n",i,frames[i],raw[0],raw[1],raw[3],raw[6]);
            if(cpu_invalidate){
                struct quest_ion_flush flush;struct quest_ion_custom custom;
                if(!quest_camera_cpu_request(raw,&flush,&custom)){
                    dprintf(2,"CPU_INVALIDATE unsupported buffer identity; capture stopped\n");
                    enqueue(sensors[i],frame);failed=1;goto cleanup;
                }
                int result;
                do{result=ioctl(cpu_ion_fd,QUEST_ION_CUSTOM,&custom);}while(result<0&&*__errno()==4);
                if(result){
                    dprintf(2,"CPU_INVALIDATE failed cam=%d fd=%d errno=%d; capture stopped\n",i,flush.fd,*__errno());
                    enqueue(sensors[i],frame);failed=1;goto cleanup;
                }
                cpu_reads[i]++;
            }
            const unsigned char *metadata=(const unsigned char *)raw[6];
            if(cpu_invalidate&&diagnostics){
                if(metadata[0x59]==(raw[3]&255U))metadata_match[i]++;
                else metadata_mismatch[i]++;
            }
            if(scene_bank_only||quest_frame_trace_active(&frame_trace,dequeue_ns)){
                unsigned capacity=0;
                if(!quest_raw_buffer_valid(raw,&capacity)){
                    dprintf(2,"CAMERA_TRACE unsupported HAL buffer layout/capacity %u; disabled\n",capacity);
                    frame_trace.enabled=0;
                    if(scene_bank_only){
                        dprintf(2,"SCENE_BANK_ONLY requires verified metadata buffer; stopping capture\n");
                        enqueue(sensors[i],frame);failed=1;goto cleanup;
                    }
                }else if(quest_frame_trace_frame(&frame_trace,dequeue_ns)){
                    /* Raw bytes keep uncertain tag/phase semantics explicit.
                     * Trace precedes short-frame filtering and publication decimation. */
                    dprintf(2,"CAMERA_TRACE_FRAME cam=%d seq=%lu driver_ns=%llu host_ns=%llu request=%u bank0_us=%u bank0_gain=%u bank1_us=%u bank1_gain=%u bank0_tag=%u bank1_tag=%u m03=%u m06=%u m07=%u m50=%u m52=%u m59=%u\n",
                        i,raw[3],raw[0]*1000000000ULL+raw[1],dequeue_ns,frame_trace.request_id,
                        requested_bank[0][i].us,requested_bank[0][i].gain,requested_bank[1][i].us,requested_bank[1][i].gain,requested_tags[0][i],requested_tags[1][i],metadata[3],metadata[6],metadata[7],
                        metadata[0x50],metadata[0x52],metadata[0x59]);
                }
            }
            /* Keep the long-exposure scene frames. The MCU interleaves short
             * controller frames; never let these produce a flickering preview. */
            unsigned exposure_us=((unsigned)metadata[6]*256+metadata[7])*19;
            if(scene_bank_only)
                quest_controller_ack_observe(&controller_ack,(unsigned)i,(unsigned)raw[3],
                    raw[0]*1000000000ULL+raw[1],exposure_us,metadata[3],metadata[0x50],metadata[0x52],monotonic_ns());
            if(diagnostics){last_us[i]=exposure_us;last_gain[i]=metadata[3];}
            if(scene_bank_only){
                unsigned frame_class=quest_camera_metadata_class(metadata[0x50],metadata[0x52]);
                if(!quest_camera_scene_selected(metadata[0x50],metadata[0x52])){
                    if(frame_class==2){
                        rejected_class2[i]++;
                        if(exposure_us==38&&metadata[3]==48)controller_stock_frames[i]++;
                    }
                    else if(frame_class==4)rejected_class4[i]++;
                    else rejected_other[i]++;
                    if(diagnostics&&exposure_us<1000)short_frames[i]++;
                    frames[i]++;enqueue(sensors[i],frame);continue;
                }
                if(!quest_camera_scene_settings_valid(exposure_us,metadata[3])){
                    rejected_scene_metadata[i]++;frames[i]++;enqueue(sensors[i],frame);continue;
                }
                if(!controller_ack.confirmed){
                    quarantined_scene[i]++;frames[i]++;enqueue(sensors[i],frame);continue;
                }
                retained_scene[i]++;
                if(diagnostics){
                    int match=quest_scene_readback_match(raw[0]*1000000000ULL+raw[1],
                        last_exposure_request,exposure_us,metadata[3],requested_bank[0][i].us,requested_bank[0][i].gain);
                    if(match<0)scene_transition[i]++;
                    else if(match)scene_match[i]++;
                    else{
                        scene_mismatch[i]++;last_bad_seq[i]=(unsigned)raw[3];
                        last_bad_us[i]=exposure_us;last_bad_gain[i]=metadata[3];
                    }
                }
            }else if(exposure_us<1000){
                if(diagnostics)short_frames[i]++;
                frames[i]++;enqueue(sensors[i],frame);continue;
            }
            const unsigned char *source=metadata+640;
            unsigned total=quest_camera_downsample(source,feed.pixels[i],denoise);
            feed.mean[i]=total/QUEST_CAMERA_PIXELS;
            feed.frames[i]=(unsigned)raw[3];feed.timestamp_ns[i]=raw[0]*1000000000ULL+raw[1];
            feed.exposure_us[i]=((unsigned)metadata[6]*256+metadata[7])*19;feed.gain_q4[i]=metadata[3];
            if(bank_diagnostic.enabled)bank_seen[i]|=quest_bank_observed(&bank_diagnostic,feed.exposure_us[i],feed.gain_q4[i]);
            /* Copy only retained scene buffers, while HAL ownership is ours.
             * Nothing is copied/allocated here with CAMERA_RAW_CAPTURE unset. */
            if(raw_capture){
                unsigned capacity=0;
                if(!quest_raw_buffer_valid(raw,&capacity)){
                    dprintf(2,"RAW_CAPTURE unsupported HAL buffer layout/capacity %u; disabled\n",capacity);
                    free(raw_capture);raw_capture=0;
                }else quest_raw_stage(raw_capture,(unsigned)i,metadata,&feed);
            }
            if(frames[i]==30){
                /* Verified buffer length307840; first640 bytes are metadata. */
                const unsigned char *pixels=(const unsigned char *)raw[6]+640;
                dprintf(2,"camera%d metadata:",i);for(int j=0;j<16;j++)dprintf(2," %02x",pixels[j-640]);dprintf(2,"\n");
                unsigned low=255,high=0;unsigned long total=0;
                for(unsigned j=0;j<640*480;j++){unsigned v=pixels[j];if(v<low)low=v;if(v>high)high=v;total+=v;}
                dprintf(2,"camera%d image range %u..%u mean %lu timestamp=%lu.%09lu\n",i,low,high,total/(640*480),raw[0],raw[1]);
                char path[64];snprintf(path,sizeof(path),"/tmp/camera%d.pgm",i);
                int out=open(path,0x241,0600);
                if(out>=0){
                    dprintf(out,"P5\n640 480\n255\n");
                    unsigned done=0;while(done<640*480){long n=write(out,pixels+done,640*480-done);if(n<=0)break;done+=(unsigned)n;}
                    close(out);dprintf(2,"saved camera%d %u image bytes\n",i,done);
                }
            }
            frames[i]++;enqueue(sensors[i],frame);
        }
        int raw_fresh=raw_capture!=0;
        for(unsigned c=0;raw_fresh&&c<4;c++)if(feed.timestamp_ns[c]<=raw_capture->requested_ns)raw_fresh=0;
        if(raw_fresh&&quest_camera_feed_synchronized(&feed)){
            unsigned flags=auto_exposure|(exposure_verified<<1)|(denoise<<2);
            int result=raw_capture_publish(raw_capture,&feed,settings,flags);
            dprintf(2,"RAW_CAPTURE one-shot %s: /tmp/quest-camera-raw-v1.bin\n",result?"FAILED":"complete");
            free(raw_capture);raw_capture=0; /* No retries or continuous recording. */
        }
        /* Isolated one-shot identification experiment. Only after both initial
         * values were seen on every camera; never change default auto policy. */
        if(bank_diagnostic.step_enabled&&frame_trace.enabled&&quest_bank_step_due(&bank_diagnostic,monotonic_ns(),exposure_verified)){
            bank_diagnostic.step_applied=1;
            dprintf(2,"BANK_DIAGNOSTIC_STEP ns=%llu bank0_us=8000 bank1_us=8000 gain=16 initial_verified=1\n",monotonic_ns());
            request_exposure(settings,3U);
            if(scene_bank_only&&scene_control_failed){failed=1;goto cleanup;}
            last_exposure_request=monotonic_ns();
        }
        if(scene_bank_only){
            enum quest_controller_ack_action controller_action=quest_controller_ack_poll(&controller_ack,monotonic_ns(),last_any_bank_command);
            if(controller_action==QUEST_CONTROLLER_ACK_LOST){
                unlink("/tmp/camera-feed");
                exposure_ack=(struct quest_exposure_ack){0};exposure_verified=0;last_exposure_request=0;
                for(unsigned c=0;c<4;c++){feed.frames[c]=0;feed.timestamp_ns[c]=0;}
                dprintf(2,"SCENE_BANK_ONLY controller readback lost; scene publication quarantined\n");
            }
            if(controller_action==QUEST_CONTROLLER_ACK_TIMEOUT)
                dprintf(2,"SCENE_BANK_ONLY controller unconfirmed for10s; writes paused, scene quarantined\n");
            if(controller_action==QUEST_CONTROLLER_ACK_RETRY){
                request_exposure(settings,2U);
                if(scene_control_failed){failed=1;goto cleanup;}
                last_any_bank_command=monotonic_ns();
                quest_controller_ack_sent(&controller_ack,last_any_bank_command);
            }
            if(controller_action==QUEST_CONTROLLER_ACK_CONFIRMED){
                unsigned us[4],gain[4];
                exposure_ack=(struct quest_exposure_ack){0};exposure_verified=0;last_exposure_request=0;
                for(unsigned c=0;c<4;c++){
                    us[c]=settings[c].us;gain[c]=settings[c].gain;
                    feed.frames[c]=0;feed.timestamp_ns[c]=0;
                }
                if(quest_exposure_ack_target(&exposure_ack,us,gain,monotonic_ns())!=QUEST_EXPOSURE_TARGET_CHANGED){
                    dprintf(2,"SCENE_BANK_ONLY scene target initialization rejected; stopping capture\n");failed=1;goto cleanup;
                }
                request_exposure(settings,1U);
                if(scene_control_failed){failed=1;goto cleanup;}
                last_exposure_request=last_any_bank_command=monotonic_ns();
                quest_exposure_ack_sent(&exposure_ack,last_exposure_request);
                dprintf(2,"SCENE_BANK_ONLY controller confirmed on all four cameras; scene target requested\n");
            }
        }
        if(mcu&&getenv("CAMERA_EXPOSURE")&&(scene_bank_only||(feed.frames[0]&&feed.frames[1]&&feed.frames[2]&&feed.frames[3]))){
            unsigned long long exposure_now=monotonic_ns();
            if(scene_bank_only&&controller_ack.confirmed){
                enum quest_exposure_ack_action action=quest_exposure_ack_poll(&exposure_ack,&feed,exposure_now);
                if(exposure_ack.confirmed&&!exposure_verified){
                    exposure_verified=1;
                    dprintf(2,"Startup scene target confirmed on two consecutive cohorts across all four cameras\n");
                }
                int send_target=action==QUEST_EXPOSURE_ACK_RETRY;
                if(action==QUEST_EXPOSURE_ACK_TIMEOUT)
                    dprintf(2,"SCENE_BANK_ONLY target unconfirmed for10s; automatic changes paused, capture continues\n");
                if(action==QUEST_EXPOSURE_ACK_EVALUATE&&auto_exposure){
                    struct quest_exposure next[4];unsigned us[4],gain[4];
                    quest_exposure_group_next(feed.pixels,feed.exposure_us,feed.gain_q4,next);
                    for(unsigned c=0;c<4;c++){us[c]=next[c].us;gain[c]=next[c].gain;}
                    int result=quest_exposure_ack_target(&exposure_ack,us,gain,exposure_now);
                    if(result==QUEST_EXPOSURE_TARGET_REFUSED){
                        dprintf(2,"SCENE_BANK_ONLY target transition refused; stopping capture\n");failed=1;goto cleanup;
                    }
                    if(result==QUEST_EXPOSURE_TARGET_CHANGED){
                        for(unsigned c=0;c<4;c++)settings[c]=next[c];
                        send_target=1;
                    }
                }
                if(send_target){
                    /* Retries preserve the entire desired tuple; never derive
                     * a new target from partial MCU application. */
                    for(unsigned c=0;c<4;c++){
                        settings[c].us=exposure_ack.desired_us[c];settings[c].gain=exposure_ack.desired_gain[c];
                    }
                    request_exposure(settings,1U);
                    if(scene_control_failed){failed=1;goto cleanup;}
                    last_exposure_request=last_any_bank_command=monotonic_ns();
                    quest_exposure_ack_sent(&exposure_ack,last_exposure_request);
                }
            }else if(!scene_bank_only){
                /* Preserve the existing default startup/AE behavior. */
                if(!exposure_verified){
                    unsigned matched=1;
                    for(unsigned i=0;i<4;i++){
                        if(bank_diagnostic.enabled){if(bank_seen[i]!=3)matched=0;}
                        else if(feed.exposure_us[i]+19<setting_us||feed.exposure_us[i]>setting_us+19||feed.gain_q4[i]!=setting_gain)matched=0;
                    }
                    if(matched){
                        exposure_verified=1;
                        dprintf(2,bank_diagnostic.enabled?"Diagnostic readback observed both requested settings on all four cameras\n":"Startup exposure verified on all four cameras\n");
                    }
                    else if(exposure_now-last_exposure_request>1000000000ULL){
                        request_exposure(settings,3U);
                        last_exposure_request=exposure_now;
                    }
                }else if(auto_exposure&&exposure_now-last_exposure_request>500000000ULL){
                    quest_exposure_group_next(feed.pixels,feed.exposure_us,feed.gain_q4,settings);
                    request_exposure(settings,3U);
                    last_exposure_request=exposure_now;
                }
            }
        }
        if(live&&(!scene_bank_only||(controller_ack.confirmed&&exposure_verified))&&feed.frames[0]>=last_published+2&&quest_camera_feed_synchronized(&feed)){
            feed.publication++;feed.published_ns=monotonic_ns();
            int out=open("/tmp/camera-feed.new",0x241,0600);
            if(out>=0){
                size_t done=0;while(done<sizeof(feed)){long n=write(out,(const char *)&feed+done,sizeof(feed)-done);if(n<=0)break;done+=(size_t)n;}
                close(out);if(done==sizeof(feed)){
                    int published=rename("/tmp/camera-feed.new","/tmp/camera-feed");
                    if(diagnostics&&published==0)published_ok++;
                }
            }
            last_published=feed.frames[0];
        }
        if(diagnostics){
            unsigned long long now=monotonic_ns();
            if(quest_camera_diagnostics_due(now,&last_diagnostic)){
                /* Five bounded lines per second; counters are cumulative. Statistics
                 * describe latest retained scene frames, not the interleaved short
                 * frames. Record last raw metadata too so rejected-frame boundaries
                 * remain diagnosable even if scene publication has stopped. */
                dprintf(2,"CAMERA_DIAG ns=%llu exposure_enabled=%u auto=%u verified=%u request_ns=%llu publication=%u published_ok=%llu denoise=%u diagnostic_banks=%u bank0_us=%u bank1_us=%u bank_step=%u scene_only=%u target_confirmed=%u target_streak=%u target_timeout=%u controller_confirmed=%u controller_timeout=%u\n",
                    now,(unsigned)(mcu&&getenv("CAMERA_EXPOSURE")!=0),auto_exposure,
                    exposure_verified,last_exposure_request,feed.publication,published_ok,denoise,bank_diagnostic.enabled,requested_bank[0][0].us,requested_bank[1][0].us,bank_diagnostic.step_applied,scene_bank_only,exposure_ack.confirmed,exposure_ack.streak,exposure_ack.timed_out,controller_ack.confirmed,controller_ack.timed_out);
                for(unsigned c=0;c<4;c++){
                    struct quest_camera_statistics stats=quest_camera_statistics(feed.pixels[c]);
                    dprintf(2,"CAMERA_DIAG cam=%u scene_seq=%u scene_ns=%llu requested_us=%u requested_gain=%u scene_us=%u scene_gain=%u last_us=%u last_gain=%u roi_p70=%u roi_clipped=%u roi_samples=%u rejected_short=%llu retained_scene=%llu rejected_class2=%llu rejected_class4=%llu rejected_other=%llu controller_stock_frames=%llu scene_match=%llu scene_mismatch=%llu scene_transition=%llu last_bad_seq=%u last_bad_us=%u last_bad_gain=%u quarantined_scene=%llu rejected_scene_metadata=%llu\n",
                        c,feed.frames[c],feed.timestamp_ns[c],bank_diagnostic.enabled?0:settings[c].us,bank_diagnostic.enabled?0:settings[c].gain,
                        feed.exposure_us[c],feed.gain_q4[c],last_us[c],last_gain[c],
                        stats.p70,stats.clipped,stats.samples,short_frames[c],retained_scene[c],rejected_class2[c],rejected_class4[c],rejected_other[c],controller_stock_frames[c],scene_match[c],scene_mismatch[c],scene_transition[c],last_bad_seq[c],last_bad_us[c],last_bad_gain[c],quarantined_scene[c],rejected_scene_metadata[c]);
                    if(cpu_invalidate)dprintf(2,"CPU_DIAG cam=%u invalidations=%llu metadata_match=%llu metadata_mismatch=%llu\n",c,cpu_reads[c],metadata_match[c],metadata_mismatch[c]);
                }
            }
        }
        if(frame_trace.enabled&&!frame_trace.reported&&!quest_frame_trace_active(&frame_trace,monotonic_ns())){
            frame_trace.reported=1;
            dprintf(2,"CAMERA_TRACE_END ns=%llu frames=%u commands=%u\n",monotonic_ns(),frame_trace.frames,frame_trace.commands);
            frame_trace.enabled=0;
        }
        usleep(2000);
    }
cleanup:
    free(raw_capture);
    if(stream_started&&camera_command(45,&count,1,125))failed=1;
    for(int i=0;i<4;i++){
        dprintf(2,"camera%d received %d frames\n",i,frames[i]);
        if(started[i])dprintf(2,"camera%d stop rc=%d\n",i,stop(sensors[i]));
        if(sensors[i])release(sensors[i]);
    }
    close_hal(hal);
    if(cpu_ion_fd>=0)close(cpu_ion_fd);
    if(mcu){
        if(camera_command(47,&count,1,125))failed=1;
        if(camera_command(41,0,0,125))failed=1;
        close(command_fd);close(control_fd);
    }
    if(live)unlink("/tmp/camera-feed");
    dprintf(2,"camera probe closed cleanly (status %d)\n",failed);_exit(failed?2:0);
}
