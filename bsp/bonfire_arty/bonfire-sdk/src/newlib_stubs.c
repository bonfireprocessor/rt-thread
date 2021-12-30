
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "uart.h"
#include <unistd.h>
#include "console.h"


#undef errno
extern int  errno;

/* Write to a file.  */
ssize_t _write(int file, const void *ptr, size_t len) {
    if (file != STDOUT_FILENO) {
        errno = ENOSYS;
        return -1;
    }

    const char *bptr = ptr;
    for (size_t i = 0; i < len; ++i)
        uart_writechar(bptr[i]);
    return 0;
}

// extern __typeof(_write) write
//     __attribute__((__weak__, __weak__, __alias__("_write")));


void _exit(int exit_status) {
    while (1);
}

int _isatty(int file) { return (file == STDOUT_FILENO); }


ssize_t _read(int file, void *ptr, size_t len) {

   if (file != STDIN_FILENO) {
        errno = ENOSYS;
        return -1;
    }
   
    int read = 0;
    char * p =(char*)ptr;

    for (; len > 0; --len) {
        *p++ = uart_readchar();
        read++;
    }

    return read;
}

int _fstat(int file, struct stat *st) {
  st->st_mode = S_IFCHR;
  return 0;
}


int _close(int file) {
    errno = ENOSYS;
    return -1;
}

off_t _lseek(int file, off_t ptr, int dir) {
    errno = ENOSYS;
    return -1;
}




void * _sbrk (int nbytes)
{
  /* Symbol defined by linker map */
  extern int  end;              /* start of free memory (as symbol) */
  extern int  _endofheap;


  static void *stack =(void*)&_endofheap;           /* end of free memory */
  static void *heap_ptr = (void *)&end;         /* Previous end */

  //printk("sbrk called with %ld heap_ptr: %lx stack: %lx\n",nbytes,heap_ptr,stack);

  if ((stack - (heap_ptr + nbytes)) > 0 )
    {
      void *base  = heap_ptr;
      heap_ptr   += nbytes;
                
      return  base;
    }
  else
    {
      errno = ENOMEM;
      return  (void *) -1;
    }
}       /* _sbrk () */



int _kill (int pid, int sig)
{
  return ENOENT;
}


int _getpid()
{
  return ENOENT;
}

time_t _gettimeofday()
{
    return 0;
}