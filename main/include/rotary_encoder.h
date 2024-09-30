#ifndef __ROTARY_ENCODER_H__
#define __ROTARY_ENCODER_H__

#include "esp_err.h"

extern volatile bool buttonPressed;
extern volatile int16_t position;
extern volatile int8_t direction; // Counter-Clockwise = 0 , Clockwise = 1

esp_err_t rotary_init();

#endif /* rotary_encoder.h */
