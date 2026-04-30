#ifndef _OV7670_SCCB_H
#define _OV7670_SCCB_H

#include <stdint.h>

void sccb_init();
void ov7670_write_reg(uint8_t reg, uint8_t val);
uint8_t ov7670_read_reg(uint8_t reg);
void ov7670_init_regs();
void ov7670_config_window(uint16_t startx, uint16_t starty, uint16_t width, uint16_t height);

#endif