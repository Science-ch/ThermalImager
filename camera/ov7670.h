#ifndef _OV7670_H
#define _OV7670_H

#include <stdint.h>

extern uint16_t OV7670_MID, OV7670_PID;
extern uint16_t ov7670_buf[320 * 240];

void ov7670_init();
void ov7670_get_single_frame();

#endif