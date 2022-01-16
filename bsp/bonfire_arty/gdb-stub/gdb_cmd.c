#include "bonfire.h"
#include "riscv-gdb-stub.h"
#include "rtthread.h"
#include "shell.h"
#include "console.h"
#include "board.h"


static rt_bool_t debugger_flag = RT_FALSE;

void BonfireHandleTrap(trapframe_t * t)
{
   if (debugger_flag)
      handle_exception(t);
   else {
      BOARD_DEBUG("Trap Exception %lx\n",t->cause);
      dump_tf(t);
      rt_hw_cpu_shutdown();
   }  
}

void gdbstub_init_debug()
{
  if (!debugger_flag) {
       gdb_setup_interface(500000);
       debugger_flag = RT_TRUE;
       rt_kprintf("Start gdb on serial port\n");
       gdb_breakpoint();
   }
}

static void gdbstub(int argc, char** argv)
{
  if (argc>=2) {
     switch (argv[1][0]) {
       case 'i':
        
          gdbstub_init_debug();
          break;
       case 'd':
          if (debugger_flag)
             gdb_breakpoint();
          else
             rt_kprintf("gdbserver not initalized, use gdbserver i first\n");   
          break;
       case 'x':
         debugger_flag = RT_FALSE;        
     }
  }    
}



MSH_CMD_EXPORT(gdbstub, gdbstub i d or x :  init /break/exit );
