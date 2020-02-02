#ifndef MOVE_H
#define MOVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led.h"
#include "pwm.h"

void move_forward(float duty_cycle);
void move_stop();

void switch_mode(bool is_degraded);

void switch_lights(bool l);
int lights_state();


#ifdef __cplusplus
}
#endif

#endif