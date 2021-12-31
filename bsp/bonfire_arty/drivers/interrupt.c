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
#include "trapframe.h"
#include "console.h"


// Copy from cpuport.c

struct rt_hw_stack_frame
{
    rt_ubase_t epc;        /* epc - epc    - program counter                     */
    rt_ubase_t ra;         /* x1  - ra     - return address for jumps            */
    rt_ubase_t mstatus;    /*              - machine status register             */
    rt_ubase_t gp;         /* x3  - gp     - global pointer                      */
    rt_ubase_t tp;         /* x4  - tp     - thread pointer                      */
    rt_ubase_t t0;         /* x5  - t0     - temporary register 0                */
    rt_ubase_t t1;         /* x6  - t1     - temporary register 1                */
    rt_ubase_t t2;         /* x7  - t2     - temporary register 2                */
    rt_ubase_t s0_fp;      /* x8  - s0/fp  - saved register 0 or frame pointer   */
    rt_ubase_t s1;         /* x9  - s1     - saved register 1                    */
    rt_ubase_t a0;         /* x10 - a0     - return value or function argument 0 */
    rt_ubase_t a1;         /* x11 - a1     - return value or function argument 1 */
    rt_ubase_t a2;         /* x12 - a2     - function argument 2                 */
    rt_ubase_t a3;         /* x13 - a3     - function argument 3                 */
    rt_ubase_t a4;         /* x14 - a4     - function argument 4                 */
    rt_ubase_t a5;         /* x15 - a5     - function argument 5                 */
    rt_ubase_t a6;         /* x16 - a6     - function argument 6                 */
    rt_ubase_t a7;         /* x17 - s7     - function argument 7                 */
    rt_ubase_t s2;         /* x18 - s2     - saved register 2                    */
    rt_ubase_t s3;         /* x19 - s3     - saved register 3                    */
    rt_ubase_t s4;         /* x20 - s4     - saved register 4                    */
    rt_ubase_t s5;         /* x21 - s5     - saved register 5                    */
    rt_ubase_t s6;         /* x22 - s6     - saved register 6                    */
    rt_ubase_t s7;         /* x23 - s7     - saved register 7                    */
    rt_ubase_t s8;         /* x24 - s8     - saved register 8                    */
    rt_ubase_t s9;         /* x25 - s9     - saved register 9                    */
    rt_ubase_t s10;        /* x26 - s10    - saved register 10                   */
    rt_ubase_t s11;        /* x27 - s11    - saved register 11                   */
    rt_ubase_t t3;         /* x28 - t3     - temporary register 3                */
    rt_ubase_t t4;         /* x29 - t4     - temporary register 4                */
    rt_ubase_t t5;         /* x30 - t5     - temporary register 5                */
    rt_ubase_t t6;         /* x31 - t6     - temporary register 6                */
#ifdef ARCH_RISCV_FPU
    rv_floatreg_t f0;      /* f0  */
    rv_floatreg_t f1;      /* f1  */
    rv_floatreg_t f2;      /* f2  */
    rv_floatreg_t f3;      /* f3  */
    rv_floatreg_t f4;      /* f4  */
    rv_floatreg_t f5;      /* f5  */
    rv_floatreg_t f6;      /* f6  */
    rv_floatreg_t f7;      /* f7  */
    rv_floatreg_t f8;      /* f8  */
    rv_floatreg_t f9;      /* f9  */
    rv_floatreg_t f10;     /* f10 */
    rv_floatreg_t f11;     /* f11 */
    rv_floatreg_t f12;     /* f12 */
    rv_floatreg_t f13;     /* f13 */
    rv_floatreg_t f14;     /* f14 */
    rv_floatreg_t f15;     /* f15 */
    rv_floatreg_t f16;     /* f16 */
    rv_floatreg_t f17;     /* f17 */
    rv_floatreg_t f18;     /* f18 */
    rv_floatreg_t f19;     /* f19 */
    rv_floatreg_t f20;     /* f20 */
    rv_floatreg_t f21;     /* f21 */
    rv_floatreg_t f22;     /* f22 */
    rv_floatreg_t f23;     /* f23 */
    rv_floatreg_t f24;     /* f24 */
    rv_floatreg_t f25;     /* f25 */
    rv_floatreg_t f26;     /* f26 */
    rv_floatreg_t f27;     /* f27 */
    rv_floatreg_t f28;     /* f28 */
    rv_floatreg_t f29;     /* f29 */
    rv_floatreg_t f30;     /* f30 */
    rv_floatreg_t f31;     /* f31 */
#endif
};



/*  interrupt handler tables */

static uint32_t irq_map_table[] =
{
  MIP_MEIP,  // EXT_IRQ
  UART0_INTNUM,
  UART1_INTNUM
};

#define MAX_HANDLERS  (sizeof(irq_map_table)/sizeof(uint32_t))

static struct rt_irq_desc irq_desc[MAX_HANDLERS];

static volatile uint32_t *pmtime = (uint32_t*)MTIME_BASE; // Pointer to memory mapped RISC-V Timer registers
static uint32_t tick_interval=0;

// Determine bit mask for MIP/MIE registers
uint32_t vector_tobitmask(int irq)
{
  RT_ASSERT(irq<MAX_HANDLERS);
  return 1 << irq_map_table[irq];

}

/**
 * This function will un-mask a interrupt.
 * @param vector the interrupt number
 */
void rt_hw_interrupt_unmask(int vector)
{
  set_csr(mie,vector_tobitmask(vector));

}

/**
 * This function will mask a interrupt.
 * @param vector the interrupt number
 */
void rt_hw_interrupt_mask(int vector)
{
   clear_csr(mie,vector_tobitmask(vector));
}


rt_isr_handler_t rt_hw_interrupt_handle(rt_uint32_t vector, void *param)
{
    rt_kprintf("UN-handled interrupt %d occurred!!!\n", vector);
    return RT_NULL;
}


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


void SystemIrqHandler(uint32_t mcause,uint32_t mepc, uint32_t mstatus, struct rt_hw_stack_frame *f)
{
     RT_ASSERT(mcause & 0x80000000);
     switch (mcause & 0x0ff) {
        case 0x07:
         //BOARD_DEBUG("Timer irq @%ld\n",pmtime[0]);
          pmtime[2]=pmtime[0]+tick_interval;  // Will as side effect clear the pending irq
          rt_tick_increase();
          break;
        case 0x0b: // EXT IRQ
          irq_desc[0].handler(0,irq_desc[0].param);
          break;
        case UART0_INTNUM:
          //rt_kprintf("UART0 Irq");
          irq_desc[1].handler(1, irq_desc[1].param);
          break;
        case UART1_INTNUM:
          irq_desc[2].handler(2,irq_desc[2].param);
          break;
      default:
        BOARD_DEBUG("Unexpeced interupt %lx\n",mcause);
     }
}



RT_WEAK void BonfireHandleTrap(trapframe_t * t)
{
   BOARD_DEBUG("Trap Exception %lx\n",t->cause);
   dump_tf(t);
   rt_hw_cpu_shutdown();

}

void SystemTrapHandler(uint32_t mcause,uint32_t mepc, uint32_t mstatus, struct rt_hw_stack_frame *f)
{
trapframe_t t;

    RT_ASSERT(!(mcause & 0x80000000));
    t.status = mstatus;
    t.epc = mepc;
    t.cause = mcause;
    t.insn = *((uint32_t*)mepc);
    t.gpr[0]=0;
    t.gpr[1]=f->ra;
    t.gpr[2]=(uint32_t)f+sizeof(struct rt_hw_stack_frame);
    t.badvaddr=read_csr(mbadaddr);

    // copy x3..x31
    uint32_t *sr = (uint32_t*)&f->gp;
    for(int i=3;i<32;i++) {
      t.gpr[i]=*sr++;
    }
    BonfireHandleTrap(&t);
    f->epc=t.epc;
    f->mstatus=t.status;
    f->ra=t.gpr[1];
    sr = (uint32_t*)&f->gp;
    for(int i=3;i<32;i++) {
      *sr++=t.gpr[i];
    }


}


void rt_hw_interrupt_init(void)
{
    int idx;

    rt_kprintf("rt_hw_interrupt_init\n");
    /* init exceptions table */
    for (idx = 0; idx < MAX_HANDLERS; idx++)
    {
        rt_hw_interrupt_mask(idx);
        irq_desc[idx].handler = (rt_isr_handler_t)rt_hw_interrupt_handle;
        irq_desc[idx].param = RT_NULL;
#ifdef RT_USING_INTERRUPT_INFO
        rt_snprintf(irq_desc[idx].name, RT_NAME_MAX - 1, "default");
        irq_desc[idx].counter = 0;
#endif
    }// enable machine external interrupt
}

/**
 * This function will install a interrupt service routine to a interrupt.
 * @param vector the interrupt number
 * @param handler the interrupt service routine to be installed
 * @param param the interrupt service function parameter
 * @param name the interrupt name
 * @return old handler
 */
rt_isr_handler_t rt_hw_interrupt_install(int vector, rt_isr_handler_t handler,
        void *param, const char *name)
{
    rt_isr_handler_t old_handler = RT_NULL;

    if(vector < MAX_HANDLERS)
    {
        old_handler = irq_desc[vector].handler;
        if (handler != RT_NULL)
        {
            irq_desc[vector].handler = (rt_isr_handler_t)handler;
            irq_desc[vector].param = param;
#ifdef RT_USING_INTERRUPT_INFO
            rt_snprintf(irq_desc[vector].name, RT_NAME_MAX - 1, "%s", name);
            irq_desc[vector].counter = 0;
#endif
        }
    }

    return old_handler;
}

