#include "ThermalImager.h"

float MIN_TEMP = 24.0f, MAX_TEMP = 30.0f, MID_TEMP = (14.0f + 30.0f) / 2.0f;
// 对应的整数版本（放大 100 倍）
static int16_t MIN_TEMP_INT;
static int16_t MAX_TEMP_INT;
static int16_t MID_TEMP_INT;
static int32_t TEMP_RANGE_INT;      // MAX - MIN
// 归一化用的预计算参数（Q16.16 定点格式）
// 用于替代 (temp - MIN) / (MAX - MIN) * 255
// 等价于 (temp - MIN) * (255 * 65536 / RANGE) >> 16
static int32_t NORM_SCALE_Q16;      // = 255 * 65536 / TEMP_RANGE_INT
static int32_t NORM_OFFSET;         // = MIN_TEMP_INT

char str[40];
paramsMLX90640 params;
int16_t max_temp_int,min_temp_int,max_temp_pos,min_temp_pos,centre_temp;
uint16_t ThermaFrameBuffer[187 * 119] = {0}, MLX90640FrameData[834];
SemaphoreHandle_t MLX90640_get_data_Semaph,OV7670_get_data_Semaph;
static int16_t temps_int[768];
TaskHandle_t taskhandle_main, taskhandle_mlx, taskhandle_ov7670, taskhandle_button;
Display_mode cur_mode;

void ov7670_get_task(__unused void *pvParameters)
{
    // time_t start_time, end_time;
    while (1)
    {
        // start_time = time_us_64();
        ov7670_get_single_frame();
        // end_time = time_us_64();
        // sprintf(str, "%6ldus", end_time - start_time);
        // st7789_basic_string(0, 120, str, strlen(str), BLACK, ST7789_FONT_12);
        xSemaphoreGive(OV7670_get_data_Semaph);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

void mlx90640_get_task(__unused void *pvParameters)
{
    int16_t _max_temp_int,_min_temp_int,_max_temp_pos,_min_temp_pos;
    while (1)
    {
        switch(cur_mode)
        {
            case mode_cam:
                MLX90640_GetFrameData(0x33, MLX90640FrameData);
                float ambientTemp = 20.0f;
                float vdd = MLX90640_GetVdd(MLX90640FrameData, &params);
                
                MLX90640_CalculateTo_int(MLX90640FrameData, &params, 0.95, ambientTemp-8, temps_int);
                
                MLX90640_BadPixelsCorrection_int(params.brokenPixels, temps_int, 1, &params);
                MLX90640_BadPixelsCorrection_int(params.outlierPixels, temps_int, 1, &params);
                _min_temp_int = 32767; _max_temp_int = -32768;
                for (int i = 0; i < 768; i++)
                {
                    if (temps_int[i] > _max_temp_int)
                    {
                        _max_temp_int = temps_int[i];
                        _max_temp_pos = i;
                    }
                    if (temps_int[i] < _min_temp_int)
                    {
                        _min_temp_int = temps_int[i];
                        _min_temp_pos = i;
                    }
                }
                max_temp_int=_max_temp_int;
                min_temp_int=_min_temp_int;
                max_temp_pos=_max_temp_pos;
                min_temp_pos=_min_temp_pos;
                centre_temp=temps_int[768 / 2 - 16];
                break;
            default:
                MLX90640_GetFrameData(0x33, MLX90640FrameData);
                // xTaskNotifyGive(taskhandle_main);
                // xTaskNotifyGive(taskhandle_ov7670);
                xSemaphoreGive(MLX90640_get_data_Semaph);
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                break;
        }
    }
    
}


void main_task(__unused void *pvParameters)
{
    time_t start_time, end_time, frame_time;
    float ambientTemp, vdd;
    while (1)
    {
        switch(cur_mode)
        {
            case mode_mix:
                xSemaphoreTake(MLX90640_get_data_Semaph, portMAX_DELAY);
                // ambientTemp = MLX90640_GetTa(MLX90640FrameData, &params);
                ambientTemp = 20.0f;
                vdd = MLX90640_GetVdd(MLX90640FrameData, &params);
                
                MLX90640_CalculateTo_int(MLX90640FrameData, &params, 0.95, ambientTemp-8, temps_int);
                xTaskNotifyGive(taskhandle_mlx);
                MLX90640_BadPixelsCorrection_int(params.brokenPixels, temps_int, 1, &params);
                MLX90640_BadPixelsCorrection_int(params.outlierPixels, temps_int, 1, &params);
                min_temp_int = 32767; max_temp_int = -32768;
                for (int i = 0; i < 768; i++)
                {
                    if (temps_int[i] > max_temp_int)
                    {
                        max_temp_int = temps_int[i];
                        max_temp_pos = i;
                    }
                    if (temps_int[i] < min_temp_int)
                    {
                        min_temp_int = temps_int[i];
                        min_temp_pos = i;
                    }
                }
                // start_time = time_us_64();
                xSemaphoreTake(OV7670_get_data_Semaph, portMAX_DELAY);
                draw_miximage_int();
                xTaskNotifyGive(taskhandle_ov7670);
                draw_arrow();
                st7789_basic_draw_picture_16bits_dma(0, 0, 186, 118, ThermaFrameBuffer);
                // end_time = time_us_64();
                // st7789_basic_draw_picture_16bits_dma(0, 0, 186, 118, ThermaFrameBuffer);
                // sprintf(str, "%10ldus", end_time - start_time);
                // st7789_basic_string(80, 120, str, strlen(str), BLACK, ST7789_FONT_12);
                sprintf(str, "%5.1f", MIN_TEMP);
                st7789_basic_string(188, 0, str, strlen(str), BLACK, 8);
                sprintf(str, "%5.1f", MID_TEMP);
                st7789_basic_string(188, 55, str, strlen(str), BLACK, 8);
                sprintf(str, "%5.1f", MAX_TEMP);
                st7789_basic_string(188, 112, str, strlen(str), BLACK, 8);
                sprintf(str, "Cnt:%6.1f", (float)temps_int[768 / 2 - 16] / 100.0f);
                st7789_basic_string(0, 120, str, strlen(str), ORANGE, ST7789_FONT_12);
                sprintf(str, "Max:%6.1f", (float)max_temp_int / 100.0f);
                st7789_basic_string(64, 120, str, strlen(str), RED, ST7789_FONT_12);
                sprintf(str, "Min:%6.1f", (float)min_temp_int / 100.0f);
                st7789_basic_string(128, 120, str, strlen(str), BLUE, ST7789_FONT_12);
                sprintf(str, "%4.2fV", (adc_read() * 2.5f / 4096.0f) * 2.0f - 0.08f);
                st7789_basic_string(209, 120, str, strlen(str), BLACK, ST7789_FONT_12);
                // sprintf(str, "Amb:%4.1f", ambientTemp);
                // st7789_basic_string(192, 120, str, strlen(str), GREEN, ST7789_FONT_12);
                break;
            case mode_ther:
                xSemaphoreTake(MLX90640_get_data_Semaph, portMAX_DELAY);
                // float ambientTemp = MLX90640_GetTa(MLX90640FrameData, &params);
                ambientTemp = 20.0f;
                vdd = MLX90640_GetVdd(MLX90640FrameData, &params);
                
                MLX90640_CalculateTo_int(MLX90640FrameData, &params, 0.95, ambientTemp-8, temps_int);
                xTaskNotifyGive(taskhandle_mlx);
                MLX90640_BadPixelsCorrection_int(params.brokenPixels, temps_int, 1, &params);
                MLX90640_BadPixelsCorrection_int(params.outlierPixels, temps_int, 1, &params);
                min_temp_int = 32767; max_temp_int = -32768;
                for (int i = 0; i < 768; i++)
                {
                    if (temps_int[i] > max_temp_int)
                    {
                        max_temp_int = temps_int[i];
                        max_temp_pos = i;
                    }
                    if (temps_int[i] < min_temp_int)
                    {
                        min_temp_int = temps_int[i];
                        min_temp_pos = i;
                    }
                }
                draw_thermal_image_int();
                draw_arrow();
                st7789_basic_draw_picture_16bits_dma(0, 0, 186, 118, ThermaFrameBuffer);
                sprintf(str, "%5.1f", MIN_TEMP);
                st7789_basic_string(188, 0, str, strlen(str), BLACK, 8);
                sprintf(str, "%5.1f", MID_TEMP);
                st7789_basic_string(188, 55, str, strlen(str), BLACK, 8);
                sprintf(str, "%5.1f", MAX_TEMP);
                st7789_basic_string(188, 112, str, strlen(str), BLACK, 8);
                sprintf(str, "Cnt:%6.1f", (float)temps_int[768 / 2 - 16] / 100.0f);
                st7789_basic_string(0, 120, str, strlen(str), ORANGE, ST7789_FONT_12);
                sprintf(str, "Max:%6.1f", (float)max_temp_int / 100.0f);
                st7789_basic_string(64, 120, str, strlen(str), RED, ST7789_FONT_12);
                sprintf(str, "Min:%6.1f", (float)min_temp_int / 100.0f);
                st7789_basic_string(128, 120, str, strlen(str), BLUE, ST7789_FONT_12);
                sprintf(str, "%4.2fV", (adc_read() * 2.5f / 4096.0f) * 2.0f - 0.08f);
                st7789_basic_string(209, 120, str, strlen(str), BLACK, ST7789_FONT_12);
                break;
            case mode_cam:
                xTaskNotifyGive(taskhandle_mlx);
                xSemaphoreTake(OV7670_get_data_Semaph, portMAX_DELAY);
                draw_camimage_int();
                xTaskNotifyGive(taskhandle_ov7670);
                draw_arrow();
                st7789_basic_draw_picture_16bits_dma(0, 0, 186, 118, ThermaFrameBuffer);
                sprintf(str, "%5.1f", MIN_TEMP);
                st7789_basic_string(188, 0, str, strlen(str), BLACK, 8);
                sprintf(str, "%5.1f", MID_TEMP);
                st7789_basic_string(188, 55, str, strlen(str), BLACK, 8);
                sprintf(str, "%5.1f", MAX_TEMP);
                st7789_basic_string(188, 112, str, strlen(str), BLACK, 8);
                sprintf(str, "Cnt:%6.1f", (float)centre_temp / 100.0f);
                st7789_basic_string(0, 120, str, strlen(str), ORANGE, ST7789_FONT_12);
                sprintf(str, "Max:%6.1f", (float)max_temp_int / 100.0f);
                st7789_basic_string(64, 120, str, strlen(str), RED, ST7789_FONT_12);
                sprintf(str, "Min:%6.1f", (float)min_temp_int / 100.0f);
                st7789_basic_string(128, 120, str, strlen(str), BLUE, ST7789_FONT_12);
                sprintf(str, "%4.2fV", (adc_read() * 2.5f / 4096.0f) * 2.0f - 0.08f);
                st7789_basic_string(209, 120, str, strlen(str), BLACK, ST7789_FONT_12);
                break;
        }
        
    }
}

void button_task(__unused void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1)
    {
        check_long_press();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(20));
    }
}

int main()
{
    Initgpios();
    irq_init();
    
    st7789_basic_init();
    st7789_basic_clear();
    st7789_basic_display_on();
    
    MLX90640_I2CInit();
    MLX90640_I2CFreqSet(1000 * 1000);

    ov7670_init();
    
    
    // PIO pio = pio0;
    // uint sm = 0;
    
    // char str1[100];
    // uint ch = 0;

    // // sprintf(str1, "MID:%x  PID:%x", OV7670_MID, OV7670_PID);
    // // st7789_basic_string(0, 0, str1, strlen(str1), ORANGE, ST7789_FONT_12);
    // // sleep_ms(5000);
    // st7789_basic_clear();
    // while (1)
    // {
    //     ov7670_get_single_frame();
    //     st7789_basic_draw_picture_16bits(60, 7, 60 + 159, 7 + 119, ov7670_buf+320);
    // }
    

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
    sprintf(str, "%5.1f", MIN_TEMP);
    st7789_basic_string(188, 0, str, strlen(str), BLACK, 8);
    sprintf(str, "%5.1f", MID_TEMP);
    st7789_basic_string(188, 55, str, strlen(str), BLACK, 8);
    sprintf(str, "%5.1f", MAX_TEMP);
    st7789_basic_string(188, 112, str, strlen(str), BLACK, 8);
    update_temp_range_params();
    for (int i = 0; i < 119; i++) {
        uint16_t color = temp_to_iron_color_int(MIN_TEMP_INT + (MAX_TEMP_INT - MIN_TEMP_INT) * (float)i / 118.0f);
        uint16_t *row_start = &ThermaFrameBuffer[10 * i];
        for (int j = 0; j < 10; j++) {
            row_start[j] = color;
        }
    }
    st7789_basic_draw_picture_16bits(209, 0, 218, 118, ThermaFrameBuffer);

    MLX90640_SetChessMode(0x33);      // 使用棋盘模式
    MLX90640_SetRefreshRate(0x33, 4); // 8Hz刷新率
    MLX90640_SetResolution(0x33, 3);  // 19位分辨率

    lcd_dma_mutex = xSemaphoreCreateBinary();
    ov7670_dma_mutex = xSemaphoreCreateBinary();
    MLX90640_get_data_Semaph = xSemaphoreCreateBinary();
    OV7670_get_data_Semaph=xSemaphoreCreateBinary();


    xTaskCreate(main_task, "mainThread", 512, NULL, 1, &taskhandle_main);
    xTaskCreate(mlx90640_get_task, "ThermaDateGetThread", 512, NULL, 2, &taskhandle_mlx);
    xTaskCreate(ov7670_get_task, "OV7670DateGetThread", 512, NULL, 2, &taskhandle_ov7670);
    xTaskCreate(button_task, "ButtonThread", 256, NULL, 3, &taskhandle_button);
    vTaskCoreAffinitySet(taskhandle_main, 1);
    vTaskCoreAffinitySet(taskhandle_mlx, 1);
    vTaskCoreAffinitySet(taskhandle_button, 1);
    vTaskCoreAffinitySet(taskhandle_ov7670, 2);
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


void update_temp_range_params(void)
{
    // 转为放大 100 倍的整数
    MIN_TEMP_INT = (int16_t)(MIN_TEMP * 100.0f + 0.5f);
    MAX_TEMP_INT = (int16_t)(MAX_TEMP * 100.0f + 0.5f);
    MID_TEMP_INT = (int16_t)(MID_TEMP * 100.0f + 0.5f);
    TEMP_RANGE_INT = MAX_TEMP_INT - MIN_TEMP_INT;

    // 预计算归一化缩放因子（Q16.16）
    // scale = 255 / TEMP_RANGE_INT，但用定点数表示
    // 实际：scale_q16 = (255 << 16) / TEMP_RANGE_INT
    NORM_SCALE_Q16 = (int32_t)(((int64_t)255 << 16) / TEMP_RANGE_INT);
    NORM_OFFSET = MIN_TEMP_INT;
}


inline uint16_t temp_to_iron_color_int(int32_t temp_int)
{
    // 1. 裁剪到范围
    if (temp_int < MIN_TEMP_INT)
        temp_int = MIN_TEMP_INT;
    if (temp_int > MAX_TEMP_INT)
        temp_int = MAX_TEMP_INT;

    // 2. 归一化：t = (temp_int - OFFSET) * NORM_SCALE_Q16 >> 16
    int32_t t = (int32_t)(((int64_t)(temp_int - NORM_OFFSET) * NORM_SCALE_Q16) >> 16);

    // 3. 查表
    return color_lut2[t];
}

void draw_thermal_image_int(void)
{
    for (int y = 0; y < 119; y++)
    {
        // 源 y 坐标 (Q16)
        uint32_t src_y_q16 = y * 13217;
        int y1 = src_y_q16 >> 16;
        if (y1 > 22) y1 = 22;

        uint32_t dy_q16 = src_y_q16 & 0xFFFF;
        uint32_t one_minus_dy = 65536 - dy_q16;

        for (int x = 0; x < 187; x++)
        {
            // 源 x 坐标 (Q16)
            uint32_t src_x_q16 = x * 11215;
            int x1 = src_x_q16 >> 16;
            if (x1 > 30) x1 = 30;

            uint32_t dx_q16 = src_x_q16 & 0xFFFF;
            uint32_t one_minus_dx = 65536 - dx_q16;

            // 计算四个双线性权重（0~65536 的无符号整数）
            uint32_t w11 = (one_minus_dx * one_minus_dy) >> 16;
            uint32_t w12 = (dx_q16      * one_minus_dy) >> 16;
            uint32_t w21 = (one_minus_dx * dy_q16)      >> 16;
            uint32_t w22 = (dx_q16      * dy_q16)      >> 16;

            // 获取四个源温度值（放大 100 倍的整数）
            int base = y1 * 32 + x1;
            int16_t t11 = temps_int[base];
            int16_t t12 = temps_int[base + 1];
            int16_t t21 = temps_int[base + 32];
            int16_t t22 = temps_int[base + 33];

            // 加权求和（使用 64 位防止溢出）
            int64_t sum = (int64_t)w11 * t11 +
                          (int64_t)w12 * t12 +
                          (int64_t)w21 * t21 +
                          (int64_t)w22 * t22;

            // 右移 16 位抵消 Q16 缩放，恢复到放大 100 倍的整数温度
            int32_t temp_int = (int32_t)(sum >> 16);

            ThermaFrameBuffer[y * 187 + (186 - x)] = temp_to_iron_color_int(temp_int);
        }
    }
}

void draw_miximage_int(void)
{
    uint8_t alpha = 80;
    for (int y = 0; y < 119; y++)
    {
        // 源 y 坐标 (Q16)
        uint32_t src_y_q16 = y * 13217;
        int y1 = src_y_q16 >> 16;
        if (y1 > 22) y1 = 22;

        uint32_t dy_q16 = src_y_q16 & 0xFFFF;
        uint32_t one_minus_dy = 65536 - dy_q16;
        int dy = (y * 112189 >> 16) * 320 + 320 * 3;

        for (int x = 0; x < 187; x++)
        {
            int index = dy + ((186 - x) * 112398 >> 16);

            // 源 x 坐标 (Q16)
            uint32_t src_x_q16 = x * 11215;
            int x1 = src_x_q16 >> 16;
            if (x1 > 30) x1 = 30;

            uint32_t dx_q16 = src_x_q16 & 0xFFFF;
            uint32_t one_minus_dx = 65536 - dx_q16;

            // 计算四个双线性权重（0~65536 的无符号整数）
            uint32_t w11 = (one_minus_dx * one_minus_dy) >> 16;
            uint32_t w12 = (dx_q16      * one_minus_dy) >> 16;
            uint32_t w21 = (one_minus_dx * dy_q16)      >> 16;
            uint32_t w22 = (dx_q16      * dy_q16)      >> 16;

            // 获取四个源温度值（放大 100 倍的整数）
            int base = y1 * 32 + x1;
            int16_t t11 = temps_int[base];
            int16_t t12 = temps_int[base + 1];
            int16_t t21 = temps_int[base + 32];
            int16_t t22 = temps_int[base + 33];

            // 加权求和（使用 64 位防止溢出）
            int64_t sum = (int64_t)w11 * t11 +
                          (int64_t)w12 * t12 +
                          (int64_t)w21 * t21 +
                          (int64_t)w22 * t22;

            // 右移 16 位抵消 Q16 缩放，恢复到放大 100 倍的整数温度
            int32_t temp_int = (int32_t)(sum >> 16);

            uint16_t thermal_color = temp_to_iron_color_int(temp_int);
            uint16_t cam_color = ov7670_buf[index];

            // 提取 R, G, B 通道（5位, 6位, 5位）
            uint8_t cam_r = (cam_color >> 11) & 0x1F;
            uint8_t cam_g = (cam_color >> 5)  & 0x3F;
            uint8_t cam_b =  cam_color        & 0x1F;

            uint8_t th_r = (thermal_color >> 11) & 0x1F;
            uint8_t th_g = (thermal_color >> 5)  & 0x3F;
            uint8_t th_b =  thermal_color        & 0x1F;

            uint8_t r = (cam_r * (256 - alpha) + th_r * alpha + 128) >> 8;
            uint8_t g = (cam_g * (256 - alpha) + th_g * alpha + 128) >> 8;
            uint8_t b = (cam_b * (256 - alpha) + th_b * alpha + 128) >> 8;

            // 合并为 RGB565
            ThermaFrameBuffer[y * 187 + (186 - x)] = (r << 11) | (g << 5) | b;
        }
    }
}

void draw_camimage_int(void)
{
    for (int y = 0; y < 119; y++)
    {
        // 源 y 坐标 (Q16)
        int dy = (y * 112189 >> 16) * 320 + 320 * 3;
        for (int x = 0; x < 187; x++)
        {
            // 源 x 坐标 (Q16)
            int index = dy + (x * 112398 >> 16);
            ThermaFrameBuffer[y * 187 + x] = ov7670_buf[index];
        }
    }
}

void draw_arrow(void)
{
    int16_t max_x, max_y, min_x, min_y;
    max_x = (uint16_t)(31 - max_temp_pos % 32) * 1497 >> 8;
    max_y = (uint16_t)(max_temp_pos >> 5) * 1270 >> 8;
    min_x = (uint16_t)(31 - min_temp_pos % 32) * 1497 >> 8;
    min_y = (uint16_t)(min_temp_pos >> 5) * 1270 >> 8;


    for (uint8_t x = 90; x < 97; x++)
        ThermaFrameBuffer[59 * 187 + x] = invert_rgb565(ThermaFrameBuffer[59 * 187 + x]);
    for (uint8_t y = 56; y < 63; y++)
        ThermaFrameBuffer[y * 187 + 93] = invert_rgb565(ThermaFrameBuffer[y * 187 + 93]);

    for (int16_t x = max_x - 3; x < max_x + 5; x++)
    {
        if (x >= 0)
        {
            ThermaFrameBuffer[max_y * 187 + x] = RED;
            ThermaFrameBuffer[max_y * 187 + x + 187] = RED;
        }
    }
    for (int16_t y = max_y - 3; y < max_y + 5; y++)
    {
        if (y >= 0)
        {
            ThermaFrameBuffer[y * 187 + max_x] = RED;
            ThermaFrameBuffer[y * 187 + max_x + 1] = RED;
        }
    }

    for (int16_t x = min_x - 3; x < min_x + 5; x++)
    {
        if (x >= 0)
        {
            ThermaFrameBuffer[min_y * 187 + x] = PURPLE;
            ThermaFrameBuffer[min_y * 187 + x + 187] = PURPLE;
        }
    }
    for (int16_t y = min_y - 3; y < min_y + 5; y++)
    {
        if (y >= 0)
        {
            ThermaFrameBuffer[y * 187 + min_x] = PURPLE;
            ThermaFrameBuffer[y * 187 + min_x + 1] = PURPLE;
        }
    }
    
}

inline uint16_t invert_rgb565(uint16_t color) {
    uint8_t r = (color >> 11) & 0x1F;       // 红 5位
    uint8_t g = (color >> 5)  & 0x3F;       // 绿 6位
    uint8_t b =  color        & 0x1F;       // 蓝 5位

    r = 0x1F - r;   // 反红
    g = 0x3F - g;   // 反绿
    b = 0x1F - b;   // 反蓝

    return (r << 11) | (g << 5) | b;
}

void check_long_press(void)
{
    absolute_time_t now = get_absolute_time();

    for (int i = 0; i < 4; i++) {
        if (buttons[i].pressed && !buttons[i].long_press_triggered) {
            int64_t elapsed = absolute_time_diff_us(buttons[i].press_time, now);
            if (elapsed >= LONG_PRESS_THRESHOLD_US) {
                buttons[i].long_press_triggered = true;  // 保证只触发一次

                // 根据按钮索引执行长按操作
                switch (BUTTON_GPIOS[i]) {
                    case 25: // K1 长按
                        cur_mode = mode_mix;
                        break;
                    case 27: // K2 长按
                        cur_mode = mode_ther;
                        break;
                    case 28: // K3 长按
                        cur_mode = mode_cam;
                        break;
                    case 29: // K4 长按
                        MIN_TEMP = (float)min_temp_int / 100.0f; 
                        MAX_TEMP = (float)max_temp_int / 100.0f;
                        MID_TEMP = (MIN_TEMP + MAX_TEMP) / 2.0f;
                        update_temp_range_params();
                        break;
                }
            }
        }
    }
}