
#include "rtthread.h"
#ifdef RT_USING_FINSH
#include "shell.h"
#endif
//#include "bonfire.h"
#include "rthw.h"
#include "string.h"
#include "malloc.h"
#include "stdio.h"
#include "mem_rw.h"


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
    rt_snprintf(msg,sizeof(msg),"Timer message %d",count++);
    sendMessage(msg);
}

#ifdef RT_USING_FINSH


static void test(int argc,char **argv)
{
 
  // rt_kprintf("argc: %d\n",argc);
  // for(int i=0;i<argc;i++) {
  //   rt_kprintf("Arg %d: %s\n",i,argv[i]);
  // }

static volatile rt_uint32_t *crash;

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
       case 'f':
         {
           FILE * f=fopen("/test.txt","w+");
           if (f) {
             fprintf(f,"This is a file content\n");
             fclose(f);
           } else {
             rt_kprintf("fopen failed\n");
           }
         }   
       case 's':          
          sendMessage(argc>=3?argv[2]:"default message");
          break;
       case 't':
          if (!timer) {
            int timeout;
            if (argc<3 || sscanf(argv[2],"%d",&timeout)!=1) timeout=10000;
            
            timer=rt_timer_create("tim01",messageTimerCallBack,NULL,rt_tick_from_millisecond(timeout),RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
            RT_ASSERT(timer);
            rt_kprintf("Timer %lx created %d ms\n",timer,timeout);
            rt_timer_start(timer);           
          } else {  
            rt_timer_delete(timer);
            timer=NULL;
          }
          break;
        case 'r':
          if (argc>=3) {
            sscanf(argv[2],"%x",&crash);
            rt_kprintf("%x\n",*crash); 
          }
           
         
        
                    
     }
  }
}

MSH_CMD_EXPORT(test,Usage: test m or test t)
#endif

#ifndef RT_USING_FINSH

void sender()
{
    while(1) {
      rt_kprintf("thread %s\n",rt_thread_self()->name);
      rt_thread_mdelay(1000);
      messageTimerCallBack();
    }
   

}

#endif

int main() {

char * message;

  mb = rt_mb_create("mb01",1,RT_IPC_FLAG_FIFO);
  RT_ASSERT(mb);
  
  #ifndef RT_USING_FINSH
  rt_thread_t t2 = rt_thread_create("Sender",sender,NULL,2048,16,100);
  RT_ASSERT(t2);
  rt_thread_startup(t2);
  #endif

  while (1) {
    //t_kprintf("thread %s\n",rt_thread_self()->name);
    //rt_thread_mdelay(1000);
    rt_err_t err=rt_mb_recv(mb,(rt_ubase_t *)&message,RT_WAITING_FOREVER);
    if (err==RT_EOK) {
      rt_kprintf("thread %s received message: %s\n",rt_thread_self()->name,message);
      free(message);
    }
  }

}