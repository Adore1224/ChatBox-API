/**********************工程声明************************/
/*        Declare: Written By WPC, 2024.11.25 
          Program Version: Ver1.0                    */
/*****************************************************/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "toy_fatfs.h"
#include "toy_led.h"
#include "toy_button.h"
#include "toy_oled.h"
#include "toy_uart.h"
#include "toy_audio.h"
#include "toy_bat.h"
#include "toy_wifi.h"
#include "toy_ble.h"
#include "toy_websocket.h"
#include "toy_https.h"


/* 程序入口(主)函数 */
void app_main(void)
{
    esp_err_t ret;
    ret = nvs_flash_init(); //初始化NVS系统
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    } //以上代码为ESP-IDF固定形式代码，检查芯片Flash有无问题
    /* 注：系统通过RTOS任务机制以及ESP事件机制共同实现多线程处理 */
    esp_event_loop_create_default(); //创建默认的事件循环，处理事件
    fatfs_init(); //初始化挂载fat文件系统
    //led_init(); //初始化led，暂无可编程LED
    blebtn_init(); //初始化蓝牙button
    wifi_init(); //初始化wifi
    uart1_init(38400); //初始化串口1，与ASRPRO芯片通信
    oled_init(); //初始化oled显示屏
    bat_ic_init(); //初始化电量监测芯片
    audiohw_init(); //初始化音频i2s通信驱动
    ws_depend_init(); //初始化websocket通信相关依赖参数
    https_init(); //初始化https通信
}
