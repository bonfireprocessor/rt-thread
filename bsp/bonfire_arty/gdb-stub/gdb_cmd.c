#include "bonfire.h"
#include "riscv-gdb-stub.h"
#include "rtthread.h"
#include "shell.h"
#include "console.h"

void BonfireHandleTrap(trapframe_t * t)
{
  
   handle_exception(t);
}

static void gdbstub(int argc, char** argv)
{
  if (argc>=2) {
     switch (argv[1][0]) {
       case 'i':
          gdb_setup_interface(500000);
          rt_kprintf("Start gdb on serial port\n");
          gdb_breakpoint();
          break;
       case 'd':
          gdb_breakpoint();
          break;     
     }
  }    
}

MSH_CMD_EXPORT(gdbstub, start integrated gdbserver);
