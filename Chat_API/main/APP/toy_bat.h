#ifndef __TOY_BAT_H
#define __TOY_BAT_H

/*****************************************INCLUDE************************************************/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "toy_oled.h"
/*****************************************全局定义***********************************************/

/*****************************************全局函数**********************************************/
void bat_ic_init(void);

#endif
