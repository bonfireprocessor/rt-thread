/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 * Copyright (c) 2021,2022 Bonfire Project, Thomas Hornschuh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 16.01.2022     Thomas Hornschuh initial version
 */


#include <rtthread.h>
#include <rtdevice.h>

#ifdef RT_USING_LWIP
#include <netif/ethernetif.h>
#include "bonfire.h"
#include "mem_rw.h"
#include "interrupt.h"
#include "xil_etherlite.h"
#include "board.h"

#define ETH_BUFSIZE_WORDS (0x07f0/4)

// Buffer Offsets
#define O_ETHERTYPE 12
#define O_PAYLOAD 14 // Begin of Payload
#define O_IP_LENGTH (O_PAYLOAD+2)

// IP Constants
#define ARP_LENGTH 26 // Size of ARP packet
#define ETYPE_IP 0x800
#define ETYPE_ARP 0x806

#define MAX_ADDR_LEN 6

// Supress Debug Output
#undef BOARD_DEBUG
#define BOARD_DEBUG(...) 

struct xe_net_device
{
    /* inherit from ethernet device */
    struct eth_device parent;

    /* interface address info. */
    rt_uint8_t  dev_addr[MAX_ADDR_LEN]; /* hw address   */  
};

static struct xe_net_device eth0 = {
    .dev_addr =  {0,0, 0x5e,0,0x0fa,0x0ce }
};

inline void _write_leds(uint8_t value)
{
   _write_word(( void* )ARTY_LEDS4TO7,value);
}

inline uint8_t _read_leds()
{
   return _read_word(( void* )ARTY_LEDS4TO7);
}


static void xe_isr(int vector, void *param)
{
    if (_read_word((void*)BONFIRE_SYSIO) & 0x01) { // Pending IRQ
      _set_bit((void*)ARTY_LEDS4TO7,0); // light LED4     
      eth_device_ready(&eth0.parent);    
      _write_word((void*)BONFIRE_SYSIO,0x01); // clear IRQ
      _clear_bit((void*)ARTY_LEDS4TO7,0);

    } else
     rt_kprintf("Uups, ethernet irq handler called without pending IRQ\n");
}


/* control the interface */
static rt_err_t xe_control(rt_device_t dev, int cmd, void *args)
{
    struct xe_net_device *nd = (struct xe_net_device *)dev;    
    switch (cmd) {
    case NIOCTL_GADDR:
        /* get mac address */
        if (args) rt_memcpy(args, nd->dev_addr, 6);
        else return -RT_ERROR;
        break;

    default :
        break;
    }

    return RT_EOK;
}

/* Open the ethernet interface */
static rt_err_t xe_open(rt_device_t dev, uint16_t oflag)
{
    return RT_EOK;
}

/* Close the interface */
static rt_err_t xe_close(rt_device_t dev)
{
    return RT_EOK;
}

/* Read */
static rt_size_t xe_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    rt_set_errno(-RT_ENOSYS);
    return RT_EOK;
}

/* Write */
static rt_size_t xe_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    rt_set_errno(-RT_ENOSYS);
    return 0;
}

// Copy dwords between Buffers. Because the Etherlite core don't
// support byte writes, the code ensures to do only 32 Bit transfers
// Attentions: Because it rounds up, it can transfer up to three bytes more
// then specified in size
static void eth_copy_buffer(void* dest, const void* src, size_t size)
{
const uint32_t *psrc = src;
uint32_t *pdest = dest;

size_t szwords =(size % 4)?size/4+1:size/4; // round up
int i;
    
    RT_ASSERT(szwords<=ETH_BUFSIZE_WORDS);
    RT_ASSERT((uintptr_t)src % 4 ==0); // Check alignment
    
    for(i=0;i<szwords;i++)
       pdest[i]=psrc[i];
}

static rt_err_t xe_tx(rt_device_t dev, struct pbuf *p)
{
static uint32_t aligned_buffer[ETH_BUFSIZE_WORDS];

   rt_base_t level=rt_hw_interrupt_disable(); 
   RT_ASSERT(p); 
   RT_ASSERT(p->tot_len<=ETH_BUFSIZE_WORDS*4);
   void *dest = (void *)aligned_buffer;
   for (struct pbuf *q=p; q != RT_NULL; q = q->next) {
       memcpy(dest,q->payload,q->len);
       dest+= q->len;
   }

   while (_read_word(ETHL_TX_PING_CTRL) & 0x01); // Wait until buffer ready 
   eth_copy_buffer(ETHL_TX_PING_BUFF,(void*)aligned_buffer,p->tot_len);
   _write_word(ETHL_TX_PING_LEN,p->tot_len);
   _write_word(ETHL_TX_PING_CTRL,0x01); // Start send

   rt_hw_interrupt_enable(level);

   return RT_EOK;

}


static  inline rt_uint16_t get_nbo_word(uintptr_t buff,int offset )
{
  rt_uint8_t *b = (rt_uint8_t*)buff;
  return b[offset+1] | (b[offset] << 8);
}

static inline rt_bool_t isFull( uintptr_t buff )
{
  return _read_word( (void*)( buff+ETHL_OFFSET_CTRL )) & 0x01;
}


static struct pbuf *xe_rx(rt_device_t dev)
{
static uintptr_t currentBuff = (uintptr_t) ETHL_RX_PING_BUFF; // start always with the ping buffer
int length;
struct pbuf *p = RT_NULL;


  rt_base_t level=rt_hw_interrupt_disable(); 
  // Check if buffers are out of sync...
  if (!isFull(currentBuff) && isFull(currentBuff ^ PONG_BUFF_OFFSET)) {
    //dbg("Etherlite buffers out of sync, correcting...\n");
    currentBuff ^= PONG_BUFF_OFFSET;
  }

  if (isFull(currentBuff)) {
     //dbg("Ethernet Buffer %d used\n",(currentBuff & PONG_BUFF_OFFSET)?1:0);
     uint8_t led = _read_leds() & 0x3; // Read leds and clear upper two (bit 3,bit 2)
     if (currentBuff & PONG_BUFF_OFFSET)
       _write_leds( (0x01<<3) | led ); // light LED7
     else
       _write_leds( (0x01<<2) | led ); // light LED6

    // Caclucate frame size
    uint16_t ethertype = get_nbo_word(currentBuff,O_ETHERTYPE);
    BOARD_DEBUG("Ethertype %x\n",ethertype);
    switch (ethertype) {
      case ETYPE_IP:
        length=get_nbo_word(currentBuff,O_IP_LENGTH) + O_PAYLOAD + 4;
        break;
      case ETYPE_ARP:
        length=O_PAYLOAD+ARP_LENGTH+4;
        break;
      default:
        length=0; // Ignore all non ARP and IP packets for now 
        //length=ethertype>MAX_FRAME?MAX_FRAME:ethertype;
    }
    //BOARD_DEBUG("Ethernet frame length %d\n",length);
    if (length) {
         BOARD_DEBUG("First 16 Bytes of Ethernet Frame:\n");
         for(int i=0;i<16;i++) BOARD_DEBUG("%x ",((uint8_t*)currentBuff)[i]);
         BOARD_DEBUG("\n");
         if (ethertype==ETYPE_IP) {
           BOARD_DEBUG("First 16 Bytes of IP Packet:\n");
           for(int i=0;i<16;i++) BOARD_DEBUG("%x ",((uint8_t*)currentBuff+O_PAYLOAD)[i]);
           BOARD_DEBUG("\n"); 
         }
         p = pbuf_alloc(PBUF_LINK, length, PBUF_POOL);
         if (p) {
              RT_ASSERT(p->tot_len == length); 
              void * src = (void*)currentBuff;  
              for (struct pbuf *q = p; q != RT_NULL; q = q->next) {
                 memcpy(q->payload,(void*)src,q->len);                
                 src+=q->len;
              }     
         } else {
             rt_kprintf("xiletherlite: pbuf alloc failed\n");             
         }
    }
    
    _write_word((void*)currentBuff+ETHL_OFFSET_CTRL,0x8); // clear buffer, enable interrupts
    
     currentBuff ^= PONG_BUFF_OFFSET;
  }
  rt_hw_interrupt_enable(level); 
 
  return p;
}


static rt_err_t xe_init(rt_device_t dev)
{
    rt_kprintf("Initalizing Ethernet core\n");
    
    rt_hw_interrupt_install(ETHL_INT_VECTOR,xe_isr,
                        (void*) &eth0.parent,"eth0 irq");
    rt_hw_interrupt_unmask(ETHL_INT_VECTOR);

    // clear pending packets, enable receive interrupts
    _write_word(ETHL_RX_PING_CTRL,0x8);
    _write_word(ETHL_RX_PONG_CTRL,0x0);
    _write_word(ETHL_TX_PING_CTRL,0);
    _write_word(ETHL_GIE,0x80000000); // Enable Ethernet Interrupts
    rt_kprintf("xe_init finished\n");
    return RT_EOK;
  
} 

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops xe_ops =
{
    xe_init,
    xe_open,
    xe_close,
    xe_read,
    xe_write,
    xe_control
};
#endif



static int  rt_hw_xe_init()
{
    eth0.parent.parent.type    = RT_Device_Class_NetIf;

#ifdef RT_USING_DEVICE_OPS
    eth0.parent.parent.ops     = &xe_ops;
#else
    eth0.parent.parent.init    = xe_init;
    eth0.parent.parent.open    = xe_open;
    eth0.parent.parent.close   = xe_close;
    eth0.parent.parent.read    = xe_read;
    eth0.parent.parent.write   = xe_write;
    eth0.parent.parent.control = xe_control;
#endif  

     /* init rt-thread ethernet device struct */
    eth0.parent.eth_rx  = xe_rx;
    eth0.parent.eth_tx  = xe_tx;
   
    eth_device_init(&(eth0.parent), "e0");
    eth_device_linkchange(&(eth0.parent),RT_TRUE);
    rt_kprintf("rt_hw_xe_init finished\n");
    return 0;
}

INIT_ENV_EXPORT(rt_hw_xe_init);

#endif