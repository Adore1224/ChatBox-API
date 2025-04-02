/*****************************************INCLUDE************************************************/
#include "toy_button.h"
/******************************************宏定义************************************************/
#define BTN1_TASK_PRIO           12          /* 任务优先级 */
#define BTN1_STK_SIZE            2560        /* 任务堆栈大小 */
#define DEBOUNCE_TIME          15          //消抖时间阈值
#define LONG_PRESS_TIME        500        //长按的时间阈值
#define BTN1_GPIO_PIN          GPIO_NUM_39
/*****************************************全局参数***********************************************/
extern TaskHandle_t audio_input_handle;   
extern bool speak_done;
/*****************************************局部参数***********************************************/
const char *TAG = "button";
TimerHandle_t button_timer_handle; //定时器句柄
static int64_t press_start = 0; //记录按键按下时间
static int press_duration = 0; //按键按下的持续时间
static bool button_pressed = false; //按键状态标志
/***************************************局部函数声明*********************************************/
static void button_timer_callback(TimerHandle_t xTimer);
static void button1_task(void *pvParameters);
/*****************************************函数实现**********************************************/
/**
 * @brief 按键检测定时器回调函数，在这里进行按键状态检测、消抖、长按和短按判断等操作
 * @param xTimer 定时器句柄
 * @return 无
 */
static void button_timer_callback(TimerHandle_t xTimer)
{
    bool current_state = gpio_get_level(BTN1_GPIO_PIN);
    if(current_state == 0) {
        //按键按下
        if (!button_pressed) {
            button_pressed = true;
            press_start = esp_timer_get_time();
        }
    } else {
        //按键松开
        if(!button_pressed) {
            return;
        } else {
            button_pressed = false;
            press_duration = (esp_timer_get_time() - press_start) / 1000; //换算为毫秒
            if (press_duration >= LONG_PRESS_TIME) {
                xTaskCreate(button1_task, "button1_task", BTN1_STK_SIZE, NULL, BTN1_TASK_PRIO, NULL); 
            }
            press_duration = 0;
            press_start = 0;
        }
    }
}
/**
 * @brief btn1的初始化，创建事件组、配置按键GPIO、创建定时器等操作
 * @param 无
 * @return 无
 */
void blebtn_init(void) 
{
    //配置按键GPIO
    gpio_config_t button_io_cfg = {
       .pin_bit_mask = (1ULL << BTN1_GPIO_PIN),
       .mode = GPIO_MODE_INPUT,               //输入模式
       .pull_up_en = GPIO_PULLUP_ENABLE,      //启用上拉
       .pull_down_en = GPIO_PULLDOWN_DISABLE, //禁用下拉
       .intr_type = GPIO_INTR_DISABLE,        //禁用中断
    };
    gpio_config(&button_io_cfg);
    //创建定时器
    button_timer_handle = xTimerCreate("Button_Timer", pdMS_TO_TICKS(200), pdTRUE, NULL, button_timer_callback);
    if (button_timer_handle!= NULL) {
        xTimerStart(button_timer_handle, 0);
    }
}
/**
 * @brief btn1任务，启动蓝牙
 * @param pvParameters 参数，未使用
 * @return 无
 */
static void button1_task(void *pvParameters)
{
    (void)pvParameters;
    ble_start(); //按键长按时启用蓝牙并进入配网程序
    vTaskDelete(NULL); //删除本任务
}
