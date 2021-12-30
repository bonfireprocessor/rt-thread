#ifndef __UART_H_
#define __UART_H_

#include <stdint.h>

void uart_writechar(char c);
char uart_readchar();
int uart_wait_receive(long timeout);
void uart_setDivisor(uint32_t divisor);
uint32_t uart_getDivisor();
void uart_setBaudRate(int baudrate);
uint32_t uart_getFramingErrors();



#endif
