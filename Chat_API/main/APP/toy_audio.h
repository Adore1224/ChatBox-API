 #ifndef __TOY_AUDIO_H_
#define __TOY_AUDIO_H_

/*****************************************INCLUDE************************************************/
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "toy_websocket.h"
#include "toy_fatfs.h"
#include "toy_oled.h"
/*****************************************全局定义***********************************************/
#define I2S_IN_PORT      I2S_NUM_0
#define I2S_OUT_PORT     I2S_NUM_1
/*****************************************全局函数**********************************************/
void audiohw_init(void);
void offline_audio_play(const char *file);

#endif
