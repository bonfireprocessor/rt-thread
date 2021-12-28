/*
 * Copyright (c) 2021 Thomas Hornschuh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 
 */

#include <rthw.h>
#include <rtthread.h>
#include "bonfire.h"
#include "uart.h"
//#include <reent.h>
#include "board.h"
//#include <malloc.h>


static volatile uint32_t *pmtime = (uint32_t*)MTIME_BASE; // Pointer to memory mapped RISC-V Timer registers
static uint32_t tick_interval=0;


static uint32_t mtime_setinterval(uint32_t interval)
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

void rt_os_tick_callback(void)
{
    rt_interrupt_enter();
    
    rt_tick_increase();

    rt_interrupt_leave();
}

void SystemIrqHandler(uint32_t mcause,uint32_t mepc,void *trapframe)
{
    if (mcause & 0x80000000) {
       // interrupt
       switch (mcause & 0x0ff) {
         case 0x07:
           //BOARD_DEBUG("Timer irq @%ld\n",pmtime[0]);
           pmtime[2]=pmtime[0]+tick_interval;  // Will as side effect clear the pending irq
           rt_os_tick_callback();
           break;
        default:
          BOARD_DEBUG("Unexpeced interupt %lx\n",mcause);    
       }  
    }  else {
        BOARD_DEBUG("Trap Exception %lx at %lx\n",mcause,mepc);
        uart_readchar();        
        rt_hw_cpu_shutdown();
    }

}


#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)

//static void * rt_heap = NULL;
extern int  end;              /* start of free memory (as symbol) */
extern int  _endofheap;

           /* end of free memory */
static void *heap_ptr = (void *)&end;         /* Previous end */

RT_WEAK void *rt_heap_begin_get(void)
{
    // if (!rt_heap) rt_heap = malloc(RT_HEAP_SIZE);
    // RT_ASSERT(rt_heap!=NULL);
    BOARD_DEBUG("Allocated rt_heap at %lx, size %ld\n",heap_ptr,(void*)&_endofheap - heap_ptr );

    return heap_ptr;
}

RT_WEAK void *rt_heap_end_get(void)
{ 
  //  RT_ASSERT(rt_heap!=NULL);
//     return rt_heap + RT_HEAP_SIZE;
    return (void*)&_endofheap;;
}
#endif



void rt_hw_cpu_shutdown()
{
     clear_csr(mie,MIP_MTIP); // Disable Timer Interrupt
     rt_hw_interrupt_disable();

     void (*sram_base)() = (void*)SRAM_BASE;
     sram_base();
}

static void assert_handler(const char *ex_string, const char *func, rt_size_t line)
{
   rt_kprintf("(%s) assertion failed at function:%s, line number:%d \n", ex_string, func, line);
   rt_hw_cpu_shutdown();         
} 


/**
 * This function will init your board.
 */
void rt_hw_board_init(void)
{

    rt_assert_set_hook(assert_handler);
    mtime_setinterval( ((long)(SYSCLK/RT_TICK_PER_SECOND)));

    /* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
    BOARD_DEBUG("invoking rt_components_board_init\n");
    rt_components_board_init();
#endif

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

}


#ifdef RT_USING_CONSOLE

static int uart_init(void)
{

    uart_setBaudRate(PLATFORM_BAUDRATE);
    return 0;
}
INIT_BOARD_EXPORT(uart_init);


void rt_hw_console_output(const char *str)
{
// Output the string 'str' through the uart."
   while (*str) {
     if (*str=='\n') uart_writechar('\r');
     uart_writechar(*str++);      
   }    
}

#ifndef RT_USING_DEVICE
char rt_hw_console_getchar(void)
{
  return uart_wait_receive(0);
}
#endif


#endif

