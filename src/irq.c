#include "include/irq.h"
#include "include/ThermalImager.h"

// 按钮GPIO
const uint BUTTON_GPIOS[4] = {25, 27, 28, 29};
volatile button_state_t buttons[4] = {0};

void irq_init()
{
    gpio_set_irq_enabled_with_callback(25, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &irq_handler);
    gpio_set_irq_enabled(27, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(28, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(29, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

void dma_irq_handler() {
    BaseType_t woken = pdFALSE;
    if (dma_channel_get_irq0_status(0)) {
        dma_channel_acknowledge_irq0(0);
        xSemaphoreGiveFromISR(ov7670_dma_mutex, &woken);
        portYIELD_FROM_ISR(woken);
    }
    if (dma_channel_get_irq0_status(1)) {
        dma_channel_acknowledge_irq0(1);
        xSemaphoreGiveFromISR(lcd_dma_mutex, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

void irq_handler(uint gpio, uint32_t events)
{
    static absolute_time_t last_edge_time[4] = {0}; // 去抖动计时
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // 确定按钮索引
    int idx = -1;
    for (int i = 0; i < 4; i++) {
        if (gpio == BUTTON_GPIOS[i]) { idx = i; break; }
    }
    if (idx < 0) return;  // 非按钮GPIO

    absolute_time_t now = get_absolute_time();

    // 去抖动：忽略过近的边沿
    if (absolute_time_diff_us(last_edge_time[idx], now) < DEBOUNCE_US) return;
    last_edge_time[idx] = now;

    if (events & GPIO_IRQ_EDGE_FALL) {
        // 下降沿：记录按下
        buttons[idx].press_time = now;
        buttons[idx].pressed = true;
        buttons[idx].long_press_triggered = false;
    }
    else if (events & GPIO_IRQ_EDGE_RISE) {
        // 上升沿：按键释放
        if (buttons[idx].pressed) {
            buttons[idx].pressed = false;

            // 如果长按尚未触发，且按下时长 < 1.5秒 → 短按
            if (!buttons[idx].long_press_triggered) {
                int64_t duration = absolute_time_diff_us(buttons[idx].press_time, now);
                if (duration < LONG_PRESS_THRESHOLD_US) {
                    // 短按处理
                    switch (gpio) {
                        case 25: // K1
                            MIN_TEMP -= 1.0f;
                            if (MIN_TEMP < -40.0f) MIN_TEMP = -40.0f;
                            MID_TEMP = (MIN_TEMP + MAX_TEMP) / 2.0f;
                            update_temp_range_params();
                            break;
                        case 27: // K2
                            MIN_TEMP += 1.0f;
                            if (MIN_TEMP > MAX_TEMP)
                                MIN_TEMP = MAX_TEMP;
                            if (MIN_TEMP > 300.0f) MIN_TEMP = 300.0f;
                            MID_TEMP = (MIN_TEMP + MAX_TEMP) / 2.0f;
                            update_temp_range_params();
                            break;
                        case 28: // K3
                            MAX_TEMP -= 1.0f;
                            if (MAX_TEMP < MIN_TEMP)
                                MAX_TEMP = MIN_TEMP;
                            if (MAX_TEMP < -40.0f) MAX_TEMP = -40.0f;
                            MID_TEMP = (MIN_TEMP + MAX_TEMP) / 2.0f;
                            update_temp_range_params();
                            break;
                        case 29: // K4
                            MAX_TEMP += 1.0f;
                            if (MAX_TEMP > 300.0f) MAX_TEMP = 300.0f;
                            MID_TEMP = (MIN_TEMP + MAX_TEMP) / 2.0f;
                            update_temp_range_params();
                            break;
                    }
                }
            }
        }
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}