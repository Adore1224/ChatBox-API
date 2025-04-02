#ifndef __TOY_WIFI_H_
#define __TOY_WIFI_H_

/*****************************************INCLUDE************************************************/
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_eap_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_err.h"
#include "toy_https.h"
#include "toy_oled.h"
/*****************************************全局定义***********************************************/
#define CONNECTED_BIT  (1 << 0)
/*****************************************全局函数**********************************************/
void wifi_init(void);
size_t read_nvs_ssid(char* ssid,int maxlen);
esp_err_t write_nvs_ssid(char* ssid);
size_t read_nvs_password(char* pwd,int maxlen);
esp_err_t write_nvs_password(char* pwd);

#endif
