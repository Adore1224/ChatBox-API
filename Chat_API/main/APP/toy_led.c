/*****************************************INCLUDE************************************************/
#include "toy_led.h"
/******************************************宏定义************************************************/

/*****************************************全局参数***********************************************/

/*****************************************局部参数***********************************************/

/***************************************局部函数声明*********************************************/

/*****************************************函数实现**********************************************/
/**
 * @brief       初始化LED
 * @param       无
 * @retval      无
 */
void led_init(void)
{
    //配置led灯GPIO
    gpio_config_t led_io_cfg = {
       .pin_bit_mask = (1ULL << LED_GPIO_PIN), //设置引脚的位掩码
       .mode =  GPIO_MODE_INPUT_OUTPUT,        //输入输出模式
       .pull_up_en = GPIO_PULLUP_ENABLE,       //启用上拉电阻
       .intr_type = GPIO_INTR_DISABLE,         //禁用中断
    };
    gpio_config(&led_io_cfg);  
    LED(1);  //关闭LED
}
