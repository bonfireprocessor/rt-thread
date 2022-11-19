/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 * Copyright (c) 2021,2022 Bonfire Project, Thomas Hornschuh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 01.01.2022     Thomas Hornschuh initial version
 */

#include <rthw.h>
#include <rtthread.h>

#include <stddef.h>
#include <rtdevice.h>
#include "bonfire.h"

#include "uart.h"
#include "interrupt.h"
#include "board.h"

#if defined(RT_USING_DEVICE) && defined(RT_USING_SERIAL)

#define UART0_VECTOR 1

#define NUM_UART 1 // temporarly

#define PENDING_MASK ((1<<BIT_RX_INT_PENDING)|(1<<BIT_TX_INT_PENDING)|(1<<BIT_FIFO_INT_PENDING))

uint32_t* get_uart_base(unsigned id) {

  if (id>NUM_UART-1) return 0;
  switch(id) {
    case 0:
      return (uint32_t*)UART0_BASE;
    case 1:
      return (uint32_t*)UART1_BASE;
    default:
     return 0;
  }
}

static void usart_handler(int vector, void *param)
{
    if (vector==1) {
       volatile uint32_t *uart_adr = get_uart_base(0);
       RT_ASSERT(uart_adr!=NULL);
       // Only call isr when there is really something in UART fifo 
       // otherwise the serial handler can crash when there is a spurious interrupt without actual data.
       uint32_t status=uart_adr[UART_STATUS];
       if  (status & 0x01) 
             rt_hw_serial_isr((struct rt_serial_device *)param, RT_SERIAL_EVENT_RX_IND);
        else
          BOARD_DEBUG("spurious UART-RX irq\n");       

       // Clear pending flag and keep RX IRQ enabled 
       uart_adr[UART_INT_REGISTER] = (1<<BIT_RX_INT_PENDING)| (1<< BIT_RX_INT_ENABLE);
    }
}

static rt_err_t usart_configure(struct rt_serial_device *serial,
                                struct serial_configure *cfg)
{
volatile uint32_t *uart_adr = get_uart_base(0);

    RT_ASSERT(uart_adr!=RT_NULL);
    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);
    uart_adr[UART_EXT_CONTROL]= 0x030000L |  (uint16_t)(SYSCLK / cfg->baud_rate -1);
    uart_adr[UART_INT_REGISTER] = PENDING_MASK; // clear all pending interrupts and disable interrupts
    


    return RT_EOK;
}

static rt_err_t usart_control(struct rt_serial_device *serial,
                              int cmd, void *arg)
{
    volatile uint32_t *uart_adr = get_uart_base(0);
    RT_ASSERT(serial != RT_NULL);
    switch (cmd)
    {
        case RT_DEVICE_CTRL_CLR_INT:
            uart_adr[UART_INT_REGISTER] = PENDING_MASK;
            rt_hw_interrupt_mask(UART0_VECTOR);
            break;
        case RT_DEVICE_CTRL_SET_INT:
            uart_adr[UART_INT_REGISTER] = PENDING_MASK | 1<< BIT_RX_INT_ENABLE;
            rt_hw_interrupt_unmask(UART0_VECTOR);
            break;
    }

    return RT_EOK;
}

static int usart_putc(struct rt_serial_device *serial, char c)
{
volatile uint32_t *uartadr=get_uart_base(0);

    RT_ASSERT(uartadr!=NULL);
    while (!(uartadr[UART_STATUS] & 0x2)); // Wait until transmitter ready
    uartadr[UART_TXRX]=(uint32_t)(c & 0x0ff);
    return 0;
}


static int usart_getc(struct rt_serial_device *serial)
{
volatile uint32_t *uart_adr=get_uart_base(0);


    RT_ASSERT(uart_adr!=RT_NULL);
    uint32_t status=uart_adr[UART_STATUS];
    if  (status & 0x01) { // receive buffer not empty?
       return uart_adr[UART_TXRX] & 0xff;      
    } else
       return -1;
}

static struct rt_uart_ops ops =
{
    usart_configure,
    usart_control,
    usart_putc,
    usart_getc,
};

static struct rt_serial_device serial =
{
    .ops = &ops,
    .config.baud_rate = 500000,
    .config.bit_order = BIT_ORDER_LSB,
    .config.data_bits = DATA_BITS_8,
    .config.parity    = PARITY_NONE,
    .config.stop_bits = STOP_BITS_1,
    .config.invert    = NRZ_NORMAL,
    .config.bufsz     = RT_SERIAL_RB_BUFSZ,
};

int rt_hw_uart_init(void)
{

    rt_hw_serial_register(
        &serial,
        RT_CONSOLE_DEVICE_NAME,
        RT_DEVICE_FLAG_STREAM
        | RT_DEVICE_FLAG_RDWR|RT_DEVICE_FLAG_INT_RX,
         RT_NULL);

    rt_hw_interrupt_install(
        UART0_VECTOR, // UART0 Vector
        usart_handler,
        (void *) & (serial.parent),
        "uart0 interrupt");


    return 0;
}

INIT_BOARD_EXPORT(rt_hw_uart_init);

#endif