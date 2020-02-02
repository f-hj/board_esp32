// batt (r)
// lights (rw)

#include "gatts_services.h"
#include "../vesc.h"

#define STATUS_TAG "SERVICE_STATUS"

esp_gatt_char_prop_t b_property = 0;
static prepare_type_env_t b_prepare_write_env;

void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(STATUS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_STATUS_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_STATUS_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_STATUS_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_STATUS_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_STATUS;

        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_STATUS_APP_ID].service_id, GATTS_NUM_HANDLE_STATUS);
        break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(STATUS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;

        if (param->read.handle == gl_profile_tab[PROFILE_STATUS_APP_ID].descs[0].descr_handle) {
            // read battery
            rsp.attr_value.len = 4;
            ESP_LOGI(STATUS_TAG, "batt percent: %f", get_battery_percent());
            float2Bytes(get_battery_percent(), rsp.attr_value.value);
        } else {
            // read lights state
            rsp.attr_value.len = 1;
            rsp.attr_value.value[0] = lights_state();
        }

        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(STATUS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d\n", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep){
            ESP_LOGI(STATUS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            esp_log_buffer_hex(STATUS_TAG, param->write.value, param->write.len);
            switch_lights(*param->write.value);
            if (gl_profile_tab[PROFILE_STATUS_APP_ID].descs[0].descr_handle == param->write.handle && param->write.len == 2){
                uint16_t descr_value= param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0001){
                    if (b_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
                        ESP_LOGI(STATUS_TAG, "notify enable");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++i)
                        {
                            notify_data[i] = i%0xff;
                        }
                        //the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_STATUS_APP_ID].descs[0].char_handle,
                                                sizeof(notify_data), notify_data, false);
                    }
                }else if (descr_value == 0x0002){
                    if (b_property & ESP_GATT_CHAR_PROP_BIT_INDICATE){
                        ESP_LOGI(STATUS_TAG, "indicate enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i%0xff;
                        }
                        //the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_STATUS_APP_ID].descs[0].char_handle,
                                                sizeof(indicate_data), indicate_data, true);
                    }
                }
                else if (descr_value == 0x0000){
                    ESP_LOGI(STATUS_TAG, "notify/indicate disable ");
                }else{
                    ESP_LOGE(STATUS_TAG, "unknown value");
                }

            }
        }
        example_write_event_env(gatts_if, &b_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(STATUS_TAG,"ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&b_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(STATUS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(STATUS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gatts_char_inst_t chars[PROFILE_STATUS_APP_CHARS] = {
            [0] = {
                .char_uuid.len = ESP_UUID_LEN_16,
                .char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_STATUS_BATT,
                .perm = ESP_GATT_PERM_READ,
                .property = ESP_GATT_CHAR_PROP_BIT_READ
            },
            [1] = {
                .char_uuid.len = ESP_UUID_LEN_16,
                .char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_STATUS_LIGHTS,
                .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                .property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE
            }
        };
        gl_profile_tab[PROFILE_STATUS_APP_ID].chars = chars;
        gl_profile_tab[PROFILE_STATUS_APP_ID].service_handle = param->create.service_handle;

        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_STATUS_APP_ID].service_handle);
        
        for (int i = 0; i < PROFILE_STATUS_APP_CHARS; i++) {
            ESP_LOGI(STATUS_TAG, "creating char %d", i);
                    esp_err_t add_char_ret = esp_ble_gatts_add_char( gl_profile_tab[PROFILE_STATUS_APP_ID].service_handle, &gl_profile_tab[PROFILE_STATUS_APP_ID].chars[i].char_uuid,
                                                        gl_profile_tab[PROFILE_STATUS_APP_ID].chars[i].perm,
                                                        gl_profile_tab[PROFILE_STATUS_APP_ID].chars[i].property,
                                                        NULL, NULL);
            if (add_char_ret){
                ESP_LOGE(STATUS_TAG, "add char failed, error code =%x",add_char_ret);
            }
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(STATUS_TAG, "\n\nADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);


        int desc = 0;
        ESP_LOGI(STATUS_TAG, "uuid len: %d", param->add_char.char_uuid.len);

        switch (param->add_char.char_uuid.uuid.uuid16) {
            case GATTS_CHAR_UUID_STATUS_BATT: {
                desc = 0;
                gatts_desc_inst_t desco = {
                    .descr_uuid.len = ESP_UUID_LEN_16,
                    .descr_uuid.uuid.uuid16 = GATTS_DESCR_UUID_STATUS_BATT,
                };
                gl_profile_tab[PROFILE_STATUS_APP_ID].descs[0] = desco;
                break;
            }
            case GATTS_CHAR_UUID_STATUS_LIGHTS: {
                desc = 1;
                gatts_desc_inst_t desco = {
                    .descr_uuid.len = ESP_UUID_LEN_16,
                    .descr_uuid.uuid.uuid16 = GATTS_DESCR_UUID_STATUS_LIGHTS,
                };
                gl_profile_tab[PROFILE_STATUS_APP_ID].descs[1] = desco;
                break;
            }
            default: {
                return;
            }
        }
        ESP_LOGI(STATUS_TAG, "desc #%d", desc);

        gl_profile_tab[PROFILE_STATUS_APP_ID].descs[desc].descr_handle = param->add_char.attr_handle;

        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_STATUS_APP_ID].service_handle, &gl_profile_tab[PROFILE_STATUS_APP_ID].descs[desc].descr_uuid,
                                     ESP_GATT_PERM_READ,
                                     NULL, NULL);
        if (add_descr_ret){
            ESP_LOGE(GSVCS_TAG, "add char descr failed, error code =%x", add_descr_ret);
        }
        break;
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        ESP_LOGI(STATUS_TAG, "\n\nADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        for (int i = 0; i < PROFILE_STATUS_APP_CHARS; i++) {
            if (gl_profile_tab[PROFILE_STATUS_APP_ID].descs[i].descr_uuid.uuid.uuid16 == param->add_char_descr.descr_uuid.uuid.uuid16) {
                ESP_LOGI(STATUS_TAG, "add desc %d", i);
                ESP_LOGI(STATUS_TAG, "c hand %d", gl_profile_tab[PROFILE_STATUS_APP_ID].descs[i].char_handle);
                ESP_LOGI(STATUS_TAG, "uu %d", gl_profile_tab[PROFILE_STATUS_APP_ID].descs[i].descr_uuid.uuid.uuid16);
                ESP_LOGI(STATUS_TAG, "uu desc len %d", param->add_char_descr.descr_uuid.len);
                gl_profile_tab[PROFILE_STATUS_APP_ID].descs[i].descr_handle = param->add_char_descr.attr_handle;
            }
        }
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(STATUS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(STATUS_TAG, "CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab[PROFILE_STATUS_APP_ID].conn_id = param->connect.conn_id;
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(STATUS_TAG, "ESP_GATTS_CONF_EVT status %d", param->conf.status);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(STATUS_TAG, param->conf.value, param->conf.len);
        }
    break;
    case ESP_GATTS_DISCONNECT_EVT:
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}