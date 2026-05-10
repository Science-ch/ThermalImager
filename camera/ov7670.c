#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "pico/multicore.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/async_context_freertos.h"
#include "include/irq.h"
#include "ov7670.h"
#include "ov7670_sccb.h"
#include "ov7670.pio.h"

SemaphoreHandle_t ov7670_dma_mutex;
uint16_t OV7670_MID, OV7670_PID;
uint16_t ov7670_buf[320 * 240];
static dma_channel_config cfg;

void ov7670_init()
{
    PIO pio = pio0;
    uint sm = 0;

    uint offset = pio_add_program(pio, &ov7670_program);

    pio_sm_config c = ov7670_program_get_default_config(offset);

    // 设置 GPIO    
    gpio_init(8);
    gpio_set_dir(8, GPIO_IN);

    gpio_init(10);
    gpio_set_dir(10, GPIO_IN);
    for (int i = 0; i < 8; i++) {
    pio_gpio_init(pio, i);
    }
    // pio_gpio_init(pio, 10);
    pio_sm_set_consecutive_pindirs(pio, sm, 0, 8, false);

    sm_config_set_in_pins(&c, 0);     // GPIO0~7
    sm_config_set_in_shift(&c, false, false, 16);
    sm_config_set_jmp_pin(&c, 10);    // HSYNC

    // PWDN
    gpio_init(11);
    gpio_set_dir(11, GPIO_OUT);
    gpio_put(11, 0);

    // VSYNC
    gpio_init(12);
    gpio_set_dir(12, GPIO_IN);

    // RST
    gpio_init(13);
    gpio_set_dir(13, GPIO_OUT);
    gpio_put(13, 0);

    // ov7670 XCLK 25Mhz时钟
    gpio_set_function(9, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(9);
    pwm_set_clkdiv(slice_num,2.5);
    pwm_set_wrap(slice_num,1);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 1);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 1);
    pwm_set_enabled(slice_num, true);

    // DMA 配置
    cfg = dma_channel_get_default_config(0);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, pio_get_dreq(pio, sm, false));
    dma_channel_set_irq0_enabled(0, true);

    dma_channel_configure(
        0,
        &cfg,
        &ov7670_buf[0],
        &pio->rxf[sm],
        320 * 210,
        false);

    gpio_put(13, 0);
    sleep_ms(1);
    gpio_put(13, 1);
    
    sccb_init();
    ov7670_init_regs();
    OV7670_MID = ov7670_read_reg(0x1C);OV7670_MID <<= 8;OV7670_MID |= ov7670_read_reg(0x1D);
    OV7670_PID = ov7670_read_reg(0x0A);OV7670_PID <<= 8;OV7670_PID |= ov7670_read_reg(0x0B);

    // 启动 SM
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

void ov7670_get_single_frame()
{
    // uint8_t skip_HSYNC = 3;
    while (gpio_get(12));
    while (!gpio_get(12));
    // while (skip_HSYNC)
    // {
    //     while(!gpio_get(10));
    //     while(gpio_get(10));
    //     skip_HSYNC--;
    // }
    
    dma_channel_set_write_addr(0, &ov7670_buf[0], true);
    // dma_channel_wait_for_finish_blocking(0);
    // xSemaphoreTake(ov7670_dma_mutex, portMAX_DELAY);
    
}