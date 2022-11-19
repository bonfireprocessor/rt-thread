/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2021-10-18     Meco Man      First version
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include "lcd_port.h"

#define LV_USE_PERF_MONITOR     1
#define LV_COLOR_DEPTH          LCD_BITS_PER_PIXEL

#define LV_USE_GPU_STM32_DMA2D  1
#define LV_GPU_DMA2D_CMSIS_INCLUDE "stm32f429xx.h"

/* music player demo */
#define LV_HOR_RES_MAX              LCD_WIDTH
#define LV_VER_RES_MAX              LCD_HEIGHT
#define LV_USE_DEMO_RTT_MUSIC       0
#define LV_DEMO_RTT_MUSIC_AUTO_PLAY 0
#define LV_FONT_MONTSERRAT_12       1
#define LV_FONT_MONTSERRAT_16       1


#define LV_USE_DEMO_WIDGETS        1
#if LV_USE_DEMO_WIDGETS
#define LV_DEMO_WIDGETS_SLIDESHOW  1
#endif

/*Demonstrate the usage of encoder and keyboard*/
#define LV_USE_DEMO_KEYPAD_AND_ENCODER     0

/*Benchmark your system*/
#define LV_USE_DEMO_BENCHMARK   0


#endif
