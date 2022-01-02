/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 * Copyright (c) 2021,2022 Bonfire Project, Thomas Hornschuh
 *
 *  Dirver for Xilinx SPI Core
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

#include "mem_rw.h"
#include "xspi_l.h"
#include "spi_msd.h"

// Switch on LOOP_BACK mode for debugging
//#define LOOP_BACK (XSP_CR_LOOPBACK_MASK)
// Switch off LOOP_BACK:
#define LOOP_BACK 0


#if defined(RT_USING_DEVICE) && defined(RT_USING_SPI)
//#pragma message "Xilinx SPI driver"

static  void *spi_base = (void*)AXI_QPSI0;
rt_bool_t c_cs = RT_FALSE; // current cs status


/* Low Level SPI functions */



static rt_err_t xs_configure(struct rt_spi_device *device, struct rt_spi_configuration *conf)
{
    
    if (conf->data_width!=8) {
        BOARD_DEBUG("xil_spi invalid data witdh %d\n",conf->data_width);
        return RT_ERROR;
    }
  
    _write_word(spi_base+XSP_SRR_OFFSET,XSP_SRR_RESET_MASK); // RESET Controller
    _write_word(spi_base+XSP_CR_OFFSET,XSP_CR_MANUAL_SS_MASK|
                      (conf->mode & RT_SPI_CPOL?XSP_CR_CLK_POLARITY_MASK:0) |
                      (conf->mode & RT_SPI_CPHA?XSP_CR_CLK_PHASE_MASK:0) |
                      XSP_CR_MASTER_MODE_MASK |
                      XSP_CR_ENABLE_MASK |
                      LOOP_BACK);

    c_cs = RT_FALSE;
    return RT_EOK;
}

static inline uint32_t spi_txrx(struct rt_spi_device *device, uint32_t data )
{

    while (_read_word(spi_base+XSP_SR_OFFSET)&XSP_SR_TX_FULL_MASK); // Wait until TX FIFO has space
    _write_word(spi_base+XSP_DTR_OFFSET,data);
    while (_read_word(spi_base+XSP_SR_OFFSET)&XSP_SR_RX_EMPTY_MASK);
    return _read_word(spi_base+XSP_DRR_OFFSET);

}

static inline void spi_select(struct rt_spi_device *device, int is_select )
{
  _write_word(spi_base + XSP_SSR_OFFSET, is_select?0:1 );
}


static rt_uint32_t xs_xfer(struct rt_spi_device *device, struct rt_spi_message *msg)
{

    //while (msg) {
        rt_uint8_t * s = (rt_uint8_t *)msg->send_buf;
        rt_uint8_t * r = (rt_uint8_t *)msg->recv_buf;
        rt_uint8_t b; 

        if (msg->cs_take) spi_select(device,1);
        for(int i=0;i<msg->length;i++) {
            b=spi_txrx(device,s?*s++:0xff);
            if (r) {
                *r++=b;
            }
        }
        if (msg->cs_release) spi_select(device,0);
       
    
    return RT_EOK; // ??

}

static struct rt_spi_ops ops = {
   xs_configure,
   xs_xfer
};


static struct rt_spi_bus bus0;
static struct rt_spi_device xil_spi;

const char busname[] = "spibus0";
const char spi0[] = "spi0";
const char sd0[] = "sd0";

int rt_hw_xilspi_init(void)
{
rt_err_t err;

    err=rt_spi_bus_register(&bus0,busname,&ops);
    RT_ASSERT(err==RT_EOK);
    err=rt_spi_bus_attach_device(&xil_spi,spi0,busname,NULL);
    RT_ASSERT(err==RT_EOK);

    msd_init(sd0,spi0);

}

INIT_BOARD_EXPORT(rt_hw_xilspi_init);

#endif