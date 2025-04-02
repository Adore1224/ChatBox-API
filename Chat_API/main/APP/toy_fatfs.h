#ifndef __TOY_FATFS_H
#define __TOY_FATFS_H

/*****************************************INCLUDE************************************************/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "esp_partition.h"
#include "toy_audio.h"
/*****************************************全局定义***********************************************/

/*****************************************全局函数**********************************************/
//void format_fatfs(void);
void fatfs_init(void);

#endif
