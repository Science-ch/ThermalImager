#include "include/irq.h"
#include "include/ThermalImager.h"

void irq_init()
{
    gpio_set_irq_enabled_with_callback(25, GPIO_IRQ_EDGE_FALL, true, &irq_handler);
    gpio_set_irq_enabled(27, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(28, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(29, GPIO_IRQ_EDGE_FALL, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

void dma_irq_handler() {
    BaseType_t woken = pdFALSE;
    if (dma_channel_get_irq0_status(1)) {
        dma_channel_acknowledge_irq0(1);
        xSemaphoreGiveFromISR(lcd_dma_mutex, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

void irq_handler(uint gpio, uint32_t events)
{
    static absolute_time_t last_time[4] = {0};
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    //K1
    if (gpio == 25 && (events & GPIO_IRQ_EDGE_FALL))
    {
        if (absolute_time_diff_us(last_time[0], get_absolute_time()) > 20000)
        {
            last_time[0] = get_absolute_time();
            MIN_TEMP -= 1.0f;
            if (MIN_TEMP < -40.0f) MIN_TEMP = -40.0f;
            MID_TEMP = (MIN_TEMP + MAX_TEMP) / 2.0f;
            update_temp_range_params();
        }
    }
    //K2
    if (gpio == 27 && (events & GPIO_IRQ_EDGE_FALL))
    {
        if(absolute_time_diff_us(last_time[1], get_absolute_time()) > 20000)
        {
            last_time[1] = get_absolute_time();
            MIN_TEMP += 1.0f;
            if (MIN_TEMP > 300.0f) MIN_TEMP = 300.0f;
            MID_TEMP = (MIN_TEMP + MAX_TEMP) / 2.0f;
            update_temp_range_params();
        }
    }
    //K3
    if (gpio == 28 && (events & GPIO_IRQ_EDGE_FALL))
    {
        if(absolute_time_diff_us(last_time[2], get_absolute_time()) > 20000)
        {
            last_time[2] = get_absolute_time();
            MAX_TEMP -= 1.0f;
            if (MAX_TEMP < -40.0f) MAX_TEMP = -40.0f;
            MID_TEMP = (MIN_TEMP + MAX_TEMP) / 2.0f;
            update_temp_range_params();
        }
    }
    //K4
    if (gpio == 29 && (events & GPIO_IRQ_EDGE_FALL))
    {
        if(absolute_time_diff_us(last_time[3], get_absolute_time()) > 20000)
        {
            last_time[3] = get_absolute_time();
            MAX_TEMP += 1.0f;
            if (MAX_TEMP > 300.0f) MAX_TEMP = 300.0f;
            MID_TEMP = (MIN_TEMP + MAX_TEMP) / 2.0f;
            update_temp_range_params();
        }
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}