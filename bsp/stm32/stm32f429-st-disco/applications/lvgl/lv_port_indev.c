/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-10-18     Meco Man     The first version
 */

#include <rtthread.h>

#ifdef PKG_USING_LVGL
#include <lvgl.h>
#include <stdbool.h>
#include <rtdevice.h>

#include <lcd_port.h>
#include <drv_touch.h>

static lv_indev_state_t last_state = LV_INDEV_STATE_REL;
static rt_int16_t last_x = 0;
static rt_int16_t last_y = 0;



static void input_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
struct touch_state ts;

    touch_get_state(&ts);

    data->point.x = (3706 - ts.x) / 14;
    data->point.y = LCD_HEIGHT - (-461 + ts.y) / 10.5 - 20;
    data->state = ts.pressed?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
}

// void lv_port_indev_input(rt_int16_t x, rt_int16_t y, lv_indev_state_t state)
// {
//     last_state = state;
//     last_x = x;
//     last_y = LCD_HEIGHT - y;
// }

lv_indev_t * button_indev;

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;



    lv_indev_drv_init(&indev_drv); /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = input_read;

    /*Register the driver in LVGL and save the created input device object*/
    button_indev = lv_indev_drv_register(&indev_drv);
}

#endif