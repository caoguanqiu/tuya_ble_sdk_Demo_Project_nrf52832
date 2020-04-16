
#include "tuya_ble_stdlib.h"
#include "tuya_ble_type.h"
#include "tuya_ble_heap.h"
#include "tuya_ble_mem.h"
#include "tuya_ble_api.h"
#include "tuya_ble_port.h"
#include "tuya_ble_main.h"
#include "tuya_ble_secure.h"
#include "tuya_ble_data_handler.h"
#include "tuya_ble_storage.h"
#include "tuya_ble_sdk_version.h"
#include "tuya_ble_utils.h"
#include "tuya_ble_event.h"
#include "tuya_ble_app_demo.h"
#include "tuya_ble_log.h"
#include "ota.h"

tuya_ble_device_param_t device_param = {0};


static const char auth_key_test[] = "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy";
static const char device_id_test[] = "zzzzzzzzzzzzzzzz";

#define APP_CUSTOM_EVENT_1  1
#define APP_CUSTOM_EVENT_2  2
#define APP_CUSTOM_EVENT_3  3
#define APP_CUSTOM_EVENT_4  4
#define APP_CUSTOM_EVENT_5  5


static uint8_t dp_data_array[255+3];
static uint16_t dp_data_len = 0;

typedef struct {
    uint8_t data[50];
} custom_data_type_t;

void custom_data_process(int32_t evt_id,void *data)
{
    custom_data_type_t *event_1_data;
    TUYA_BLE_LOG_DEBUG("custom event id = %d",evt_id);
    switch (evt_id)
    {
        case APP_CUSTOM_EVENT_1:
            event_1_data = (custom_data_type_t *)data;
            TUYA_BLE_LOG_HEXDUMP_DEBUG("received APP_CUSTOM_EVENT_1 data:",event_1_data->data,50);
            break;
        case APP_CUSTOM_EVENT_2:
            break;
        case APP_CUSTOM_EVENT_3:
            break;
        case APP_CUSTOM_EVENT_4:
            break;
        case APP_CUSTOM_EVENT_5:
            break;
        default:
            break;
  
    }
}

custom_data_type_t custom_data;

void custom_evt_1_send_test(uint8_t data)
{    
    tuya_ble_custom_evt_t event;
    
    for(uint8_t i=0; i<50; i++)
    {
        custom_data.data[i] = data;
    }
    event.evt_id = APP_CUSTOM_EVENT_1;
    event.custom_event_handler = (void *)custom_data_process;
    event.data = &custom_data;
    tuya_ble_custom_event_send(event);
}


static void tuya_cb_handler(tuya_ble_cb_evt_param_t* event)
{
    int16_t result = 0;
    switch (event->evt)
    {
    case TUYA_BLE_CB_EVT_CONNECTE_STATUS:
        TUYA_BLE_LOG_INFO("received tuya ble conncet status update event,current connect status = %d",event->connect_status);
        break;
    case TUYA_BLE_CB_EVT_DP_WRITE:
        dp_data_len = event->dp_write_data.data_len;
        memset(dp_data_array,0,sizeof(dp_data_array));
        memcpy(dp_data_array,event->dp_write_data.p_data,dp_data_len);        
        TUYA_BLE_LOG_HEXDUMP_DEBUG("received dp write data :",dp_data_array,dp_data_len);
        tuya_ble_dp_data_report(dp_data_array,dp_data_len);
        //custom_evt_1_send_test(dp_data_len);
        break;
    case TUYA_BLE_CB_EVT_DP_DATA_REPORT_RESPONSE:
        TUYA_BLE_LOG_INFO("received dp data report response result code =%d",event->dp_response_data.status);
        break;
    case TUYA_BLE_CB_EVT_DP_DATA_WTTH_TIME_REPORT_RESPONSE:
        TUYA_BLE_LOG_INFO("received dp data report response result code =%d",event->dp_response_data.status);
        break;
    case TUYA_BLE_CB_EVT_UNBOUND:
        
        TUYA_BLE_LOG_INFO("received unbound req");

        break;
    case TUYA_BLE_CB_EVT_ANOMALY_UNBOUND:
        
        TUYA_BLE_LOG_INFO("received anomaly unbound req");

        break;
    case TUYA_BLE_CB_EVT_DEVICE_RESET:
        
        TUYA_BLE_LOG_INFO("received device reset req");

        break;
    case TUYA_BLE_CB_EVT_DP_QUERY:
        TUYA_BLE_LOG_INFO("received TUYA_BLE_CB_EVT_DP_QUERY event");
        tuya_ble_dp_data_report(dp_data_array,dp_data_len);
        break;
    case TUYA_BLE_CB_EVT_OTA_DATA:
        tuya_ota_proc(event->ota_data.type,event->ota_data.p_data,event->ota_data.data_len);
        break;
    case TUYA_BLE_CB_EVT_NETWORK_INFO:
        TUYA_BLE_LOG_INFO("received net info : %s",event->network_data.p_data);
        tuya_ble_net_config_response(result);
        break;
    case TUYA_BLE_CB_EVT_WIFI_SSID:

        break;
    case TUYA_BLE_CB_EVT_TIME_STAMP:
        TUYA_BLE_LOG_INFO("received unix timestamp : %s ,time_zone : %d",event->timestamp_data.timestamp_string,event->timestamp_data.time_zone);
        break;
    case TUYA_BLE_CB_EVT_TIME_NORMAL:

        break;
    case TUYA_BLE_CB_EVT_DATA_PASSTHROUGH:
        TUYA_BLE_LOG_HEXDUMP_DEBUG("received ble passthrough data :",event->ble_passthrough_data.p_data,event->ble_passthrough_data.data_len);
        tuya_ble_data_passthrough(event->ble_passthrough_data.p_data,event->ble_passthrough_data.data_len);
        break;
    default:
        TUYA_BLE_LOG_WARNING("app_tuya_cb_queue msg: unknown event type 0x%04x",event->evt);
        break;
    }
}




void tuya_ble_app_init(void)
{
    device_param.device_id_len = 16;
    memcpy(device_param.auth_key,(void *)auth_key_test,AUTH_KEY_LEN);
    memcpy(device_param.device_id,(void *)device_id_test,DEVICE_ID_LEN);
    device_param.p_type = TUYA_BLE_PRODUCT_ID_TYPE_PID;
    device_param.product_id_len = 8;
    memcpy(device_param.product_id,APP_PRODUCT_ID,8);
    device_param.firmware_version = TY_APP_VER_NUM;
    device_param.hardware_version = TY_HARD_VER_NUM;

    tuya_ble_sdk_init(&device_param);
    tuya_ble_callback_queue_register(tuya_cb_handler);

    tuya_ota_init();

    TUYA_BLE_LOG_INFO("app version : "TY_APP_VER_STR);

}





