#ifndef __TRAPFRAME_H_
#define __TRAPFRAME_H_

// RISC-V Trapframe


typedef struct
{
  uint32_t gpr[32];
  uint32_t status;
  uint32_t epc;
  uint32_t badvaddr;
  uint32_t cause;
  uint32_t insn;
} trapframe_t;

#endif