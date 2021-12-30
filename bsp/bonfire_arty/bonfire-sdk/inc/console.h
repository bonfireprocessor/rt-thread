#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "trapframe.h"


void printk(const char* s, ...);
void vprintk(const char* s, va_list vl);
void dump_tf(trapframe_t* tf);
void do_panic(const char* s, ...);
void kassert_fail(const char* s);
int readBuffer(char *b,int sz);
void hex_dump(void *mem,int numWords);


#endif
