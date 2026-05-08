#include "ThermalImager.h"

float MIN_TEMP = 14.0f, MAX_TEMP = 30.0f, MID_TEMP = (14.0f + 30.0f) / 2.0f;
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
uint16_t ThermaFrameBuffer[96 * 72] = {0}, MLX90640FrameData[834];
SemaphoreHandle_t MLX90640_get_data_mutex,LCD_refresh_mutex;
static int16_t temps_int[768];
TaskHandle_t taskhandle_main, taskhandle_mlx;

void mlx90640_get_task(__unused void *pvParameters)
{
    while (1)
    {
        MLX90640_GetFrameData(0x33, MLX90640FrameData);
        xTaskNotifyGive(taskhandle_main);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
    
}


void main_task(__unused void *pvParameters)
{
    time_t start_time, end_time, frame_time;
    int16_t max_temp_int, min_temp_int;
    while (1)
    {
        frame_time = time_us_64();
        sprintf(str, "%5.1f", MIN_TEMP);
        st7789_basic_string(97, 0, str, strlen(str), BLACK, 8);
        sprintf(str, "%5.1f", MID_TEMP);
        st7789_basic_string(97, 32, str, strlen(str), BLACK, 8);
        sprintf(str, "%5.1f", MAX_TEMP);
        st7789_basic_string(97, 65, str, strlen(str), BLACK, 8);

        sprintf(str, "battery:%4.2fV", (adc_read() * 2.5f / 4096.0f) * 2.0f - 0.13f);
        st7789_basic_string(130, 0, str, strlen(str), BLACK, ST7789_FONT_12);

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        float ambientTemp = MLX90640_GetTa(MLX90640FrameData, &params);
        float vdd = MLX90640_GetVdd(MLX90640FrameData, &params);
        
        start_time = time_us_64();
        MLX90640_CalculateTo_int(MLX90640FrameData, &params, 0.95, ambientTemp-8, temps_int);
        end_time = time_us_64();
        xTaskNotifyGive(taskhandle_mlx);
        sprintf(str, "CalTemp:%8ldus", end_time - start_time);
        st7789_basic_string(130, 24, str, strlen(str), BLACK, ST7789_FONT_12);

        MLX90640_BadPixelsCorrection_int(params.brokenPixels, temps_int, 1, &params);
        MLX90640_BadPixelsCorrection_int(params.outlierPixels, temps_int, 1, &params);

        start_time = time_us_64();
        min_temp_int = 32767; max_temp_int = -32768;
        for (int i = 0; i < 768; i++)
        {
            if (temps_int[i] > max_temp_int)
                max_temp_int = temps_int[i];
            if (temps_int[i] < min_temp_int)
                min_temp_int = temps_int[i];
        }
        end_time = time_us_64();
        sprintf(str, "MinMax:%9ldus", end_time - start_time);
        st7789_basic_string(130, 36, str, strlen(str), BLACK, ST7789_FONT_12);

        start_time = time_us_64();
        draw_thermal_image_int();
        end_time = time_us_64();
        sprintf(str, "DrawImage:%6ldus", end_time - start_time);
        st7789_basic_string(130, 48, str, strlen(str), BLACK, ST7789_FONT_12);

        sprintf(str, "AmbientTemp:%4.1f", ambientTemp);
        st7789_basic_string(0, 72, str, strlen(str), GREEN, ST7789_FONT_12);
        sprintf(str, "CentreTemp:%6.1f", (float)temps_int[768 / 2 - 16] / 100.0f);
        st7789_basic_string(0, 84, str, strlen(str), ORANGE, ST7789_FONT_12);
        sprintf(str, "MaxTemp:%6.1f", (float)max_temp_int / 100.0f);
        st7789_basic_string(0, 96, str, strlen(str), RED, ST7789_FONT_12);
        sprintf(str, "MinTemp:%6.1f", (float)min_temp_int / 100.0f);
        st7789_basic_string(0, 108, str, strlen(str), BLUE, ST7789_FONT_12);

        end_time = time_us_64();
        sprintf(str, "Frame:%10ldus", end_time - frame_time);
        st7789_basic_string(130, 12, str, strlen(str), BLACK, ST7789_FONT_12);
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

    // ov7670_init();
    
    
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
    st7789_basic_string(97, 0, str, strlen(str), BLACK, 8);
    sprintf(str, "%5.1f", MID_TEMP);
    st7789_basic_string(97, 32, str, strlen(str), BLACK, 8);
    sprintf(str, "%5.1f", MAX_TEMP);
    st7789_basic_string(97, 65, str, strlen(str), BLACK, 8);
    update_temp_range_params();
    for (int i = 0; i < 72; i++) {
        uint16_t color = temp_to_iron_color_int(MIN_TEMP_INT + (MAX_TEMP_INT - MIN_TEMP_INT) * (float)i / 71.0f);
        uint16_t *row_start = &ThermaFrameBuffer[10 * i];
        for (int j = 0; j < 10; j++) {
            row_start[j] = color;
        }
    }
    st7789_basic_draw_picture_16bits(118, 0, 127, 71, ThermaFrameBuffer);

    MLX90640_SetChessMode(0x33);      // 使用棋盘模式
    MLX90640_SetRefreshRate(0x33, 4); // 8Hz刷新率
    MLX90640_SetResolution(0x33, 3);  // 19位分辨率

    lcd_dma_mutex = xSemaphoreCreateBinary();
    MLX90640_get_data_mutex = xSemaphoreCreateMutex();
    LCD_refresh_mutex = xSemaphoreCreateMutex();

    xTaskCreate(main_task, "mainThread", 1024 * 2, NULL, 1, &taskhandle_main);
    xTaskCreate(mlx90640_get_task, "ThermaDateGetThread", 512, NULL, 2, &taskhandle_mlx);
    vTaskCoreAffinitySet(taskhandle_main, 1);
    vTaskCoreAffinitySet(taskhandle_mlx, 1);
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

// ========================
// 当 MIN_TEMP / MAX_TEMP 变化时调用此函数
// ========================
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

// ========================
// 完全整数化的归一化 + 查表（热循环内联）
// temp_int: 放大 100 倍的温度
// ========================
inline uint16_t temp_to_iron_color_int(int32_t temp_int)
{
    // 1. 裁剪到范围
    if (temp_int < MIN_TEMP_INT)
        temp_int = MIN_TEMP_INT;
    if (temp_int > MAX_TEMP_INT)
        temp_int = MAX_TEMP_INT;

    // 2. 归一化：t = (temp_int - OFFSET) * NORM_SCALE_Q16 >> 16
    // 这里完全无除法，只有一个乘法和一个移位
    int32_t t = (int32_t)(((int64_t)(temp_int - NORM_OFFSET) * NORM_SCALE_Q16) >> 16);

    // 3. 查表
    return color_lut2[t];
}

// ========================
// 纯整数双线性插值 + 绘制
// ========================
void draw_thermal_image_int(void)
{
    const uint32_t STEP_Y_Q16 = 21845;  // 24/72 = 1/3
    const uint32_t STEP_X_Q16 = 21845;  // 32/96 = 1/3

    for (int y = 0; y < 72; y++)
    {
        uint32_t src_y_q16 = y * STEP_Y_Q16;
        int y1 = src_y_q16 >> 16;
        if (y1 > 22) y1 = 22;
        uint32_t dy_q16 = src_y_q16 & 0xFFFF;
        uint32_t one_minus_dy = 65536 - dy_q16;

        for (int x = 0; x < 96; x++)
        {
            uint32_t src_x_q16 = x * STEP_X_Q16;
            int x1 = src_x_q16 >> 16;
            if (x1 > 30) x1 = 30;
            uint32_t dx_q16 = src_x_q16 & 0xFFFF;
            uint32_t one_minus_dx = 65536 - dx_q16;

            // 权重，Q16 格式（实际值 * 65536），使用 uint32 防止溢出
            uint32_t w11 = (one_minus_dx * one_minus_dy) >> 16;
            uint32_t w12 = (dx_q16      * one_minus_dy) >> 16;
            uint32_t w21 = (one_minus_dx * dy_q16)      >> 16;
            uint32_t w22 = (dx_q16      * dy_q16)      >> 16;

            // 源温度索引
            int base = y1 * 32 + x1;
            int16_t t11 = temps_int[base];
            int16_t t12 = temps_int[base + 1];
            int16_t t21 = temps_int[base + 32];
            int16_t t22 = temps_int[base + 33];

            int64_t sum = (int64_t)w11 * t11 +
                          (int64_t)w12 * t12 +
                          (int64_t)w21 * t21 +
                          (int64_t)w22 * t22;

            int32_t temp_int = (int32_t)(sum >> 16);

            // 查表上色
            ThermaFrameBuffer[y * 96 + (95 - x)] = temp_to_iron_color_int(temp_int);
        }
    }

    st7789_basic_draw_picture_16bits_dma(0, 0, 95, 71, ThermaFrameBuffer);
}