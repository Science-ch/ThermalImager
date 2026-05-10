#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "ov7670_sccb.h"

#define OV7670_ADDR 0x21   // 7bit
#define SCCB_SCL 14
#define SCCB_SDA 15

static inline void sccb_scl_high() {
    gpio_set_dir(SCCB_SCL, GPIO_IN);  // 释放 = 上拉为高
}

static inline void sccb_scl_low() {
    gpio_set_dir(SCCB_SCL, GPIO_OUT);
    gpio_put(SCCB_SCL, 0);
}

static inline void sccb_sda_high() {
    gpio_set_dir(SCCB_SDA, GPIO_IN);
}

static inline void sccb_sda_low() {
    gpio_set_dir(SCCB_SDA, GPIO_OUT);
    gpio_put(SCCB_SDA, 0);
}

static inline int sccb_sda_read() {
    gpio_set_dir(SCCB_SDA, GPIO_IN);
    return gpio_get(SCCB_SDA);
}

static inline void sccb_delay() {
    sleep_us(2);  // ~100~200kHz
}

void sccb_init() {
    gpio_init(SCCB_SCL);
    gpio_init(SCCB_SDA);

    gpio_pull_up(SCCB_SCL);
    gpio_pull_up(SCCB_SDA);

    sccb_scl_high();
    sccb_sda_high();
}

void sccb_start() {
    sccb_sda_high();
    sccb_scl_high();
    sccb_delay();

    sccb_sda_low();
    sccb_delay();
    sccb_scl_low();
}

void sccb_stop() {
    sccb_sda_low();
    sccb_delay();
    sccb_scl_high();
    sccb_delay();
    sccb_sda_high();
    sccb_delay();
}

void sccb_write_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        if (data & 0x80)
            sccb_sda_high();
        else
            sccb_sda_low();

        sccb_delay();
        sccb_scl_high();
        sccb_delay();
        sccb_scl_low();

        data <<= 1;
    }

    // ACK 位（忽略）
    sccb_sda_high();
    sccb_delay();
    sccb_scl_high();
    sccb_delay();
    sccb_scl_low();
}

void ov7670_write_reg(uint8_t reg, uint8_t val) {
    sccb_start();

    sccb_write_byte(OV7670_ADDR << 1); // 写
    sccb_write_byte(reg);
    sccb_write_byte(val);

    sccb_stop();
}
uint8_t ov7670_read_reg(uint8_t reg) {
    sccb_start();

    sccb_write_byte(OV7670_ADDR << 1); // 写
    sccb_write_byte(reg);

    sccb_start(); // 重复起始

    sccb_write_byte((OV7670_ADDR << 1) | 1); // 读

    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        data <<= 1;
        sccb_delay();
        sccb_scl_high();
        if (sccb_sda_read())
            data |= 1;
        sccb_scl_low();
    }

    // NACK
    sccb_sda_high();
    sccb_delay();
    sccb_scl_high();
    sccb_delay();
    sccb_scl_low();

    sccb_stop();

    return data;
}

void ov7670_init_regs() {
    // 复位
    ov7670_write_reg(0x12, 0x80);
    sleep_ms(50);
	
	ov7670_write_reg(0x3a, 0x04);//dummy
	ov7670_write_reg(0x40, 0xd0);//565   
	ov7670_write_reg(0x12, 0x14);//QVGA,RGB输出
 
	//输出窗口设置
	// ov7670_config_window(220,0,266,177);
	ov7670_config_window(175,7,320,240);
 
	ov7670_write_reg(0x0c, 0x00);
	ov7670_write_reg(0x15, 0x00);//0x00
	ov7670_write_reg(0x3e, 0x00);//10
	ov7670_write_reg(0x70, 0x3a);
	ov7670_write_reg(0x71, 0x35);//0x80 调试彩条
	ov7670_write_reg(0x72, 0x11);
	ov7670_write_reg(0x73, 0x00);//
 
	ov7670_write_reg(0xa2, 0x02);//15
	ov7670_write_reg(0x11, 0x01);//时钟分频设置,0,不分频.
	ov7670_write_reg(0x7a, 0x20);
	ov7670_write_reg(0x7b, 0x1c);
	ov7670_write_reg(0x7c, 0x28);
 
	ov7670_write_reg(0x7d, 0x3c);//20
	ov7670_write_reg(0x7e, 0x55);
	ov7670_write_reg(0x7f, 0x68);
	ov7670_write_reg(0x80, 0x76);
	ov7670_write_reg(0x81, 0x80);
 
	ov7670_write_reg(0x82, 0x88);
	ov7670_write_reg(0x83, 0x8f);
	ov7670_write_reg(0x84, 0x96);
	ov7670_write_reg(0x85, 0xa3);
	ov7670_write_reg(0x86, 0xaf);
 
	ov7670_write_reg(0x87, 0xc4);//30
	ov7670_write_reg(0x88, 0xd7);
	ov7670_write_reg(0x89, 0xe8);
	ov7670_write_reg(0x13, 0xe0);
	ov7670_write_reg(0x00, 0x00);//AGC
 
	ov7670_write_reg(0x10, 0x00);
	ov7670_write_reg(0x0d, 0x00);//全窗口， 位[5:4]: 01 半窗口，10 1/4窗口，11 1/4窗口 
	ov7670_write_reg(0x14, 0x28);//0x38, limit the max gain
	ov7670_write_reg(0xa5, 0x05);
	ov7670_write_reg(0xab, 0x07);
 
	ov7670_write_reg(0x24, 0x75);//40
	ov7670_write_reg(0x25, 0x63);
	ov7670_write_reg(0x26, 0xA5);
	ov7670_write_reg(0x9f, 0x78);
	ov7670_write_reg(0xa0, 0x68);
 
	ov7670_write_reg(0xa1, 0x03);//0x0b,
	ov7670_write_reg(0xa6, 0xdf);//0xd8,
	ov7670_write_reg(0xa7, 0xdf);//0xd8,
	ov7670_write_reg(0xa8, 0xf0);
	ov7670_write_reg(0xa9, 0x90);
 
	ov7670_write_reg(0xaa, 0x94);//50
	ov7670_write_reg(0x13, 0xe5);
	ov7670_write_reg(0x0e, 0x61);
	ov7670_write_reg(0x0f, 0x4b);
	ov7670_write_reg(0x16, 0x02);
 
	ov7670_write_reg(0x1e, 0x31);//图像输出镜像控制.0x07
	ov7670_write_reg(0x21, 0x02);
	ov7670_write_reg(0x22, 0x91);
	ov7670_write_reg(0x29, 0x07);
	ov7670_write_reg(0x33, 0x0b);
 
	ov7670_write_reg(0x35, 0x0b);//60
	ov7670_write_reg(0x37, 0x1d);
	ov7670_write_reg(0x38, 0x71);
	ov7670_write_reg(0x39, 0x2a);
	ov7670_write_reg(0x3c, 0x78);
 
	ov7670_write_reg(0x4d, 0x40);
	ov7670_write_reg(0x4e, 0x20);
	ov7670_write_reg(0x69, 0x00);
	ov7670_write_reg(0x6b, 0x4a);//PLL*4=48Mhz
	ov7670_write_reg(0x74, 0x19);
	ov7670_write_reg(0x8d, 0x4f);
 
	ov7670_write_reg(0x8e, 0x00);//70
	ov7670_write_reg(0x8f, 0x00);
	ov7670_write_reg(0x90, 0x00);
	ov7670_write_reg(0x91, 0x00);
	ov7670_write_reg(0x92, 0x00);//0x19,//0x66
 
	ov7670_write_reg(0x96, 0x00);
	ov7670_write_reg(0x9a, 0x80);
	ov7670_write_reg(0xb0, 0x84);
	ov7670_write_reg(0xb1, 0x0c);
	ov7670_write_reg(0xb2, 0x0e);
 
	ov7670_write_reg(0xb3, 0x82);//80
	ov7670_write_reg(0xb8, 0x0a);
	ov7670_write_reg(0x43, 0x14);
	ov7670_write_reg(0x44, 0xf0);
	ov7670_write_reg(0x45, 0x34);
 
	ov7670_write_reg(0x46, 0x58);
	ov7670_write_reg(0x47, 0x28);
	ov7670_write_reg(0x48, 0x3a);
	ov7670_write_reg(0x59, 0x88);
	ov7670_write_reg(0x5a, 0x88);
 
	ov7670_write_reg(0x5b, 0x44);//90
	ov7670_write_reg(0x5c, 0x67);
	ov7670_write_reg(0x5d, 0x49);
	ov7670_write_reg(0x5e, 0x0e);
	ov7670_write_reg(0x64, 0x04);
	ov7670_write_reg(0x65, 0x20);
 
	ov7670_write_reg(0x66, 0x05);
	ov7670_write_reg(0x94, 0x04);
	ov7670_write_reg(0x95, 0x08);
	ov7670_write_reg(0x6c, 0x0a);
	ov7670_write_reg(0x6d, 0x55);
 
 
	ov7670_write_reg(0x4f, 0x80);
	ov7670_write_reg(0x50, 0x80);
	ov7670_write_reg(0x51, 0x00);
	ov7670_write_reg(0x52, 0x22);
	ov7670_write_reg(0x53, 0x5e);
	ov7670_write_reg(0x54, 0x80);
 
	//ov7670_write_reg(0x54, 0x40);//110
 
	ov7670_write_reg(0x09, 0x03);//驱动能力最大
 
	ov7670_write_reg(0x6e, 0x11);//100
	ov7670_write_reg(0x6f, 0x9f);//0x9e for advance AWB
	ov7670_write_reg(0x55, 0x00);//亮度
	ov7670_write_reg(0x56, 0x40);//对比度 0x40
	ov7670_write_reg(0x57, 0x40);//0x40,  change according to Jim's request
	
	ov7670_write_reg(0x6a, 0x40);
	ov7670_write_reg(0x01, 0x40);
	ov7670_write_reg(0x02, 0x40);
	ov7670_write_reg(0x13, 0xe7);
	ov7670_write_reg(0x15, 0x00);  
	
		
	ov7670_write_reg(0x58, 0x9e);
	
	ov7670_write_reg(0x41, 0x08);
	ov7670_write_reg(0x3f, 0x00);
	ov7670_write_reg(0x75, 0x05);
	ov7670_write_reg(0x76, 0xe1);
	ov7670_write_reg(0x4c, 0x00);
	ov7670_write_reg(0x77, 0x01);
	ov7670_write_reg(0x3d, 0xc2);	
	ov7670_write_reg(0x4b, 0x09);
	ov7670_write_reg(0xc9, 0x60);
	ov7670_write_reg(0x41, 0x38);
	
	ov7670_write_reg(0x34, 0x11);
	ov7670_write_reg(0x3b, 0x02); 
	ov7670_write_reg(0xa4, 0x89);
	ov7670_write_reg(0x96, 0x00);
	ov7670_write_reg(0x97, 0x30);
	ov7670_write_reg(0x98, 0x20);
	ov7670_write_reg(0x99, 0x30);
	ov7670_write_reg(0x9a, 0x84);
	ov7670_write_reg(0x9b, 0x29);
	ov7670_write_reg(0x9c, 0x03);
	ov7670_write_reg(0x9d, 0x4c);
	ov7670_write_reg(0x9e, 0x3f);
	ov7670_write_reg(0x78, 0x04);
	

}

void ov7670_config_window(uint16_t startx,uint16_t starty,uint16_t width, uint16_t height)
{
    uint16_t endx=(startx+width*2)%784;
    uint16_t endy=(starty+height*2);
    uint8_t x_reg = 0x80, y_reg = 0x00;
    uint8_t state,temp;
    // x_reg = ov7670_read_reg(0x32) & 0xC0;
    // y_reg = ov7670_read_reg(0x03) & 0xF0;

    //设置 HREF
    temp = x_reg|((endx&0x7)<<3)|(startx&0x7);
    ov7670_write_reg(0x32, temp );
    temp = (startx&0x7F8)>>3;
    ov7670_write_reg(0x17, temp );
    temp = (endx&0x7F8)>>3;
    ov7670_write_reg(0x18, temp );
    //设置 VREF
    temp = y_reg|((endy&0x3)<<2)|(starty&0x3);
    ov7670_write_reg(0x03, temp );
    temp = (starty&0x3FC)>>2;
    ov7670_write_reg(0x19, temp );
    temp = (endy&0x3FC)>>2;
    ov7670_write_reg(0x1A, temp );

}