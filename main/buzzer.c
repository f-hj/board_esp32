#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BUZZ_PIN 22

#define SHORT_SIZE 20
#define LONG_SIZE 40
#define INTERVAL 100

void init_buzz() {
    gpio_pad_select_gpio(BUZZ_PIN);
    gpio_set_direction(BUZZ_PIN, GPIO_MODE_OUTPUT);
}

void _short() {
    gpio_set_level(BUZZ_PIN, 1);
    vTaskDelay(SHORT_SIZE / portTICK_PERIOD_MS);
    gpio_set_level(BUZZ_PIN, 0);
}

void _long() {
    gpio_set_level(BUZZ_PIN, 1);
    vTaskDelay(LONG_SIZE / portTICK_PERIOD_MS);
    gpio_set_level(BUZZ_PIN, 0);
}

void buzz_short(void *pvParameters) {
    _short();
    vTaskDelete(NULL);
}

void buzz_double_short(void *pvParameters) {
    _short();
    vTaskDelay(INTERVAL / portTICK_PERIOD_MS);
    _short();

    vTaskDelete(NULL);
}

void buzz_triple_short(void *pvParameters) {
    _short();
    vTaskDelay(INTERVAL / portTICK_PERIOD_MS);
    _short();
    vTaskDelay(INTERVAL / portTICK_PERIOD_MS);
    _short();

    vTaskDelete(NULL);
}

void buzz_triple_long(void *pvParameters) {
    _long();
    vTaskDelay(INTERVAL / portTICK_PERIOD_MS);
    _long();
    vTaskDelay(INTERVAL / portTICK_PERIOD_MS);
    _long();

    vTaskDelete(NULL);
}