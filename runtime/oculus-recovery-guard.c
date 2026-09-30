/* Resettable software recovery timer. Does not open or disable hardware watchdogs.
 * Parent owns deadline; isolated input child may request renewals. Child failure
 * or parent termination requests recovery. Adoption never precedes arming.
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "recovery-deadline.h"
#ifndef STATE
#define STATE "/run/oculus-recovery"
#endif
#define STATUS STATE "/status"
#define INPUTS 8
static volatile sig_atomic_t interrupted, poweroff_requested, renew_requested;
static void on_signal(int s) { if(s==SIGUSR2)poweroff_requested=1;else if(s==SIGUSR1)renew_requested=1;else interrupted=1; }
static double monotonic(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec/1e9; }
static int proc_stat(pid_t pid, pid_t *parent, unsigned long long *ticks) {
    char path[80], buf[4096]; snprintf(path,sizeof(path),"/proc/%d/stat",pid);
    FILE *f=fopen(path,"r"); if(!f)return 0;
    int ok=fgets(buf,sizeof(buf),f)!=NULL; fclose(f); if(!ok)return 0;
    char *p=strrchr(buf,')'),*save=NULL; if(!p)return 0;
    int field=3; for(char *s=strtok_r(p+2," ",&save);s;s=strtok_r(NULL," ",&save),field++) {
        if(field==4)*parent=(pid_t)strtol(s,NULL,10);
        if(field==22){*ticks=strtoull(s,NULL,10);return 1;}
    } return 0;
}
static int cmdline(pid_t pid,char *buf,size_t size) {
    char path[80];snprintf(path,sizeof(path),"/proc/%d/cmdline",pid);
    int fd=open(path,O_RDONLY);if(fd<0)return 0;
    ssize_t n=read(fd,buf,size-1);close(fd);if(n<=0)return 0;
    for(ssize_t i=0;i<n-1;i++)if(!buf[i])buf[i]=' ';
    buf[n]=0;return 1;
}
static int legacy_command(pid_t pid,int early) {
    char s[1024];return cmdline(pid,s,sizeof(s)) && !strcmp(s,early?"sh /hooks/20-oculus-recovery.sh":"/bin/busybox ash -c echo ready > /run/oculus-root-watchdog-ready; sleep 300; /usr/sbin/reboot-mode bootloader");
}
struct legacy { pid_t shell,sleep; unsigned long long shell_ticks,sleep_ticks; double deadline; };
static int find_legacy(struct legacy *old,int early) {
    DIR *d=opendir("/proc");if(!d)return 0;struct dirent *e;int count=0;
    while((e=readdir(d))) {pid_t pid=(pid_t)atoi(e->d_name);if(pid>1&&legacy_command(pid,early)){old->shell=pid;count++;}}
    closedir(d);if(count!=1)return 0;
    pid_t parent; if(!proc_stat(old->shell,&parent,&old->shell_ticks))return 0;
    d=opendir("/proc");if(!d)return 0;count=0;
    while((e=readdir(d))) {
        pid_t pid=(pid_t)atoi(e->d_name); unsigned long long ticks;char s[128];
        if(pid>1&&proc_stat(pid,&parent,&ticks)&&parent==old->shell&&cmdline(pid,s,sizeof(s))&&
           (!strcmp(s,"sleep 300")||!strcmp(s,"/bin/busybox sleep 300"))) {
            old->sleep=pid;old->sleep_ticks=ticks;count++;
        }
    }
    closedir(d);old->deadline=(double)old->sleep_ticks/sysconf(_SC_CLK_TCK)+300.0;
    return count==1 && old->deadline-monotonic()>30;
}
static int same_process(pid_t pid,unsigned long long ticks) {
    pid_t parent;unsigned long long actual;
    return proc_stat(pid,&parent,&actual)&&actual==ticks;
}
static int publish(double deadline,unsigned resets,unsigned long long ticks) {
    FILE *f=fopen(STATE "/status.new","w");if(!f)return 0;
    fprintf(f,"%d %llu %.3f %.3f %u\n",getpid(),ticks,deadline,monotonic(),resets);
    if(fclose(f))return 0;
    return rename(STATE "/status.new",STATUS)==0;
}
static int check(void) {
    FILE *f=fopen(STATUS,"r");if(!f)return 1;int pid;unsigned resets;unsigned long long ticks;double end,updated;
    int n=fscanf(f,"%d %llu %lf %lf %u",&pid,&ticks,&end,&updated,&resets);fclose(f);
    double now=monotonic();char s[1024];
    return !(n==5&&pid>1&&end>now&&updated<=now&&now-updated<2&&same_process(pid,ticks)&&
             cmdline(pid,s,sizeof(s))&&!strcmp(s,"/usr/sbin/oculus-recovery-guard --run"));
}
static _Noreturn void recovery(void) {
#ifdef RECOVERY_GUARD_TEST
    _exit(77); /* Test builds cannot invoke reboot executables. */
#else
    /* This is only reached by an armed guard, never --check or failed preflight. */
    fprintf(stderr,"Recovery deadline expired or guard failed; returning to bootloader\n");fflush(stderr);
    pid_t child=fork();if(child==0){execl("/usr/sbin/reboot-mode","reboot-mode","bootloader",NULL);_exit(127);}
    for(int i=0;i<50;i++){struct timespec t={0,100000000};nanosleep(&t,NULL);}
    execl("/sbin/reboot","reboot",NULL);_exit(111);
#endif
}
static _Noreturn void orderly_poweroff(void) {
#ifdef RECOVERY_GUARD_TEST
    _exit(78);
#else
    /* Explicit root request only: remain here until init completes shutdown.
     * Ignore init's TERM during this authorized path; force power-off if init
     * cannot finish within 30 seconds. Ordinary TERM still requests recovery. */
    signal(SIGTERM,SIG_IGN);signal(SIGINT,SIG_IGN);signal(SIGHUP,SIG_IGN);
    pid_t child=fork();
    if(child==0){execl("/sbin/poweroff","poweroff",NULL);_exit(127);}
    for(int i=0;i<300;i++){struct timespec t={0,100000000};nanosleep(&t,NULL);}
    sync();execl("/sbin/poweroff","poweroff","-f",NULL);recovery();
#endif
}
struct input { int fd, dropped; dev_t device; ino_t inode; struct recovery_hold hold; };
static int key_down(int fd) {
    unsigned char bits[(KEY_MAX+8)/8]={0};
    if(ioctl(fd,EVIOCGKEY(sizeof(bits)),bits)<0)return -1;
    return !!(bits[BTN_TRIGGER/8]&(1U<<(BTN_TRIGGER%8)));
}
static void scan_inputs(struct input *inputs) {
    DIR *d=opendir("/dev/input");if(!d)return;struct dirent *e;
    while((e=readdir(d))) {
        if(strncmp(e->d_name,"event",5))continue;
        char path[300];snprintf(path,sizeof(path),"/dev/input/%s",e->d_name);
        struct stat st;if(stat(path,&st))continue;int seen=0,slot=-1;
        for(int i=0;i<INPUTS;i++){if(inputs[i].fd<0)slot=i;else if(inputs[i].device==st.st_rdev&&inputs[i].inode==st.st_ino)seen=1;}
        if(seen||slot<0)continue;
        int fd=open(path,O_RDONLY|O_NONBLOCK|O_CLOEXEC);if(fd<0)continue;
        char name[128]={0};unsigned char keys[(KEY_MAX+8)/8]={0};
        if(ioctl(fd,EVIOCGNAME(sizeof(name)),name)<0||strcmp(name,"Oculus Touch desktop pointer")||
           ioctl(fd,EVIOCGBIT(EV_KEY,sizeof(keys)),keys)<0||!(keys[BTN_TRIGGER/8]&(1U<<(BTN_TRIGGER%8)))){close(fd);continue;}
        inputs[slot]=(struct input){.fd=fd,.device=st.st_rdev,.inode=st.st_ino};
        int down=key_down(fd);if(down==0)recovery_input(&inputs[slot].hold,0,monotonic());
    }closedir(d);
}
static void input_worker(int output) {
    struct input inputs[INPUTS];for(int i=0;i<INPUTS;i++)inputs[i]=(struct input){.fd=-1};
    double scanned=-10;
    for(;;) {
        double now=monotonic();if(now-scanned>=2){scan_inputs(inputs);scanned=now;}
        struct pollfd polls[INPUTS];for(int i=0;i<INPUTS;i++)polls[i]=(struct pollfd){.fd=inputs[i].fd,.events=POLLIN};
        if(poll(polls,INPUTS,50)<0&&errno!=EINTR)_exit(2);
        now=monotonic();
        for(int i=0;i<INPUTS;i++) {
            struct input *in=&inputs[i];if(in->fd<0)continue;
            if(polls[i].revents&(POLLHUP|POLLERR|POLLNVAL)){close(in->fd);in->fd=-1;continue;}
            struct input_event event;ssize_t n;
            while((n=read(in->fd,&event,sizeof(event)))==(ssize_t)sizeof(event)) {
                if(event.type==EV_SYN&&event.code==SYN_DROPPED){in->dropped=1;in->hold=(struct recovery_hold){0};}
                else if(in->dropped){
                    if(event.type==EV_SYN&&event.code==SYN_REPORT){in->dropped=0;if(key_down(in->fd)==0)recovery_input(&in->hold,0,now);}
                } else if(event.type==EV_KEY&&event.code==BTN_TRIGGER&&event.value!=2)recovery_input(&in->hold,event.value!=0,now);
            }
            if(n==0||(n<0&&errno!=EAGAIN&&errno!=EINTR)){close(in->fd);in->fd=-1;continue;}
            /* Live key query avoids renewing from a stale/disconnected press. */
            if(in->hold.down&&key_down(in->fd)!=1)in->hold=(struct recovery_hold){0};
            if(recovery_hold_ready(&in->hold,now)&&write(output,"R",1)!=1)_exit(3);
        }
    }
}
static _Noreturn void recovery_loop(pid_t worker,int input_fd,double deadline,unsigned long long ticks) {
    unsigned resets=0;
    for(;;) {
        if(poweroff_requested)orderly_poweroff();
        if(interrupted||monotonic()>=deadline)recovery();
        /* Explicit root maintenance renewal; no automatic keepalive. */
        if(renew_requested){renew_requested=0;if(!recovery_renew(monotonic(),&deadline))recovery();resets++;}
        int status;if(waitpid(worker,&status,WNOHANG)!=0)recovery();
        struct pollfd p={.fd=input_fd,.events=POLLIN};int r=poll(&p,1,100);
        if(r<0&&errno!=EINTR)recovery();
        if(p.revents&(POLLHUP|POLLERR|POLLNVAL))recovery();
        char requests[8];ssize_t n=read(input_fd,requests,sizeof(requests));
        if(n>0){if(!recovery_renew(monotonic(),&deadline))recovery();resets++;}
        if(!publish(deadline,resets,ticks))recovery();
    }
}
int main(int argc,char **argv) {
    if(argc==2&&!strcmp(argv[1],"--check"))return check();
    if(argc==2&&(!strcmp(argv[1],"--poweroff")||!strcmp(argv[1],"--renew"))){
        if(geteuid()!=0||check())return 2;
        FILE *f=fopen(STATUS,"r");int pid=0;if(!f)return 2;
        (void)fscanf(f,"%d",&pid);fclose(f);
        /* A replaced on-disk binary must never signal an older live guard that
         * may not implement this request. Activate upgrades on the next boot. */
        char executable[64];struct stat running,self;
        snprintf(executable,sizeof(executable),"/proc/%d/exe",pid);
        if(stat(executable,&running)||stat("/proc/self/exe",&self)||
           running.st_dev!=self.st_dev||running.st_ino!=self.st_ino){
            fputs("Guard upgrade requires a fresh boot before control requests\n",stderr);return 2;
        }
        return pid>1&&kill(pid,!strcmp(argv[1],"--poweroff")?SIGUSR2:SIGUSR1)==0?0:2;
    }
    if(argc!=2||strcmp(argv[1],"--run")){fputs("usage: oculus-recovery-guard --run|--check|--poweroff|--renew\n",stderr);return 2;}
    if(geteuid()!=0)return 2;
    FILE *hw=fopen("/sys/devices/soc/17817000.qcom,wdt/disable","r");int disabled=1;
    if(hw){(void)fscanf(hw,"%d",&disabled);fclose(hw);}if(disabled)return 2;
    umask(077);if(mkdir(STATE,0700)&&errno!=EEXIST)return 2;
    int lock=open(STATE "/lock",O_CREAT|O_RDWR|O_CLOEXEC,0600);if(lock<0||flock(lock,LOCK_EX|LOCK_NB))return 2;
    struct legacy old={0};if(!find_legacy(&old,0)){fputs("Cannot safely adopt exact legacy timer; leaving it unchanged\n",stderr);return 2;}
    struct legacy early={0};
    if(!find_legacy(&early,1)){fputs("Cannot identify initramfs recovery timer; leaving guards unchanged\n",stderr);return 2;}
    FILE *legacy_pid=fopen("/run/oculus-initramfs-watchdog.pid","r");int recorded=0;
    if(legacy_pid){(void)fscanf(legacy_pid,"%d",&recorded);fclose(legacy_pid);}
    if(recorded!=early.shell)return 2;
    pid_t parent;unsigned long long ticks;if(!proc_stat(getpid(),&parent,&ticks))return 2;
    int pipefd[2];if(pipe2(pipefd,O_CLOEXEC|O_NONBLOCK))return 2;
    pid_t worker=fork();if(worker<0)return 2;
    if(worker==0){close(pipefd[0]);close(lock);input_worker(pipefd[1]);_exit(3);}
    close(pipefd[1]);signal(SIGTERM,on_signal);signal(SIGINT,on_signal);signal(SIGHUP,on_signal);signal(SIGUSR2,on_signal);signal(SIGUSR1,on_signal);
    double deadline=old.deadline<early.deadline?old.deadline:early.deadline;unsigned resets=0;
    if(!publish(deadline,resets,ticks)){kill(worker,SIGKILL);return 2;}
    /* Arm BEFORE stopping legacy shell. Preserve its remaining time, not a new
     * five minutes. Exact cmdline/starttime checks prevent unrelated PID kills.
     * No shell child can finish and dispatch reboot while its parent is stopped.
     */
    if(!same_process(old.shell,old.shell_ticks)||!legacy_command(old.shell,0)||kill(old.shell,SIGSTOP))recovery();
    if(!same_process(early.shell,early.shell_ticks)||!legacy_command(early.shell,1)||kill(early.shell,SIGSTOP))recovery();
    if(!same_process(old.sleep,old.sleep_ticks)||monotonic()>=deadline-10){kill(old.shell,SIGCONT);recovery();}
    if(!same_process(early.sleep,early.sleep_ticks))recovery();
    if(kill(old.shell,SIGKILL)||kill(old.sleep,SIGKILL)||kill(early.shell,SIGKILL)||kill(early.sleep,SIGKILL))recovery();
    fprintf(stderr,"Adopted recovery timer, %.1fs remaining; hold either trigger 2s to renew to 300s\n",deadline-monotonic());fflush(stderr);
    recovery_loop(worker,pipefd[0],deadline,ticks);
}
