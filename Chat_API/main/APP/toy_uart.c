/*****************************************INCLUDE************************************************/
#include "toy_uart.h"
/******************************************宏定义************************************************/
#define UART_TASK_PRIO           5           /* 任务优先级 */
#define UART_STK_SIZE            4096        /* 任务堆栈大小 */
/*****************************************全局参数***********************************************/
extern TaskHandle_t audio_input_handle;
extern int vol;
extern double volume_ratio;
/*****************************************局部参数***********************************************/
static const char *TAG = "uart";
const char *wake_wav = "/storage/wake.wav"; //"我在呢" (开心)
static QueueHandle_t uart1_queue;
/***************************************局部函数声明*********************************************/
static void uart_rev_task(void *pvParameters);
/*****************************************函数实现**********************************************/
/** 串口初始化函数，串口0默认为日志调试输出串口，一般不用
 * @param baudrate 波特率
 * @return 无
*/ 
void uart1_init(uint32_t baudrate)
{
    uart_config_t uart1_cfg = {
        .baud_rate = baudrate,                 //波特率38400
        .data_bits = UART_DATA_8_BITS,         //数据位8位
        .parity = UART_PARITY_DISABLE,         //奇偶校验无
        .stop_bits = UART_STOP_BITS_1,         //停止位1位
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, //流控无
        .source_clk = UART_SCLK_DEFAULT        //时钟源
    };
    //安装串口驱动程序
    uart_driver_install(UART_NUM_1, 256, 0, 10, &uart1_queue, 0);
    ESP_LOGI(TAG, "uart init start");
    //根据以上配置初始化串口配置
    uart_param_config(UART_NUM_1, &uart1_cfg);
    //设置串口通信引脚映射，串口1的TX和RX映射到GPIO4、GPIO5，RTS和CTS未使用不设置
    uart_set_pin(UART_NUM_1, 4, 5, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    //为RTOS创建uart任务
    xTaskCreate(uart_rev_task, "uart_rev_task", UART_STK_SIZE, NULL, UART_TASK_PRIO, NULL);
    ESP_LOGI(TAG, "uart init done");
}
/** uart_rev任务，串口接收数据处理
 * @param pvParameters 参数，未使用
 * @return 无
*/
static void uart_rev_task(void *pvParameters) 
{
    (void)pvParameters;
    uart_event_t event;
    char* dtmp = (char*) malloc(16);
    //利用串口中断读取接收缓冲区数据
    while(1) {
        if (xQueueReceive(uart1_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, 16);
            switch (event.type) {
                case UART_DATA: //数据接收事件
                    uart_read_bytes(UART_NUM_1, dtmp, event.size, portMAX_DELAY);
                    if (strcmp(dtmp, "audio on") == 0) { //是audio on则启用音频输入
                        offline_audio_play(wake_wav); 
                        xTaskNotifyGive(audio_input_handle); //通知音频任务
                        break;
                    }
                    if (strcmp(dtmp, "vol add") == 0) { //是audio on则启用音频输入
                        if (vol < 3) {
                            vol += 1; //更新音量参数
                            volume_ratio = vol;     
                        } else {
                            //offline_audio_play(); //“对不起，已经是最大了”
                        }
                        break;
                    }
                    if (strcmp(dtmp, "vol sub") == 0) { //是audio on则启用音频输入
                        if (vol > 1) {
                            vol -= 1; //更新音量参数
                            volume_ratio = vol;
                        } else {
                            //offline_audio_play(); //“对不起，已经是最小了”
                        }
                        break;
                    }
                    break;
                case UART_FIFO_OVF: //FIFO溢出
                    uart_flush_input(UART_NUM_1);
                    xQueueReset(uart1_queue);
                    break;
                case UART_BUFFER_FULL: //缓冲区满
                    uart_flush_input(UART_NUM_1);
                    xQueueReset(uart1_queue);
                    break;
                default: break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL); //删除本任务
}
