#ifndef __RISCV_GDB_STUB_H__
#define __RISCV_GDB_STUB_H__

#include "trapframe.h"

typedef  trapframe_t* (*t_ptrapfuntion)(trapframe_t*);

t_ptrapfuntion gdb_initDebugger(int set_mtvec);


void gdb_setup_interface(int baudrate);

trapframe_t* handle_exception (trapframe_t *ptf);


/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger. */

inline void gdb_breakpoint (void)
{
  asm("sbreak");

}


#endif
