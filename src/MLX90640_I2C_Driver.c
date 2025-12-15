#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "include/MLX90640_I2C_Driver.h"    
// 初始化 I2C 接口
void MLX90640_I2CInit(void) {
    i2c_init(I2C_PORT, 100 * 1000);  // 默认 100 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

// I2C 通用复位（可选实现）
int MLX90640_I2CGeneralReset(void) {
    // MLX90640 不支持标准 I2C 复位，但可以尝试发送开始-停止条件
    // i2c_perform_reset(I2C_PORT);
    return 0;  // 成功
}

// I2C 读取函数
int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data) {
    uint8_t buf[2];
    int result;
    
    // 准备要读取的地址 (大端序)
    buf[0] = (uint8_t)(startAddress >> 8);  // 地址高字节
    buf[1] = (uint8_t)(startAddress);       // 地址低字节
    
    // 写入目标地址
    result = i2c_write_blocking(I2C_PORT, slaveAddr, buf, 2, 1);
    if (result != 2) return -1;  // 写入失败
    
    // 读取数据
    uint8_t read_buf[2 * nMemAddressRead];
    result = i2c_read_blocking(I2C_PORT, slaveAddr, read_buf, 2 * nMemAddressRead, 0);
    if (result != 2 * nMemAddressRead) return -2;  // 读取失败
    
    // 将读取的数据转换为 uint16_t 数组 (大端序)
    for (int i = 0; i < nMemAddressRead; i++) {
        data[i] = (read_buf[2 * i] << 8) | read_buf[2 * i + 1];
    }
    
    return 0;  // 成功
}

// I2C 写入函数
int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data) {
    uint8_t buf[4];
    
    // 准备要写入的数据 (地址 + 数据，均为大端序)
    buf[0] = (uint8_t)(writeAddress >> 8);  // 地址高字节
    buf[1] = (uint8_t)(writeAddress);       // 地址低字节
    buf[2] = (uint8_t)(data >> 8);          // 数据高字节
    buf[3] = (uint8_t)(data);               // 数据低字节
    
    // 写入数据
    int result = i2c_write_blocking(I2C_PORT, slaveAddr, buf, 4, 0);
    
    return (result == 4) ? 0 : -1;  // 返回成功或失败
}

// 设置 I2C 频率
void MLX90640_I2CFreqSet(int freq) {
    i2c_init(I2C_PORT, freq);  // 重新初始化 I2C 端口
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
}