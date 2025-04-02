#ifndef __TOY_LED_H_
#define __TOY_LED_H_

/*****************************************INCLUDE************************************************/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/timers.h"
/*****************************************全局定义***********************************************/
#define LED_GPIO_PIN    GPIO_NUM_1  /* LED连接的GPIO端口 */
/* 引脚的输出的电平状态 */
enum GPIO_OUTPUT_STATE
{
    PIN_RESET,
    PIN_SET
};
/* LED端口定义 */
#define LED(x)          do { x ?                                      \
                             gpio_set_level(LED_GPIO_PIN, PIN_SET) :  \
                             gpio_set_level(LED_GPIO_PIN, PIN_RESET); \
                        } while(0)  
/* LED取反定义 */
#define LED_TOGGLE()    do { gpio_set_level(LED_GPIO_PIN, !gpio_get_level(LED_GPIO_PIN)); } while(0)  
/*****************************************全局函数**********************************************/
void led_init(void); 

#endif
