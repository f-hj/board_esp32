#ifndef GATTS_SERVICES_H
#define GATTS_SERVICES_H

#ifdef __cplusplus
extern "C" {
#endif

#define GSVCS_TAG "GATTS_SERVICES"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "../buzzer.h"
#include "../move.h"

// Profiles ids
#define PROFILE_NUM 3

#define PROFILE_MOTOR_APP_ID 0
#define PROFILE_MOTOR_APP_CHARS 1

#define PROFILE_STATUS_APP_ID 1
#define PROFILE_STATUS_APP_CHARS 2

#define PROFILE_C_APP_ID 2
#define PROFILE_C_APP_CHARS 1

typedef struct {
  esp_bt_uuid_t char_uuid;
  esp_gatt_perm_t perm;
  esp_gatt_char_prop_t property;
} gatts_char_inst_t;

typedef struct {
  uint16_t char_handle;
  uint16_t descr_handle;
  esp_bt_uuid_t descr_uuid;
} gatts_desc_inst_t;

// Internal config
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    gatts_char_inst_t *chars;
    gatts_desc_inst_t *descs;
};

static uint8_t adv_service_uuid128[48] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x8F, 0xC0, 0x00, 0x00,
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x51, 0xA1, 0x00, 0x00,
    //second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x10, 0x80, 0x00, 0x00,
};

// The length of adv data must be less than 31 bytes
//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
//adv data
static esp_ble_adv_data_t adv_data;

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 48,
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)
static uint8_t adv_config_done = 0;

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

void create_services_table();
void float2Bytes(float val, uint8_t *bytes_array);


#define TEST_DEVICE_NAME            "FBoardD"

// Motor control
#define GATTS_SERVICE_UUID_MOTOR   0x8010

#define GATTS_CHAR_UUID_MOTOR_CONTROL      0xFF01
#define GATTS_DESCR_UUID_MOTOR_CONTROL     0x3333

#define GATTS_NUM_HANDLE_MOTOR     4


// Status control
#define GATTS_SERVICE_UUID_STATUS   0x51A1

#define GATTS_CHAR_UUID_STATUS_BATT      0xEE01
#define GATTS_DESCR_UUID_STATUS_BATT     0x2222

#define GATTS_CHAR_UUID_STATUS_LIGHTS      0xEE02
#define GATTS_DESCR_UUID_STATUS_LIGHTS     0x4444

#define GATTS_NUM_HANDLE_STATUS     8


// Config control
#define GATTS_SERVICE_UUID_TEST_C   0xC08F
#define GATTS_CHAR_UUID_TEST_C      0xDD01
#define GATTS_DESCR_UUID_TEST_C     0x1111
#define GATTS_NUM_HANDLE_TEST_C     4

// Profile handles
void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void gatts_profile_c_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);


// Internal funcs
void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

struct gatts_profile_inst gl_profile_tab[PROFILE_NUM];

#ifdef __cplusplus
};
#endif

#endif