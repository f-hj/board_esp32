// motor duty cycle (w)
// command mode (rw)

#include "gatts_services.h"

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40
uint8_t char1_str[] = {0x11,0x22,0x33};

esp_gatt_char_prop_t a_property = 0;
static prepare_type_env_t a_prepare_write_env;
esp_attr_value_t gatts_demo_char1_val =
{
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char1_str),
    .attr_value   = char1_str,
};

void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(GSVCS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_MOTOR_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_MOTOR_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_MOTOR_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_MOTOR_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_MOTOR;

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(TEST_DEVICE_NAME);
        if (set_dev_name_ret){
            ESP_LOGE(GSVCS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }

        //config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret){
            ESP_LOGE(GSVCS_TAG, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= adv_config_flag;
        //config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret){
            ESP_LOGE(GSVCS_TAG, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= scan_rsp_config_flag;

        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_MOTOR_APP_ID].service_id, GATTS_NUM_HANDLE_MOTOR);
        break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(GSVCS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 0xde;
        rsp.attr_value.value[1] = 0xed;
        rsp.attr_value.value[2] = 0xbe;
        rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(GSVCS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep){
            ESP_LOGI(GSVCS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            esp_log_buffer_hex(GSVCS_TAG, param->write.value, param->write.len);
            move_forward(*param->write.value);
            if (gl_profile_tab[PROFILE_MOTOR_APP_ID].descs[0].descr_handle == param->write.handle && param->write.len == 2){
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0001){
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
                        ESP_LOGI(GSVCS_TAG, "notify enable");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++i)
                        {
                            notify_data[i] = i%0xff;
                        }
                        //the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_MOTOR_APP_ID].descs[0].char_handle,
                                                sizeof(notify_data), notify_data, false);
                    }
                }else if (descr_value == 0x0002){
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE){
                        ESP_LOGI(GSVCS_TAG, "indicate enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i%0xff;
                        }
                        //the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_MOTOR_APP_ID].descs[0].char_handle,
                                                sizeof(indicate_data), indicate_data, true);
                    }
                }
                else if (descr_value == 0x0000){
                    ESP_LOGI(GSVCS_TAG, "notify/indicate disable ");
                }else{
                    ESP_LOGE(GSVCS_TAG, "unknown descr value");
                    esp_log_buffer_hex(GSVCS_TAG, param->write.value, param->write.len);
                }

            }
        }
        example_write_event_env(gatts_if, &a_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(GSVCS_TAG,"ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&a_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(GSVCS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(GSVCS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gatts_char_inst_t chars[PROFILE_MOTOR_APP_CHARS] = {
            [0] = {
                .char_uuid.len = ESP_UUID_LEN_16,
                .char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_MOTOR_CONTROL
            },
        };
        
        gl_profile_tab[PROFILE_MOTOR_APP_ID].chars = chars;
        gl_profile_tab[PROFILE_MOTOR_APP_ID].service_handle = param->create.service_handle;

        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_MOTOR_APP_ID].service_handle);
        a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        
        for (int i = 0; i < PROFILE_MOTOR_APP_CHARS; i++) {
            esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_MOTOR_APP_ID].service_handle, &gl_profile_tab[PROFILE_MOTOR_APP_ID].chars[i].char_uuid,
                                                            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                            a_property,
                                                            &gatts_demo_char1_val, NULL);
            
            if (add_char_ret){
                ESP_LOGE(GSVCS_TAG, "add char failed, error code =%x",add_char_ret);
            }
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT: {
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(GSVCS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        
        gatts_desc_inst_t descs[PROFILE_MOTOR_APP_CHARS] = {
            [0] = {
                .char_handle = param->add_char.attr_handle,
                .descr_uuid.len = ESP_UUID_LEN_16,
                .descr_uuid.uuid.uuid16 = GATTS_DESCR_UUID_MOTOR_CONTROL
            },
        };
        gl_profile_tab[PROFILE_MOTOR_APP_ID].descs = descs;
        
        esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
        if (get_attr_ret == ESP_FAIL){
            ESP_LOGE(GSVCS_TAG, "ILLEGAL HANDLE");
        }

        ESP_LOGI(GSVCS_TAG, "the gatts demo char length = %x\n", length);
        for(int i = 0; i < length; i++){
            ESP_LOGI(GSVCS_TAG, "prf_char[%x] =%x\n",i,prf_char[i]);
        }
        for (int i = 0; i < PROFILE_MOTOR_APP_CHARS; i++) {
            esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_MOTOR_APP_ID].service_handle, &gl_profile_tab[PROFILE_MOTOR_APP_ID].descs[i].descr_uuid,
                                                                    ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
            if (add_descr_ret){
                ESP_LOGE(GSVCS_TAG, "add char descr failed, error code =%x", add_descr_ret);
            }
        }

        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        for (int i = 0; i < PROFILE_MOTOR_APP_CHARS; i++) {
            if (gl_profile_tab[PROFILE_MOTOR_APP_ID].descs[i].descr_uuid.uuid.uuid16 == param->add_char_descr.descr_uuid.uuid.uuid16) {
                gl_profile_tab[PROFILE_MOTOR_APP_ID].descs[i].descr_handle = param->add_char_descr.attr_handle;
            }
        }
        ESP_LOGI(GSVCS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GSVCS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT: {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
        ESP_LOGI(GSVCS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab[PROFILE_MOTOR_APP_ID].conn_id = param->connect.conn_id;
        //start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);

        xTaskCreate(buzz_triple_short, "buzz_conn", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
        move_forward(40);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GSVCS_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);

        move_stop();
        xTaskCreate(buzz_triple_long, "buzz_dconn", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(GSVCS_TAG, "ESP_GATTS_CONF_EVT, status %d", param->conf.status);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(GSVCS_TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}