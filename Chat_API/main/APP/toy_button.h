#ifndef __TOY_BUTTON_H_
#define __TOY_BUTTON_H_

/*****************************************INCLUDE************************************************/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "toy_ble.h"
#include "toy_websocket.h"
/*****************************************全局定义***********************************************/

/*****************************************全局函数**********************************************/
void blebtn_init(void);

#endif
