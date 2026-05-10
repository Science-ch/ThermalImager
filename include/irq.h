#ifndef _IRQ_H_
#define _IRQ_H_

#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/async_context_freertos.h"

extern SemaphoreHandle_t lcd_dma_mutex, ov7670_dma_mutex;

void irq_init();
void irq_handler(uint gpio, uint32_t events);
void dma_irq_handler();

#endif