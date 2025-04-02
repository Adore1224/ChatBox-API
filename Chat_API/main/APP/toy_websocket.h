#ifndef __TOY_WEBSOCKET_H
#define __WEBSOCKET_H

/*****************************************INCLUDE************************************************/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "cJSON.h"
#include "esp_websocket_client.h"
#include "toy_wifi.h"
#include "toy_audio.h"
#include "toy_oled.h"
/*****************************************全局定义***********************************************/

/*****************************************全局函数***********************************************/
void get_mac_address(char *uid);
void send_stt_firstreq(void);
void send_stt_audioreq(int16_t *data, size_t bytes, bool is_last);
void send_tts_req(char *text);
void tts_client_init(void);
void stt_client_init(void);
void ws_depend_init(void);

#endif
