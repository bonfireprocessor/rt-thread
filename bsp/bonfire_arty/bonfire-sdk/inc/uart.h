#ifndef __UART_H_
#define __UART_H_

#include <stdint.h>

#define UART_TXRX 0
#define UART_STATUS 1
#define UART_EXT_CONTROL 2
#define UART_INT_REGISTER 3

#define BIT_RX_INT_ENABLE 0
#define BIT_RX_INT_PENDING 16

#define BIT_TX_INT_ENABLE 1
#define BIT_TX_INT_PENDING 17

#define BIT_FIFO_INT_PENDING 19
#define BIT_FIFO_INT_ENABLE 3


void uart_writechar(char c);
char uart_readchar();
int uart_wait_receive(long timeout);
void uart_setDivisor(uint32_t divisor);
uint32_t uart_getDivisor();
void uart_setBaudRate(int baudrate);
uint32_t uart_getFramingErrors();



#endif
