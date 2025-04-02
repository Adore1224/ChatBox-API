#ifndef __TOY_BLE_H_
#define __TOY_BLE_H_

/*****************************************INCLUDE************************************************/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "esp_efuse.h"
#include "esp_mac.h"
#include "toy_wifi.h"
#include "toy_oled.h"
/*****************************************全局参数***********************************************/
#define BLE_GET_BIT        (1 << 0)
#define BLE_DONE_BIT       (1 << 1)
/*****************************************全局函数**********************************************/
void ble_start(void);
void ble_stop(void);


#endif
