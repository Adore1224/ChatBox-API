/*****************************************INCLUDE************************************************/
#include "toy_ble.h"
/******************************************宏定义************************************************/
#define BLE_TASK_PRIO       11          /* 任务优先级 */
#define BLE_STK_SIZE        2560        /* 任务堆栈大小 */
#define GATTS_SERVICE_UUID    0x00FF
#define GATTS_CHAR_UUID       0xFF01
#define GATTS_NUM_HANDLE      4
#define GAP_APP_ID            1
/*****************************************全局参数***********************************************/
extern wifi_config_t wifi_config;
extern TimerHandle_t button_timer_handle;
/*****************************************局部参数***********************************************/
static uint16_t service_handle = 0;
static uint16_t conn_id = 0;
static esp_gatt_if_t gatts_if = 0;
static EventGroupHandle_t ble_event_group;
static const char* TAG = "ble";
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x0006, 
    .max_interval        = 0x0010, 
    .appearance          = 0x00,
    .manufacturer_len    = 0,    
    .p_manufacturer_data = NULL, 
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x60,
    .adv_int_max        = 0x80,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};
/***************************************局部函数声明*********************************************/
static void set_blename_dyn(const char *prefix);
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void ble_task(void *pvParameters);
/*****************************************函数实现**********************************************/
/** 动态设置蓝牙名称
 * @param 无
 * @return 无
*/
static void set_blename_dyn(const char *prefix)
{
    //获取本机MAC地址
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    //动态生成设备名称
    char device_name[10]; 
    snprintf(device_name, sizeof(device_name), "%s_%02X%02X", prefix, mac[4], mac[5]); //拼接前缀和MAC地址后两字节
    //设置蓝牙设备名称
    esp_ble_gap_set_device_name(device_name);
}
/** 蓝牙GATT层回调函数
 * @param  ***
 * @return 无
*/
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if_param, 
                                 esp_ble_gatts_cb_param_t *param) 
{
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            set_blename_dyn("Toy");
            esp_ble_gap_config_adv_data(&adv_data);
            esp_gatt_srvc_id_t service_id = {
                .is_primary = true,
                .id.inst_id = 0x00,
                .id.uuid.len = ESP_UUID_LEN_16,
                .id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID,
            };
            esp_ble_gatts_create_service(gatts_if_param, &service_id, GATTS_NUM_HANDLE);
            gatts_if = gatts_if_param;  //保存接口
            break;
        }
        case ESP_GATTS_CREATE_EVT: {
            service_handle = param->create.service_handle;
            esp_ble_gatts_start_service(service_handle);
            esp_bt_uuid_t char_uuid = {
                .len = ESP_UUID_LEN_16,
                .uuid.uuid16 = GATTS_CHAR_UUID,
            };
            esp_ble_gatts_add_char(service_handle, &char_uuid, 
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                NULL, NULL);
            break;
        }
        case ESP_GATTS_WRITE_EVT: {
            if (param->write.len > 0 && param->write.len < 128) {
                ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT, data_len=%d", param->write.len);
                char* data = strndup((char*)param->write.value, param->write.len);
                if (data) {
                    char* ssid = strtok(data, ",");
                    char* password = strtok(NULL, ",");
                    if (ssid && password) {
                        ESP_LOGI(TAG, "SSID: %s", ssid);
                        ESP_LOGI(TAG, "PASSWORD: %s", password);
                        snprintf((char*)wifi_config.sta.ssid,32,"%s",ssid);  //格式化并保存到wifi配置参数中
                        snprintf((char*)wifi_config.sta.password,64,"%s",password);
                        xEventGroupSetBits(ble_event_group, BLE_GET_BIT);
                        write_nvs_ssid(ssid); //将ssid写入NVS
                        write_nvs_password(password); //将password写入NVS
                        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));   
                        ESP_ERROR_CHECK(esp_wifi_connect()); 
                    } else {
                        ESP_LOGE(TAG, "Invalid data format");
                    }
                    free(data);
                }
            }
            break;
        }
        case ESP_GATTS_CONNECT_EVT: {
            ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT");
            conn_id = param->connect.conn_id;
            break;
        }
        case ESP_GATTS_DISCONNECT_EVT: {
            //获取BLE_GET_BIT标志位是否被设置，获取后不清除标志位
            EventBits_t uxBits = xEventGroupGetBits(ble_event_group);
            if (uxBits & BLE_GET_BIT) {
                //设置完成标志，在任务中同步关闭ble资源
                xEventGroupSetBits(ble_event_group, BLE_DONE_BIT);
            } else {
                //未设置则视为意外断开，重新启动ble广播
                ESP_LOGI(TAG, "Disconnected , restarting");
                esp_ble_gap_start_advertising(&adv_params); 
            }
            break;
        }
        default:
            break;
    }
}
/** 蓝牙初始化并开启蓝牙
 * @param 无
 * @return 无
*/
void ble_start(void)
{
    esp_wifi_disconnect(); //断开当前wifi连接   
    ble_event_group = xEventGroupCreate(); //创建ble事件组
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)); //释放经典蓝牙栈
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable(); 
    //注册GATT服务器事件处理函数
    esp_ble_gatts_register_callback(gatts_event_handler);
    //注册GAP回调
    esp_ble_gap_register_callback(NULL);
    //注册GATT应用程序
    esp_ble_gatts_app_register(GAP_APP_ID); 
    //开始广播
    esp_ble_gap_start_advertising(&adv_params); 
    //为RTOS创建BLE处理任务
    xTaskCreate(ble_task, "ble_task", BLE_STK_SIZE, NULL, BLE_TASK_PRIO, NULL);
    oled_show_ble();
}
/** 蓝牙关闭
 * @param 无
 * @return 无
*/
void ble_stop(void)
{
    //停止蓝牙广播
    esp_ble_gap_stop_advertising();
    //禁用蓝牙堆栈
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    //禁用蓝牙控制器
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    //释放蓝牙控制器内存
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    oled_clear_ble();
}
/** ble任务，关闭蓝牙 
 * @param pvParameters 参数，未使用
 * @return 无
 */
static void ble_task(void *pvParameters)
{
    (void)pvParameters;
    while(1) {
        //阻塞等待ble_event_group事件组标志位，获取后清除标志位
        EventBits_t uxBits = xEventGroupWaitBits(ble_event_group, BLE_DONE_BIT, true, false, portMAX_DELAY);  
        if (uxBits & BLE_DONE_BIT) { 
            ble_stop(); //关闭蓝牙
            vTaskDelete(NULL); //删除本任务
        }
    }
}
