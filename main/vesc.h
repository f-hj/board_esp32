#ifndef VESC_H
#define VESC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

void init_uart();
void start_uart(void *pvParameters);

float get_battery_percent();
void set_duty_cycle (float duty_cycle);

#ifdef __cplusplus
}
#endif

#endif