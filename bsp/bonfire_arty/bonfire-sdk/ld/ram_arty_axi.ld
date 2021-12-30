
__load = 0x10000;        
__top  = 0x0ffffff0; /* must be aligned to a 16 Byte boundary !!! */
__stacksize = 64k;
__irq_stacksize = 4k;
__trap_stacksize = 4k;

INCLUDE ram_generic.ld