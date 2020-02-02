// name (rw)
// wifi

#include "gatts_services.h"

esp_gatt_char_prop_t c_property = 0;

void gatts_profile_c_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(GSVCS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_C_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_C_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_C_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_C_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_C;

        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_C_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_C);
        break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(GSVCS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 1;
        rsp.attr_value.value[0] = 42;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(GSVCS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile_tab[PROFILE_C_APP_ID].service_handle = param->create.service_handle;
        gatts_char_inst_t chars[PROFILE_C_APP_CHARS] = {
            [0] = {
                .char_uuid.len = ESP_UUID_LEN_16,
                .char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_C,
            }
        };
        gl_profile_tab[PROFILE_C_APP_ID].chars = chars;

        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_C_APP_ID].service_handle);
        c_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

        for (int i = 0; i < PROFILE_C_APP_CHARS; i++) {
                    esp_err_t add_char_ret = esp_ble_gatts_add_char( gl_profile_tab[PROFILE_C_APP_ID].service_handle, &gl_profile_tab[PROFILE_C_APP_ID].chars[i].char_uuid,
                                                        ESP_GATT_PERM_READ,
                                                        c_property,
                                                        NULL, NULL);
            if (add_char_ret){
                ESP_LOGE(GSVCS_TAG, "add char failed, error code =%x",add_char_ret);
            }
        }
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GSVCS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(GSVCS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);

        gatts_desc_inst_t descs[PROFILE_C_APP_CHARS] = {
            [0] = {
                .char_handle = param->add_char.attr_handle,
                .descr_uuid.len = ESP_UUID_LEN_16,
                .descr_uuid.uuid.uuid16 = GATTS_DESCR_UUID_TEST_C
            }
        };
        gl_profile_tab[PROFILE_C_APP_ID].descs = descs;

        for (int i = 0; i < PROFILE_C_APP_CHARS; i++) {
            esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_C_APP_ID].service_handle, &gl_profile_tab[PROFILE_C_APP_ID].descs[i].descr_uuid,
                                     ESP_GATT_PERM_READ,
                                     NULL, NULL);
        }
        break;
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        for (int i = 0; i < PROFILE_C_APP_CHARS; i++) {
            if (gl_profile_tab[PROFILE_C_APP_ID].descs[i].descr_uuid.uuid.uuid16 == param->add_char_descr.descr_uuid.uuid.uuid16) {
                gl_profile_tab[PROFILE_C_APP_ID].descs[i].descr_handle = param->add_char_descr.attr_handle;
            }
        }
        ESP_LOGI(GSVCS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    default:
        break;
    }
}