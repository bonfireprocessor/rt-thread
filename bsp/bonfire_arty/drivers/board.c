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
#include "interrupt.h"
#include "riscv-gdb-stub.h"



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
     write_csr(mie,0); // Disable all interrupts
     rt_hw_interrupt_disable();

     // Jump to Firmware
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
#ifdef BOOT_DEBUG    
     gdbstub_init_debug();
#endif

    rt_assert_set_hook(assert_handler);
    mtime_setinterval( ((long)(SYSCLK/RT_TICK_PER_SECOND)));
    rt_hw_interrupt_init();
     //rt_hw_uart_init();
     
#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif     

    /* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
    BOARD_DEBUG("invoking rt_components_board_init\n");
    rt_components_board_init();
   
#endif



#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
    BOARD_DEBUG("Calling rt_console_set_device with %s\n",RT_CONSOLE_DEVICE_NAME);
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
  return uart_readchar();
}
#endif

#endif