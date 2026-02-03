#ifndef _THERMALIMAGER_H
#define _THERMALIMAGER_H

#include <stdint.h>

void Initgpios();
void InitIRQ();
void draw_thermal_image(float *temps);
uint16_t temp_to_iron_color(float temp);
float normalize_temp(float temp);
void Temp2RGB(float *temp, int size, float maxTemp, uint16_t *rgb);
void bilinear_scale(const uint16_t *src, uint16_t *dst, int srcW, int srcH, int dstW, int dstH);
void irq_handler(uint gpio, uint32_t events);

#endif