#include "ESP8266VESC.h"

#include "vesc.h"

#define VESC_UART UART_NUM_2

#define BUF_SIZE (1024)

ESP8266VESC vesc = ESP8266VESC(VESC_UART);
VESCValues vescValues;

float min = 30;
float max = 43.2;

void init_uart() {
}

float get_battery_percent() {
    return (vescValues.inputVoltage - min) / (max - min);
}

void start_uart (void *pvParameters) {
    while (1) {
        vesc.getVESCValues(vescValues);
        ESP_LOGI("VESC", "input voltage = %f", vescValues.inputVoltage);

        ESP_LOGI("VESC", "batt percent: %f", get_battery_percent() * 100);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void set_duty_cycle (float duty_cycle) {
    vesc.setDutyCycle(duty_cycle);
}