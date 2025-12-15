#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "include/driver_st7789_basic.h"
#include "include/MLX90640_I2C_Driver.h"
#include "include/color_lut.h"

#define MIN_TEMP 7.0f
#define MAX_TEMP 40.0f
#define MID_TEMP ((MIN_TEMP + MAX_TEMP) / 2.0f)


uint16_t frame_buffer[96*72]={0};

void draw_thermal_image(float *temps);
uint16_t temp_to_iron_color(float temp);
float normalize_temp(float temp);
void Temp2RGB(float *temp, int size, float maxTemp, uint16_t *rgb);
void bilinear_scale(const uint16_t *src, uint16_t *dst, int srcW, int srcH, int dstW, int dstH);

int main()
{
    stdio_init_all();
    gpio_set_function(14, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(14);
    pwm_set_clkdiv(slice_num,5.2);
    pwm_set_wrap(slice_num,1);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 1);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 1);
    pwm_set_enabled(slice_num, true);
    interp_config cfg = interp_default_config();
    // Now use the various interpolator library functions for your use case
    // e.g. interp_config_clamp(&cfg, true);
    //      interp_config_shift(&cfg, 2);
    // Then set the config 
    interp_set_config(interp0, 0, &cfg);

    // Timer example code - This example fires off the callback after 2000ms
    // add_alarm_in_ms(2000, alarm_callback, NULL, false);

    MLX90640_I2CInit();
    MLX90640_I2CFreqSet(1000*1000);
    

    st7789_basic_init();
    st7789_basic_clear();
    st7789_basic_display_on();

    uint16_t eeData[832];
    if(MLX90640_DumpEE(0x33, eeData) != 0) {
        st7789_basic_string(0,0,"EEPROM Read Error!",19,RED,ST7789_FONT_12);
        return 0;
    }
    paramsMLX90640 params;
    if(MLX90640_ExtractParameters(eeData, &params) != 0) {
        st7789_basic_string(0,40,"Params Read Error!",19,RED,ST7789_FONT_12);
        return 0;
    }

    st7789_basic_clear();
    char str[30];
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
    // for (int i = 0; i < 16; i++)
    // {
    //     for (int j = 0; j < 128; j++)
    //     {
    //         st7789_basic_write_point(j, 100 + i, color_lut2[j + 128 * (i / 8)]);
    //     }
        
        
    // }
    
    st7789_basic_draw_picture_16bits(118, 0, 127, 71, frame_buffer);

    MLX90640_SetChessMode(0x33);      // 使用棋盘模式
    MLX90640_SetRefreshRate(0x33, 3); // 4Hz刷新率
    MLX90640_SetResolution(0x33, 3);  // 19位分辨率

    // frame_buffer[96 * 1 + 1] = WHITE;
    // char str[50]="Science_ch";
    // if(st7789_basic_draw_picture_16bits(0, 0, 95, 71, frame_buffer)!=0)
    //     sprintf(str, "Return:1");
    // st7789_basic_string(0,73,str,strlen(str),MAROON,ST7789_FONT_12);
    // sprintf(str,"%d %d %d %d %d %d %d %d %d %d ",badPixels[0],badPixels[1],badPixels[2],badPixels[3],badPixels[4],
    // badPixels[5],badPixels[6],badPixels[7],badPixels[8],badPixels[9]);
    // st7789_basic_string(0,0,str,strlen(str),CYAN,ST7789_FONT_12);

    while(1) 
    {
        MLX90640_TriggerMeasurement(0x33);
        
        sleep_ms(20);

        uint16_t frameData[834];
        int a = MLX90640_GetFrameData(0x33, frameData);
        if(a > 2 && a < 0) {
            continue;
        }

        float ambientTemp = MLX90640_GetTa(frameData, &params);
        float vdd = MLX90640_GetVdd(frameData, &params);
        
        float temperatures[768];
        MLX90640_CalculateTo(frameData, &params, 0.95, ambientTemp-8, temperatures);
        
        MLX90640_BadPixelsCorrection(params.brokenPixels, temperatures, 1, &params);
        MLX90640_BadPixelsCorrection(params.outlierPixels, temperatures, 1, &params);

        if (a==1)
        {
            draw_thermal_image(temperatures);

            sprintf(str, "AmbientTemp:%4.1f", ambientTemp);
            st7789_basic_string(0, 72, str, strlen(str), ORANGE, ST7789_FONT_12);
            sprintf(str, "CentreTemp:%5.1f", temperatures[768 / 2]);
            st7789_basic_string(0, 84, str, strlen(str), ORANGE, ST7789_FONT_12); 
        }
        

    }
    return 0;
}

void draw_thermal_image(float *temps)
{
    // for (int i = 0; i < 24; i++)
    // {
    //     for (int j = 0; j < 32; j++)
    //     {
    //         float temp = temps[32 * i + j];
    //         uint16_t color = temp_to_iron_color(temp);

    //         for (int dy = 0; dy < 3; dy++)
    //         {
    //             for (int dx = 0; dx < 3; dx++)
    //             {
    //                 // frame_buffer[3 * 32 * 3 * i + 3 * j + 3 * 32 * dy + dx] = color;
    //                 frame_buffer[(3 * i + dy) * 96 + (3 * (31 - j) + dx)] = color;
    //             }
                
    //         }
            
    //     }
        
    // }

    uint16_t color[32 * 24];
    for (int i = 0; i < 24; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            float temp = temps[32 * i + j];
            color[i * 32 + 31 - j] = temp_to_iron_color(temp);
        }
        
    }

    bilinear_scale(color, frame_buffer, 32, 24, 96, 72);

    st7789_basic_draw_picture_16bits(0, 0, 95, 71, frame_buffer);
}

uint16_t temp_to_iron_color(float temp)
{
    int t = normalize_temp(temp) * 255;
    return color_lut2[t];

    // // 蓝->青->黄->红的渐变
    // float r, g, b;
    
    // if (t < 0.25f) {
    //     // 深蓝 -> 蓝
    //     r = 0.0f;
    //     g = 0.0f;
    //     b = 0.5f + 0.5f * (t / 0.25f);
    // } else if (t < 0.5f) {
    //     // 蓝 -> 青
    //     r = 0.0f;
    //     g = (t - 0.25f) / 0.25f;
    //     b = 1.0f;
    // } else if (t < 0.75f) {
    //     // 青 -> 黄
    //     r = (t - 0.5f) / 0.25f;
    //     g = 1.0f;
    //     b = 1.0f - (t - 0.5f) / 0.25f;
    // } else {
    //     // 黄 -> 红
    //     r = 1.0f;
    //     g = 1.0f - (t - 0.75f) / 0.25f;
    //     b = 0.0f;
    // }
    
    // // 转换为RGB565
    // uint8_t r5 = (uint8_t)(r * 31);
    // uint8_t g6 = (uint8_t)(g * 63);
    // uint8_t b5 = (uint8_t)(b * 31);
    
    // return (r5 << 11) | (g6 << 5) | b5;


    // double miniNum = 0.0002;
	// float L = MAX_TEMP;
	// float PI = 3.14;
    // /* 转温度为灰度 */
    // float grey = normalize_temp(temp) * 255;
    // /* 计算HSI */
    // float I = grey,H = (2*PI*grey)/L;
    // float S;
    // /* grey < L/2 */
    // if((grey-L/2) < miniNum){
    // //if(grey<L/2){
    //     S = 1.5 * grey;
    // }else{
    //     S = 1.5 * (L-grey);
    // }
    // /* 计算RGB */
    // float V1 = S* cos(H);
    // float V2 = S* sin(H);
    // float R = I - 0.204*V1 + 0.612*V2;
    // float G = I - 0.204*V1 - 0.612*V2;
    // float B = I + 0.408*V1; 
    // /* 转为16bits RGB[5-6-5]色彩 */
    // /* 
    //     (2^5-1)/(2^8-1) = 0.12 
    //     (2^6-1)/(2^8-1) = 0.24
    // */		
    // uint16_t rbits = (R*0.125);
    // uint16_t gbits = (G*0.250);
    // uint16_t bbits = (B*0.125);
    // return (rbits<<11)|(gbits<<5)|bbits;

}

float normalize_temp(float temp) {
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