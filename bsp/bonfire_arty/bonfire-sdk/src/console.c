// "Borrowed" from RISC-V proxy kernel
// See LICENSE for license details.



#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include <stdlib.h>
#include <ctype.h>
#include "uart.h"
#include "console.h"



static void writestr(char *p)
{
  while (*p) {
    uart_writechar(*p);
    p++;
  }
}


// Like Writestr but expands \n to \n\r
static void write_console(char *p)
{
   while (*p) {
    if (*p=='\n') uart_writechar('\r');
    uart_writechar(*p);
    p++;
  }

}

void vprintk(const char* s, va_list vl)
{
char out[256];

  vsnprintf(out, sizeof(out), s, vl);
  write_console(out);
}

void printk(const char* s, ...)
{
  va_list vl;
  va_start(vl, s);

  vprintk(s, vl);

  va_end(vl);
}

void dump_tf(trapframe_t* tf)
{
  static const char*  regnames[] = {
    "z ", "ra", "sp", "gp", "tp", "t0",  "t1",  "t2",
    "s0", "s1", "a0", "a1", "a2", "a3",  "a4",  "a5",
    "a6", "a7", "s2", "s3", "s4", "s5",  "s6",  "s7",
    "s8", "s9", "sa", "sb", "t3", "t4",  "t5",  "t6"
  };

  tf->gpr[0] = 0;

  for(int i = 0; i < 32; i+=4)
  {
    for(int j = 0; j < 4; j++)
      printk("%s %08lx%c",regnames[i+j],tf->gpr[i+j],j < 3 ? ' ' : '\n');
  }
  printk("pc %08lx va %08lx op %08lx sr %08lx\n\n\n", tf->epc, tf->badvaddr,
         (uint32_t)tf->insn, tf->status);
}

void do_panic(const char* s, ...)
{
  va_list vl;
  va_start(vl, s);

  vprintk(s, vl);
  while(1); // endless loop

}

void kassert_fail(const char* s)
{
  register uintptr_t ra asm ("ra");
  do_panic("assertion failed @ %p: %s\n", ra, s);
}



int readBuffer(char *b,int sz)
{
char c;
char *p;

   p=b;
   c=uart_readchar();
   while (c!='\r') {

      if (c==8) {// backspace
        if (p>b) {
          p--;
          writestr("\b \b");
        }
      } else if ( p<(b+sz-1) ) {
          *p++=c;
          uart_writechar(c); // echo
      }
      else
        uart_writechar('\a'); // beep

      c=uart_readchar();

   }
   *p='\0';
   write_console("\n");
   return p-b; // len
}

void hex_dump(void *mem,int numWords)
{
uint32_t *pmem = mem;
int i;

    for(i=0;i<numWords;i++) {
      if ((i % 4)==0) { // Write Memory address for every four words
        printk("\n%lx    ",(uint32_t)&pmem[i]);
      }
      printk("%lx ",pmem[i]);
      //writeHex(pmem[i]);
    }
}
