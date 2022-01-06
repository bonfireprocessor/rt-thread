#include "rtthread.h"
#include "bonfire.h"
#include "mem_rw.h"


static void test_thread()
{
rt_bool_t i=RT_FALSE;
  
  rt_kprintf("Test thread started\n");
  _write_word(( void* )( ARTY_LEDS4TO7+4 ),0); // Set Output Mode
  while(1) { 
   
    if (i)
      _set_bit((void*)ARTY_LEDS4TO7,1);
    else 
      _clear_bit((void*)ARTY_LEDS4TO7,1);  
    i = ! i;
    rt_thread_delay(rt_tick_from_millisecond(500));
  
  }   
 
}

int test_init()
{
   rt_thread_t thread=rt_thread_create("test",test_thread,RT_NULL,4096,1,10);
   RT_ASSERT(thread);
   rt_thread_startup(thread);
}

INIT_ENV_EXPORT(test_init);