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



#if defined(RT_USING_DEVICE) && defined(RT_USING_SPI)
#pragma message "Xilinx SPI driver"


static rt_err_t xs_configure(struct rt_spi_device *device, struct rt_spi_configuration *configuration)
{
    BOARD_DEBUG("xs_configure\n");
    return RT_EOK;
}
static rt_uint32_t xs_xfer(struct rt_spi_device *device, struct rt_spi_message *message)
{
    BOARD_DEBUG("xs_xfer\n");
    return 0;

}

static struct rt_spi_ops ops = {
   xs_configure,
   xs_xfer
};


static struct rt_spi_bus bus0;
static struct rt_spi_device xil_spi;

const char busname[] = "spibus0";

int rt_hw_xilspi_init(void)
{
rt_err_t err;

    err=rt_spi_bus_register(&bus0,busname,&ops);
    RT_ASSERT(err==RT_EOK);
    err=rt_spi_bus_attach_device(&xil_spi,"spi0",busname,NULL);
    RT_ASSERT(err==RT_EOK);

    msd_init("sd0","spi0");

}

INIT_BOARD_EXPORT(rt_hw_xilspi_init);

#endif