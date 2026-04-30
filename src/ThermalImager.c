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
#include "camera/ov7670.h"
#include "camera/ov7670_sccb.h"
#include "pico/multicore.h"
#include "FreeRTOS.h"
#include "task.h"

// Which core to run on if configNUMBER_OF_CORES==1
#ifndef RUN_FREE_RTOS_ON_CORE
#define RUN_FREE_RTOS_ON_CORE 0
#endif

#include "pico/async_context_freertos.h"
#include "ThermalImager.h"

float MIN_TEMP = 14.0f, MAX_TEMP = 30.0f, MID_TEMP = (14.0f + 30.0f) / 2.0f;

// #define MIN_TEMP 7.0f
// #define MAX_TEMP 40.0f
// #define MID_TEMP ((MIN_TEMP + MAX_TEMP) / 2.0f)

paramsMLX90640 params;
uint16_t frame_buffer[96*72]={0};

void main_task(__unused void *pvParameters)
{
    time_t start_time,end_time;
    char str[100];
    uint16_t frameData[834];
    float temperatures[768];
    while (1)
    {
        sprintf(str, "%5.1f", MIN_TEMP);
        st7789_basic_string(97, 0, str, strlen(str), BLACK, 8);
        sprintf(str, "%5.1f", MID_TEMP);
        st7789_basic_string(97, 32, str, strlen(str), BLACK, 8);
        sprintf(str, "%5.1f", MAX_TEMP);
        st7789_basic_string(97, 65, str, strlen(str), BLACK, 8);

        sprintf(str, "battery:%4.2fV", (adc_read() * 2.5f / 4096.0f) * 2.0f - 0.13f);
        st7789_basic_string(130, 0, str, strlen(str), BLACK, ST7789_FONT_12);

        start_time = time_us_64();
        MLX90640_TriggerMeasurement(0x33);

        MLX90640_GetFrameData(0x33, frameData);
        MLX90640_GetFrameData(0x33, frameData);
        end_time = time_us_64();
        sprintf(str, "GetFrame:%7ldus", end_time - start_time);
        st7789_basic_string(130, 12, str, strlen(str), BLACK, ST7789_FONT_12);

        float ambientTemp = MLX90640_GetTa(frameData, &params);
        float vdd = MLX90640_GetVdd(frameData, &params);
        
        start_time = time_us_64();
        MLX90640_CalculateTo(frameData, &params, 0.95, ambientTemp-8, temperatures);
        end_time = time_us_64();
        sprintf(str, "CalTemp:%8ldus", end_time - start_time);
        st7789_basic_string(130, 24, str, strlen(str), BLACK, ST7789_FONT_12);

        start_time = time_us_64();
        MLX90640_BadPixelsCorrection(params.brokenPixels, temperatures, 1, &params);
        MLX90640_BadPixelsCorrection(params.outlierPixels, temperatures, 1, &params);
        end_time = time_us_64();
        sprintf(str, "BadPixelFix:%4ldus", end_time - start_time);
        st7789_basic_string(130, 36, str, strlen(str), BLACK, ST7789_FONT_12);

        start_time = time_us_64();
        draw_thermal_image(temperatures);
        end_time = time_us_64();
        sprintf(str, "DrawImage:%6ldus", end_time - start_time);
        st7789_basic_string(130, 48, str, strlen(str), BLACK, ST7789_FONT_12);

        sprintf(str, "AmbientTemp:%4.1f", ambientTemp);
        st7789_basic_string(0, 72, str, strlen(str), ORANGE, ST7789_FONT_12);
        sprintf(str, "CentreTemp:%5.1f", temperatures[768 / 2 - 16]);
        st7789_basic_string(0, 84, str, strlen(str), ORANGE, ST7789_FONT_12);
    }
}

int main()
{
    // ov7670_init();
    // while (1)
    // {
        
    // }
    
    Initgpios();
    InitIRQ();

    MLX90640_I2CInit();
    MLX90640_I2CFreqSet(1000*1000);
    
    st7789_basic_init();
    st7789_basic_clear();
    st7789_basic_display_on();

    ov7670_init();
    
    
    PIO pio = pio0;
    uint sm = 0;
    
    char str1[100];
    uint ch = 0;

    // sprintf(str1, "MID:%x  PID:%x", OV7670_MID, OV7670_PID);
    // st7789_basic_string(0, 0, str1, strlen(str1), ORANGE, ST7789_FONT_12);
    // sleep_ms(5000);
    st7789_basic_clear();
    while (1)
    {
        ov7670_get_single_frame();
        st7789_basic_draw_picture_16bits(60, 7, 60 + 159, 7 + 119, ov7670_buf+320);
    }
    

    uint16_t *eeData = (uint16_t *)malloc(832);
    if(MLX90640_DumpEE(0x33, eeData) != 0) {
        st7789_basic_string(0,0,"EEPROM Read Error!",18,RED,ST7789_FONT_12);
        free(eeData);
        return 0;
    }
    
    if(MLX90640_ExtractParameters(eeData, &params) != 0) {
        st7789_basic_string(0,0,"Params Read Error!",18,RED,ST7789_FONT_12);
        free(eeData);
        return 0;
    }
    free(eeData);

    st7789_basic_clear();
    char str[100];
    sprintf(str, "%5.1f", MIN_TEMP);
    st7789_basic_string(97, 0, str, strlen(str), BLACK, 8);
    sprintf(str, "%5.1f", MID_TEMP);
    st7789_basic_string(97, 32, str, strlen(str), BLACK, 8);
    sprintf(str, "%5.1f", MAX_TEMP);
    st7789_basic_string(97, 65, str, strlen(str), BLACK, 8);
    for (int i = 0; i < 72; i++) {
        uint16_t color = temp_to_iron_color(MIN_TEMP + (MAX_TEMP - MIN_TEMP) * (float)i / 71.0f);
        uint16_t *row_start = &frame_buffer[10 * i];
        for (int j = 0; j < 10; j++) {
            row_start[j] = color;
        }
    }
    st7789_basic_draw_picture_16bits(118, 0, 127, 71, frame_buffer);

    MLX90640_SetChessMode(0x33);      // 使用棋盘模式
    MLX90640_SetRefreshRate(0x33, 4); // 8Hz刷新率
    MLX90640_SetResolution(0x33, 3);  // 19位分辨率

    xTaskCreate(main_task, "mainThread", 1024 * 2, NULL, 1, NULL);
    vTaskStartScheduler();
    while(1);
    return 0;
}

void Initgpios()
{
    stdio_init_all();
    gpio_init(24);//充电指示
    gpio_set_dir(24, GPIO_IN);
    gpio_pull_up(24);

    gpio_init(25);//K1
    gpio_set_dir(25, GPIO_IN);
    gpio_pull_up(25);
    gpio_init(27);//K2
    gpio_set_dir(27, GPIO_IN);
    gpio_pull_up(27);
    gpio_init(28);//K3
    gpio_set_dir(28, GPIO_IN);
    gpio_pull_up(28);
    gpio_init(29);//K4
    gpio_set_dir(29, GPIO_IN);
    gpio_pull_up(29);

    adc_init();
    adc_gpio_init(26);//电池电压
    adc_select_input(0);

    gpio_set_function(BL, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BL);
    pwm_set_clkdiv(slice_num,125000000/500000);
    pwm_set_wrap(slice_num,100);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 60);
    pwm_set_enabled(slice_num, true);
}

void InitIRQ()
{
    gpio_set_irq_enabled_with_callback(25, GPIO_IRQ_EDGE_FALL, true, &irq_handler);
    gpio_set_irq_enabled(27, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(28, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(29, GPIO_IRQ_EDGE_FALL, true);
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
        }
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void draw_thermal_image(float *temps)
{  
    const float scale_x = 32.0f / 96.0f;
    const float scale_y = 24.0f / 72.0f;
    
    for (int y = 0; y < 72; y++)
    {
        for (int x = 0; x < 96; x++)
        {
            float src_x = x * scale_x;
            float src_y = y * scale_y;
            
            int x1 = (int)src_x;
            int y1 = (int)src_y;
            
            // 限制边界
            if (x1 > 30) x1 = 30;
            if (y1 > 22) y1 = 22;
            
            float dx = src_x - x1;
            float dy = src_y - y1;
            
            // 预计算权重
            float w11 = (1 - dx) * (1 - dy);
            float w12 = dx * (1 - dy);
            float w21 = (1 - dx) * dy;
            float w22 = dx * dy;
            
            // 获取温度值
            int base_idx = y1 * 32 + x1;
            float t11 = temps[base_idx];
            float t12 = temps[base_idx + 1];
            float t21 = temps[base_idx + 32];
            float t22 = temps[base_idx + 33];
            
            // 计算插值温度
            float temp = w11 * t11 + w12 * t12 + w21 * t21 + w22 * t22;
            
            // 直接存入frame_buffer
            frame_buffer[y * 96 + (95 - x)] = temp_to_iron_color(temp);
        }
    }

    st7789_basic_draw_picture_16bits(0, 0, 95, 71, frame_buffer);
}
inline uint16_t temp_to_iron_color(float temp)
{
    int t = normalize_temp(temp) * 255;
    return color_lut2[t];
}

inline float normalize_temp(float temp) {
    float min_temp = MIN_TEMP;
    float max_temp = MAX_TEMP;
    if (temp < min_temp) return 0.0f;
    if (temp > max_temp) return 1.0f;
    return (temp - min_temp) / (max_temp - min_temp);
}

void Temp2RGB(float* temp,int size,float maxTemp,uint16_t* rgb)
{
	double miniNum = 0.0002;
	float L = maxTemp;
	float PI = 3.14;
	for(int i=0;i<size;i++){
		/* 转温度为灰度 */
		float grey = (temp[i]*255)/300;
		/* 计算HSI */
		float I = grey,H = (2*PI*grey)/L;
		float S;
		/* grey < L/2 */
		if((grey-L/2) < miniNum){
		//if(grey<L/2){
			S = 1.5 * grey;
		}else{
			S = 1.5 * (L-grey);
		}
		/* 计算RGB */
		float V1 = S* cos(H);
		float V2 = S* sin(H);
		float R = I - 0.204*V1 + 0.612*V2;
		float G = I - 0.204*V1 - 0.612*V2;
		float B = I + 0.408*V1; 
		/* 转为16bits RGB[5-6-5]色彩 */
		/* 
			(2^5-1)/(2^8-1) = 0.12 
			(2^6-1)/(2^8-1) = 0.24
		*/		
		uint16_t rbits = (R*0.125);
		uint16_t gbits = (G*0.250);
		uint16_t bbits = (B*0.125);
		rgb[i] = (rbits<<11)|(gbits<<5)|bbits;
	}
	
}

void bilinear_scale(const uint16_t *src, uint16_t *dst, int srcW, int srcH, int dstW, int dstH) {
    float scaleX = (float)(srcW - 1) / (dstW - 1);
    float scaleY = (float)(srcH - 1) / (dstH - 1);

    for (int y = 0; y < dstH; y++) {
        for (int x = 0; x < dstW; x++) {
            float srcX = x * scaleX;
            float srcY = y * scaleY;

            int x1 = (int)srcX;
            int y1 = (int)srcY;
            int x2 = (x1 + 1 >= srcW) ? srcW - 1 : x1 + 1;
            int y2 = (y1 + 1 >= srcH) ? srcH - 1 : y1 + 1;

            float dx = srcX - x1;
            float dy = srcY - y1;

            uint16_t Q11 = src[y1 * srcW + x1];
            uint16_t Q21 = src[y1 * srcW + x2];
            uint16_t Q12 = src[y2 * srcW + x1];
            uint16_t Q22 = src[y2 * srcW + x2];

            // 提取 RGB565 分量
            float r1 = (Q11 >> 11) & 0x1F;
            float g1 = (Q11 >> 5)  & 0x3F;
            float b1 = Q11         & 0x1F;

            float r2 = (Q21 >> 11) & 0x1F;
            float g2 = (Q21 >> 5)  & 0x3F;
            float b2 = Q21         & 0x1F;

            float r3 = (Q12 >> 11) & 0x1F;
            float g3 = (Q12 >> 5)  & 0x3F;
            float b3 = Q12         & 0x1F;

            float r4 = (Q22 >> 11) & 0x1F;
            float g4 = (Q22 >> 5)  & 0x3F;
            float b4 = Q22         & 0x1F;

            // 水平插值（上方）
            float topR = (1 - dx) * r1 + dx * r2;
            float topG = (1 - dx) * g1 + dx * g2;
            float topB = (1 - dx) * b1 + dx * b2;

            // 水平插值（下方）
            float bottomR = (1 - dx) * r3 + dx * r4;
            float bottomG = (1 - dx) * g3 + dx * g4;
            float bottomB = (1 - dx) * b3 + dx * b4;

            // 垂直插值
            float R = (1 - dy) * topR + dy * bottomR;
            float G = (1 - dy) * topG + dy * bottomG;
            float B = (1 - dy) * topB + dy * bottomB;

            // 合并成 RGB565
            dst[y * dstW + x] = ((uint16_t)(R) << 11) | ((uint16_t)(G) << 5) | (uint16_t)(B);
        }
    }
}