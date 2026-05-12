#ifndef _IRQ_H_
#define _IRQ_H_

#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/async_context_freertos.h"

#define LONG_PRESS_THRESHOLD_US 400000   // 0.4秒
#define DEBOUNCE_US 20000

extern const uint BUTTON_GPIOS[4];

// 每个按钮的状态
typedef struct {
    absolute_time_t press_time;       // 按下时刻
    bool pressed;                     // 当前是否处于按下状态
    bool long_press_triggered;        // 长按是否已触发（防重复）
} button_state_t;

extern volatile button_state_t buttons[4];
extern SemaphoreHandle_t lcd_dma_mutex, ov7670_dma_mutex;

void irq_init();
void irq_handler(uint gpio, uint32_t events);
void dma_irq_handler();

#endif