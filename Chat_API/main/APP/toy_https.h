#ifndef __TOY_HTTP_WEBSK_H
#define __TOY_HTTP_WEBSK_H

/*****************************************INCLUDE************************************************/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "toy_wifi.h"
#include "toy_websocket.h"
/*****************************************全局参数***********************************************/

/*****************************************全局函数**********************************************/
void https_init(void);

#endif
