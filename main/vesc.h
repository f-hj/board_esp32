#ifndef VESC_H
#define VESC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

void init_uart();
void start_uart(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif