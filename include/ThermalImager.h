#ifndef _THERMALIMAGER_H
#define _THERMALIMAGER_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "include/driver_st7789_basic.h"
#include "include/MLX90640_I2C_Driver.h"
#include "include/color_lut.h"
#include "include/irq.h"
#include "camera/ov7670.h"
#include "camera/ov7670_sccb.h"
#include "pico/multicore.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/async_context_freertos.h"

extern float MIN_TEMP, MAX_TEMP, MID_TEMP;

typedef enum {
    mode_mix,
    mode_ther,
    mode_cam
} Display_mode;


void Initgpios();
void update_temp_range_params(void);
uint16_t temp_to_iron_color_int(int32_t temp_int);
void draw_thermal_image_int(void);
void draw_miximage_int(void);
void draw_camimage_int(void);
void draw_arrow(void);
uint16_t invert_rgb565(uint16_t color);
void check_long_press(void);

#endif