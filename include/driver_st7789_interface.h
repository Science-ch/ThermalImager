/**
 * Copyright (c) 2015 - present LibDriver All rights reserved
 * 
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. 
 *
 * @file      driver_st7789_interface.h
 * @brief     driver st7789 interface header file
 * @version   1.0.0
 * @author    Shifeng Li
 * @date      2023-04-15
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2023/04/15  <td>1.0      <td>Shifeng Li  <td>first upload
 * </table>
 */

#ifndef DRIVER_ST7789_INTERFACE_H
#define DRIVER_ST7789_INTERFACE_H

#include "driver_st7789.h"

#ifdef __cplusplus
extern "C"{
#endif

#define SPI_PORT spi0
#define PIN_MISO 4
#define PIN_CS   5
#define PIN_SCK  2
#define PIN_MOSI 3
#define RES 0
#define DC 1
#define BL 9

/* 常用 RGB565 颜色定义 (16位: R-5bit, G-6bit, B-5bit) */

// 基础颜色
#define WHITE       0xFFFF  // 白色 (255,255,255)
#define BLACK       0x0000  // 黑色 (0,0,0)
#define RED         0xF800  // 红色 (255,0,0)
#define GREEN       0x07E0  // 绿色 (0,255,0)
#define BLUE        0x001F  // 蓝色 (0,0,255)
#define YELLOW      0xFFE0  // 黄色 (255,255,0)
#define MAGENTA     0xF81F  // 品红 (255,0,255)
#define CYAN        0x07FF  // 青色 (0,255,255)
#define GRAY        0x8410  // 灰色 (128,128,128)

// 扩展颜色
#define ORANGE      0xFD20  // 橙色 (255,165,0)
#define PINK        0xFC9F  // 粉色 (255,192,203)
#define PURPLE      0x8010  // 紫色 (128,0,128)
#define BROWN       0xA145  // 棕色 (165,42,42)
#define LIME        0x87E0  // 酸橙绿 (50,205,50)
#define TEAL        0x0410  // 凫蓝 (0,128,128)
#define NAVY        0x0010  // 海军蓝 (0,0,128)
#define MAROON      0x8000  // 栗色 (128,0,0)
#define OLIVE       0x8400  // 橄榄色 (128,128,0)

// 灰度渐变
#define GRAY10      0x2104  // 10%灰度
#define GRAY20      0x4208  // 20%灰度
#define GRAY30      0x630C  // 30%灰度
#define GRAY40      0x8410  // 40%灰度
#define GRAY50      0xA514  // 50%灰度
#define GRAY60      0xC618  // 60%灰度
#define GRAY70      0xE71C  // 70%灰度
#define GRAY80      0xEF7D  // 80%灰度
#define GRAY90      0xF79E  // 90%灰度

#define RGB_TO_565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

/**
 * @defgroup st7789_interface_driver st7789 interface driver function
 * @brief    st7789 interface driver modules
 * @ingroup  st7789_driver
 * @{
 */

/**
 * @brief  interface spi bus init
 * @return status code
 *         - 0 success
 *         - 1 spi init failed
 * @note   none
 */
uint8_t st7789_interface_spi_init(void);

/**
 * @brief  interface spi bus deinit
 * @return status code
 *         - 0 success
 *         - 1 spi deinit failed
 * @note   none
 */
uint8_t st7789_interface_spi_deinit(void);

/**
 * @brief     interface spi bus write
 * @param[in] *buf pointer to a data buffer
 * @param[in] len length of data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
uint8_t st7789_interface_spi_write_cmd(uint8_t *buf, uint16_t len);

/**
 * @brief     interface delay ms
 * @param[in] ms time
 * @note      none
 */
void st7789_interface_delay_ms(uint32_t ms);

/**
 * @brief     interface print format data
 * @param[in] fmt format data
 * @note      none
 */
void st7789_interface_debug_print(const char *const fmt, ...);

/**
 * @brief  interface command && data gpio init
 * @return status code
 *         - 0 success
 *         - 1 gpio init failed
 * @note   none
 */
uint8_t st7789_interface_cmd_data_gpio_init(void);

/**
 * @brief  interface command && data gpio deinit
 * @return status code
 *         - 0 success
 *         - 1 gpio deinit failed
 * @note   none
 */
uint8_t st7789_interface_cmd_data_gpio_deinit(void);

/**
 * @brief     interface command && data gpio write
 * @param[in] value written value
 * @return    status code
 *            - 0 success
 *            - 1 gpio write failed
 * @note      none
 */
uint8_t st7789_interface_cmd_data_gpio_write(uint8_t value);

/**
 * @brief  interface reset gpio init
 * @return status code
 *         - 0 success
 *         - 1 gpio init failed
 * @note   none
 */
uint8_t st7789_interface_reset_gpio_init(void);

/**
 * @brief  interface reset gpio deinit
 * @return status code
 *         - 0 success
 *         - 1 gpio deinit failed
 * @note   none
 */
uint8_t st7789_interface_reset_gpio_deinit(void);

/**
 * @brief     interface reset gpio write
 * @param[in] value written value
 * @return    status code
 *            - 0 success
 *            - 1 gpio write failed
 * @note      none
 */
uint8_t st7789_interface_reset_gpio_write(uint8_t value);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
