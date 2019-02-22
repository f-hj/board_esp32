#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"
#include "nvs_flash.h"

#include "buzzer.h"
#include "bluetooth.h"
#include "led.h"
#include "pwm.h"
#include "vesc.h"
#include "move.h"

extern "C" void app_main() {
    esp_err_t ret;

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        printf("NVS ERASE!!!\n");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_buzz();
    xTaskCreate(buzz_short, "buzz_start", configMINIMAL_STACK_SIZE, NULL, 5, NULL);

    init_led();

    init_uart();

    init_pwm();

    init_bt();
    xTaskCreate(buzz_double_short, "buzz_end", configMINIMAL_STACK_SIZE, NULL, 5, NULL);

    move_stop();
}