/*
 * Copyright (c) 2021 Thomas Hornschuh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 
 */

#include <rthw.h>
#include <rtthread.h>
#include "bonfire.h"
#include "board.h"


static volatile uint32_t *pmtime = (uint32_t*)MTIME_BASE; // Pointer to memory mapped RISC-V Timer registers
static uint32_t tick_interval=0;


uint32_t mtime_setinterval(uint32_t interval)
{
// Implementation for 32 Bit timer in Bonfire. Need to be adapted in case of a 64Bit Timer

   tick_interval=interval;

   BOARD_DEBUG("Set tick interval to %ld\n",interval);

   if (interval >0) {
     pmtime[2]=pmtime[0]+interval;
     set_csr(mie,MIP_MTIP); // Enable Timer Interrupt
   } else {
     clear_csr(mie,MIP_MTIP); // Disable Timer Interrupt

   }
   return tick_interval;
}


void SystemIrqHandler(uint32_t mcause,uint32_t mepc,void *trapframe)
{
    if (mcause & 0x80000000) {
       // interrupt
       switch (mcause & 0x0ff) {
         case 0x07:
           //BOARD_DEBUG("Timer irq @%ld\n",pmtime[0]);
           pmtime[2]=pmtime[0]+tick_interval;  // Will as side effect clear the pending irq
            rt_tick_increase();
           break;
        default:
          BOARD_DEBUG("Unexpeced interupt %lx\n",mcause);    
       }  
    }  else {
        BOARD_DEBUG("Trap Exception %lx at %lx\n",mcause,mepc); 
        rt_hw_cpu_shutdown();
    }

}
