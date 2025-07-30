#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "driver/adc.h"

#define TAG "SAMPLER"

#define SAMPLER_SERVICE_UUID   0x00FF
#define STREAM_CHAR_UUID       0xFF01
#define GATTS_NUM_HANDLE       4
#define DEVICE_NAME            "BLE_Sampler"

#define SAMPLE_COUNT 10
#define ADC_CHANNEL  ADC1_CHANNEL_0

static uint16_t service_handle;
static uint16_t char_handle;
static esp_gatt_if_t global_gatts_if;
static uint16_t conn_id = 0;
static bool is_connected = false;

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

void sample_adc_bundle_and_notify(uint16_t conn_id, uint16_t char_handle, esp_gatt_if_t gatts_if)
{
    static bool adc_initialized = false;
    uint16_t samples[SAMPLE_COUNT];
    uint8_t payload[SAMPLE_COUNT * 2];

    if (!adc_initialized) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_11);
        adc_initialized = true;
    }

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        samples[i] = adc1_get_raw(ADC_CHANNEL);
        payload[2 * i] = samples[i] & 0xFF;
        payload[2 * i + 1] = (samples[i] >> 8) & 0xFF;
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    esp_ble_gatts_send_indicate(gatts_if, conn_id, char_handle, sizeof(payload), payload, false);
    ESP_LOGI(TAG, "Sent ADC sample bundle via notify");
}

void adc_stream_task(void *param)
{
    while (is_connected) {
        sample_adc_bundle_and_notify(conn_id, char_handle, global_gatts_if);
    }
    vTaskDelete(NULL);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    if (event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT) {
        esp_ble_gap_start_advertising(&adv_params);
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                 esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT: {
        esp_ble_gap_set_device_name(DEVICE_NAME);
        esp_ble_gap_config_adv_data(&adv_data);

        esp_gatt_srvc_id_t service_id = {
            .is_primary = true,
            .id.inst_id = 0,
            .id.uuid.len = ESP_UUID_LEN_16,
            .id.uuid.uuid.uuid16 = SAMPLER_SERVICE_UUID,
        };
        esp_ble_gatts_create_service(gatts_if, &service_id, GATTS_NUM_HANDLE);
        break;
    }
    case ESP_GATTS_CREATE_EVT: {
        service_handle = param->create.service_handle;
        esp_ble_gatts_start_service(service_handle);

        esp_bt_uuid_t char_uuid = {
            .len = ESP_UUID_LEN_16,
            .uuid.uuid16 = STREAM_CHAR_UUID,
        };
        esp_gatt_char_prop_t prop = ESP_GATT_CHAR_PROP_BIT_NOTIFY;

        esp_ble_gatts_add_char(service_handle, &char_uuid,
                               ESP_GATT_PERM_READ,
                               prop, NULL, NULL);
        break;
    }
    case ESP_GATTS_ADD_CHAR_EVT: {
        char_handle = param->add_char.attr_handle;
            esp_bt_uuid_t descr_uuid = {
        .len = ESP_UUID_LEN_16,
        .uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,
       };

        esp_gatt_perm_t perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;

        esp_ble_gatts_add_char_descr(service_handle, &descr_uuid, perm, NULL, NULL);
        break;
    }
    case ESP_GATTS_CONNECT_EVT: {
        is_connected = true;
        conn_id = param->connect.conn_id;
        global_gatts_if = gatts_if;

        ESP_LOGI(TAG, "Client connected. Starting sampling task.");
        xTaskCreate(adc_stream_task, "adc_stream_task", 4096, NULL, 5, NULL);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT: {
        is_connected = false;
        esp_ble_gap_start_advertising(&adv_params);
        ESP_LOGI(TAG, "Client disconnected, restarted advertising.");
        break;
    }
    default:
        break;
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));
}
