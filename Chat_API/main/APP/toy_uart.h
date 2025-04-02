#ifndef __TOY_UART_H
#define __TOY_UART_H

/*****************************************INCLUDE************************************************/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "toy_audio.h"
#include "toy_audio.h"
/*****************************************全局定义***********************************************/

/*****************************************全局函数**********************************************/
void uart1_init(uint32_t baudrate);

#endif
