
#include "rtthread.h"
//#include "shell.h"
//#include "bonfire.h"
#include "rthw.h"
#include "string.h"
#include "malloc.h"
#include "stdio.h"


static void reboot(int argc, char** argv)
{
   rt_hw_cpu_shutdown();
}

MSH_CMD_EXPORT(reboot, reboot sample: reboot);


rt_mailbox_t  mb;


static void sendMessage(char *msg) 
{
    char * buffer = malloc(strlen(msg)+1);
    strcpy(buffer,msg);
    rt_err_t err=rt_mb_send(mb,(rt_ubase_t)buffer);
    rt_kprintf("thread %s: mb_send returns %d\n",rt_thread_self()->name,err);

}

static rt_timer_t timer = NULL;

static void  messageTimerCallBack()
{
char msg[80];
static int count=0;

    rt_kprintf("in messageTimerCallBack\n");
    snprintf(msg,sizeof(msg),"Timer message %d",count++);
    sendMessage(msg);
}


static void test(int argc,char **argv)
{
 
  // rt_kprintf("argc: %d\n",argc);
  // for(int i=0;i<argc;i++) {
  //   rt_kprintf("Arg %d: %s\n",i,argv[i]);
  // }
  if (argc>=2) {
     switch (argv[1][0]) {
       case 'm':
          {
            void *buffer = malloc(256);
            rt_kprintf("Buffer allocated at %lx\n",buffer);
            rt_thread_mdelay(100);
            free(buffer);
          }
          break;
       case 'c':
          {
            rt_thread_t t = rt_thread_self();
            rt_kprintf("Current thread id %lx name: %s\n",t,t->name);
          }
          break;   
       case 's':          
          sendMessage(argc>=3?argv[2]:"default message");
          break;
       case 't':
          if (!timer) {
            timer=rt_timer_create("tim01",messageTimerCallBack,NULL,10000,RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
            RT_ASSERT(timer);
            rt_kprintf("Timer %lx created\n",timer);
            rt_timer_start(timer);           
          } else {  
            rt_timer_delete(timer);
            timer=NULL;
          }            
                    
     }
  }
}

//MSH_CMD_EXPORT(test,Usage: test m or test t)


void sender()
{
    while(1) {
      rt_kprintf("thread %s\n",rt_thread_self()->name);
      rt_thread_mdelay(1000);
      messageTimerCallBack();
    }
   

}

int main() {

char * message;

  //rt_kprintf("Pulling finsh_system_init:  %lx\n",finsh_system_init);
  mb = rt_mb_create("mb01",1,RT_IPC_FLAG_FIFO);
  RT_ASSERT(mb);
  
  rt_thread_t t2 = rt_thread_create("Sender",sender,NULL,2048,16,100);
  RT_ASSERT(t2);
  rt_thread_startup(t2);

  //timer=rt_timer_create("tim01",messageTimerCallBack,NULL,2000,RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
  //rt_kprintf("Timer %lx created\n",timer);
  while (1) {
    rt_kprintf("thread %s\n",rt_thread_self()->name);
    //rt_thread_mdelay(1000);
    rt_err_t err=rt_mb_recv(mb,(rt_ubase_t *)&message,10000);
    if (err==RT_EOK) {
      rt_kprintf("thread %s received message: %s\n",rt_thread_self()->name,message);
      free(message);
    }
  }

}