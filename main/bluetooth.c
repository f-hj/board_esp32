#include "bluetooth.h"

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"

#include "sdkconfig.h"

#define BT_TAG "bt"

void init_bt()
{
    esp_err_t ret;

    //ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(BT_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret) {
        ESP_LOGE(BT_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(BT_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(BT_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    esp_bt_cod_t cod;
    cod.major = 0b001000;
    cod.minor = 0b0010000;
    cod.service = 0b00000010110;
    ret = esp_bt_gap_set_cod(cod, ESP_BT_INIT_COD);
    if (ret) {
        ESP_LOGE(BT_TAG, "%s cannot set cod: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    init_spp();
    init_gatts();

    esp_a2d_sink_init();

    /* initialize AVRCP controller */
    esp_avrc_ct_init();

    return;
}