/* Bionic-only diagnostic preload. No stock proprietary code is included.
 * Compile with clang --target=aarch64-linux-android29 -fuse-ld=lld -nostdlib
 * -shared -fPIC -O2 -Wall -Wextra -Werror -LOWNER_BIONIC -lc -ldl.
 * Explicit libc dependency is required for constructor initialization order.
 * Run only in the restricted RAM root.
 * Initial milestone: load the owner's HAL and open/query/close sensors, no stream.
 */
#include "camera-feed.h"
typedef unsigned long size_t;
extern void *dlopen(const char *, int);
extern void *dlsym(void *, const char *);
extern char *dlerror(void);
extern int dprintf(int, const char *, ...);
extern int vdprintf(int, const char *, __builtin_va_list);
extern int *__errno(void);
extern unsigned alarm(unsigned);
extern char *getenv(const char *);
extern int usleep(unsigned);
extern int open(const char *,int,...);
extern long read(int,void *,size_t);
extern long write(int,const void *,size_t);
extern int close(int);
extern int rename(const char *,const char *);
extern int unlink(const char *);
extern void (*signal(int,void (*)(int)))(int);
struct timespec {long tv_sec,tv_nsec;};
extern int clock_gettime(int,struct timespec *);
static volatile int keep_running=1;
static void interrupted(int sig){(void)sig;keep_running=0;}
static unsigned long long monotonic_ns(void){struct timespec t;clock_gettime(1,&t);return (unsigned long long)t.tv_sec*1000000000+t.tv_nsec;}
static struct quest_camera_feed feed;
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
__attribute__((constructor)) static void probe(void) {
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
    if(live&&!(started[0]&&started[1]&&started[2]&&started[3])){failed=1;goto cleanup;}
    if(mcu&&started[0]){
        if(getenv("CAMERA_EXPOSURE")){
            const unsigned char mode[]={141,1};camera_command(3,mode,2,125);
        }
        if(camera_command(44,&count,1,0)){failed=1;goto cleanup;}
        stream_started=1;
    }
    unsigned last_published=0;
    feed.magic=QUEST_CAMERA_MAGIC;feed.version=1;feed.width=320;feed.height=240;feed.cameras=4;
    for(unsigned attempt=0;keep_running&&(live||attempt<1500)&&getenv("CAMERA_STREAM");attempt++){
        if(attempt==10&&mcu&&getenv("CAMERA_EXPOSURE")){
            /* Current stock command54: u16 exposure-us[4], Q4 gain[4],
             * tags[4], controller-exposure flag. Sent after stream starts. */
            unsigned char exposure[]={0xe0,0x2e,0xe0,0x2e,0xe0,0x2e,0xe0,0x2e,240,0,240,0,240,0,240,0,1,1,1,1,0};
            camera_command(54,exposure,sizeof(exposure),0);
            exposure[20]=1;camera_command(54,exposure,sizeof(exposure),0);
        }
        for(int i=0;i<4;i++)if(started[i]){
            void *frame=dequeue(sensors[i]);if(!frame)continue;
            unsigned long *raw=frame;
            if(frames[i]<3)dprintf(2,"camera%d frame%d timestamp=%lu.%09lu seq=%lu data=%lx\n",i,frames[i],raw[0],raw[1],raw[3],raw[6]);
            const unsigned char *metadata=(const unsigned char *)raw[6];
            /* Keep the long-exposure scene frames. The MCU interleaves short
             * controller frames; never let these produce a flickering preview. */
            unsigned exposure_us=((unsigned)metadata[6]*256+metadata[7])*19;
            if(exposure_us<1000){frames[i]++;enqueue(sensors[i],frame);continue;}
            const unsigned char *source=metadata+640;
            unsigned total=0;
            for(unsigned y=0;y<240;y++)for(unsigned x=0;x<320;x++){
                unsigned value=source[y*1280+x*2];feed.pixels[i][y*320+x]=(unsigned char)value;total+=value;
            }
            feed.mean[i]=total/QUEST_CAMERA_PIXELS;
            feed.frames[i]=(unsigned)raw[3];feed.timestamp_ns[i]=raw[0]*1000000000ULL+raw[1];
            feed.exposure_us[i]=((unsigned)metadata[6]*256+metadata[7])*19;feed.gain_q4[i]=metadata[3];
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
        if(live&&feed.frames[0]>=last_published+2&&feed.frames[1]&&feed.frames[2]&&feed.frames[3]){
            feed.publication++;feed.published_ns=monotonic_ns();
            int out=open("/tmp/camera-feed.new",0x241,0600);
            if(out>=0){
                size_t done=0;while(done<sizeof(feed)){long n=write(out,(const char *)&feed+done,sizeof(feed)-done);if(n<=0)break;done+=(size_t)n;}
                close(out);if(done==sizeof(feed))rename("/tmp/camera-feed.new","/tmp/camera-feed");
            }
            last_published=feed.frames[0];
        }
        usleep(2000);
    }
cleanup:
    if(stream_started&&camera_command(45,&count,1,125))failed=1;
    for(int i=0;i<4;i++){
        dprintf(2,"camera%d received %d frames\n",i,frames[i]);
        if(started[i])dprintf(2,"camera%d stop rc=%d\n",i,stop(sensors[i]));
        if(sensors[i])release(sensors[i]);
    }
    close_hal(hal);
    if(mcu){
        if(camera_command(47,&count,1,125))failed=1;
        if(camera_command(41,0,0,125))failed=1;
        close(command_fd);close(control_fd);
    }
    if(live)unlink("/tmp/camera-feed");
    dprintf(2,"camera probe closed cleanly (status %d)\n",failed);_exit(failed?2:0);
}
