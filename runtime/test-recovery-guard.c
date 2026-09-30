/* Native Linux process tests. Compile-time stub guarantees NO reboot calls. */
#define RECOVERY_GUARD_TEST
#define STATE "state"
#define main production_main
#include "oculus-recovery-guard.c"
#undef main
#include <assert.h>
static void pause_ms(int ms){struct timespec t={ms/1000,(ms%1000)*1000000};nanosleep(&t,NULL);}
static void scenario(int renew,int child_fails,int already_expired,int poweroff_test) {
 int pipefd[2];assert(pipe2(pipefd,O_CLOEXEC|O_NONBLOCK)==0);
 pid_t guard=fork();assert(guard>=0);
 if(!guard){
  pid_t worker=fork();assert(worker>=0);
  if(!worker){close(pipefd[0]);if(child_fails)_exit(0);if(renew){pause_ms(50);assert(write(pipefd[1],"R",1)==1);}pause_ms(900);_exit(0);}
  close(pipefd[1]);signal(SIGTERM,on_signal);signal(SIGUSR2,on_signal);signal(SIGUSR1,on_signal);
  recovery_loop(worker,pipefd[0],monotonic()+(already_expired?-.1:.2),0);_exit(90);
 }
 close(pipefd[0]);close(pipefd[1]);
 if(renew&&!child_fails&&!already_expired){
  pause_ms(350);assert(kill(guard,0)==0);
  FILE *f=fopen(STATUS,"r");assert(f);int pid;unsigned resets;unsigned long long ticks;double end,updated;
  assert(fscanf(f,"%d %llu %lf %lf %u",&pid,&ticks,&end,&updated,&resets)==5);fclose(f);
  assert(pid==guard&&resets==1&&end-monotonic()>299);
  assert(kill(guard,SIGUSR1)==0);
  for(int attempt=0;attempt<10;attempt++){
   pause_ms(30);f=fopen(STATUS,"r");assert(f);
   assert(fscanf(f,"%d %llu %lf %lf %u",&pid,&ticks,&end,&updated,&resets)==5);fclose(f);
   if(resets==2)break;
  }
  assert(resets==2&&end-monotonic()>299);
  assert(kill(guard,poweroff_test?SIGUSR2:SIGTERM)==0); /* Termination must request recovery, not exit quietly. */
 }
 int status;assert(waitpid(guard,&status,0)==guard);assert(WIFEXITED(status)&&WEXITSTATUS(status)==(poweroff_test?78:77));
}
int main(void){
 char temp[]="/tmp/quest-guard-test-XXXXXX";assert(mkdtemp(temp));assert(chdir(temp)==0);assert(mkdir(STATE,0700)==0);
 scenario(0,0,0,0); /* Unattended expiry. */
 scenario(1,0,0,0); /* Renewal stays live beyond original deadline; SIGTERM recovers. */
 scenario(0,1,0,0); /* Input worker death fails closed. */
 scenario(1,0,1,0); /* An already-expired deadline cannot be revived. */
 scenario(1,0,0,1); /* Explicit poweroff is distinct from failure recovery. */
 pause_ms(1000);unlink(STATUS);unlink(STATE "/status.new");rmdir(STATE);assert(chdir("/")==0);rmdir(temp);
 puts("Guard process tests passed: expiry, renewal, termination, worker failure, overdue input, explicit poweroff");
}
