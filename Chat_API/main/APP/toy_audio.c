/*****************************************INCLUDE************************************************/
#include "toy_audio.h"
/******************************************宏定义************************************************/
#define AUDIO_INPUT_TASK_PRIO      9           /* 任务优先级 */
#define AUDIO_INPUT_STK_SIZE       14336       /* 任务堆栈大小 */
TaskHandle_t audio_input_handle = NULL;        /* 任务句柄 */
#define ICS43434_WS      GPIO_NUM_21  
#define ICS43434_SCK     GPIO_NUM_47  
#define ICS43434_SD      GPIO_NUM_48  
#define MAX98357_LRC     GPIO_NUM_11 
#define MAX98357_BCLK    GPIO_NUM_12  
#define MAX98357_DIN     GPIO_NUM_13 

#define SAMPLE_RATE         16000      //i2s通信采样率
#define NOISE_THRESHOLD     15         //噪声阈值
#define NO_VOICE_TIMEOUT_MS 500       //停止讲话时间阈值ms
/*****************************************全局参数***********************************************/
extern bool is_wifi_connect; //wifi连接标志位
extern bool is_stt_end; //stt语音识别完成标志位
bool speak_done; //讲话完成标志位
int vol = 1; //离线音频默认音量大小
/*****************************************局部参数***********************************************/
static const char *TAG = "audio";
size_t buffer_index = 0; 
int16_t silence_audio[32] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
}; //16位单声道静音PCM数据
const char *start_wav = "/storage/start.wav"; //"你好呀，我叫小花，是你的小助手哦，有什么能帮到你的吗"
const char *net_wav = "/storage/network.wav"; //"糟糕，网络有点问题,你给我联网了吗"
const char *bat_wav = "/storage/bat.wav"; //"我要没电啦，要尽快给我充电呀"
const char *stop_wav = "/storage/stop.wav"; //"拜拜，有事儿记得叫我"
/***************************************局部函数声明*********************************************/
static void audio_input_task(void *pvParameters);
/*****************************************函数实现**********************************************/
/** 播放保存在fat文件系统中的离线音频
 * @param *file 文件系统中保存的音频文件指针
 * @return 无
 */
void offline_audio_play(const char *file)
{
    char audio_buf[1024];
    size_t bytes_read;
    size_t w_bytes = 0;
    FILE *wav_file = fopen(file, "r");
    i2s_stop(I2S_IN_PORT);
    if (wav_file) {
        fseek(wav_file, 20, SEEK_SET);
        while ((bytes_read = fread(audio_buf, 1, sizeof(audio_buf), wav_file)) > 0) {
            if(vol!=1) { //离线音频音量调节
                for (size_t i = 0; i < bytes_read; i += 2) {
                    //读取16位采样值
                    int16_t sample = ((uint16_t)audio_buf[i + 1] << 8) | (uint16_t)audio_buf[i];
                    //应用音量系数
                    sample = (int16_t)(sample * vol);
                    //将调整后的采样值写回缓冲区
                    audio_buf[i] = (uint8_t)sample;
                    audio_buf[i + 1] = (uint8_t)(sample >> 8);
                }
            }          
            i2s_write(I2S_OUT_PORT, audio_buf, bytes_read, &w_bytes, pdMS_TO_TICKS(100)); //写入i2s播放
        }
        fclose(wav_file);
    } else {
        ESP_LOGE(TAG, "Failed to open WAV file");
    }
    //清空DMA缓冲区并停止I2S
    i2s_zero_dma_buffer(I2S_OUT_PORT);
    i2s_start(I2S_IN_PORT);
} 
/** 音频模块相关硬件初始化：ics43434麦克风-外设i2s0，max98357音频解码放大器-外设i2s1
 * @param 无
 * @return 无
 */
void audiohw_init(void)
{
    speak_done = false;
    //ICS43434
    i2s_config_t i2s0_cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 512,
        .use_apll = false
    };
    i2s_pin_config_t ics_pin_cfg = {
        .bck_io_num = ICS43434_SCK,
        .ws_io_num = ICS43434_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = ICS43434_SD
    };
    i2s_driver_install(I2S_IN_PORT, &i2s0_cfg, 0, NULL);
    i2s_set_pin(I2S_IN_PORT, &ics_pin_cfg);
    //MAX98357
    i2s_config_t i2s1_cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 512,
        .use_apll = false
    };
    i2s_pin_config_t max_pin_cfg = {
        .bck_io_num = MAX98357_BCLK,
        .ws_io_num = MAX98357_LRC,
        .data_out_num = MAX98357_DIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    i2s_driver_install(I2S_OUT_PORT, &i2s1_cfg, 0, NULL);
    i2s_set_pin(I2S_OUT_PORT, &max_pin_cfg); 
    //为RTOS创建audio任务
    xTaskCreate(audio_input_task, "audio_input_task", AUDIO_INPUT_STK_SIZE, NULL, AUDIO_INPUT_TASK_PRIO, &audio_input_handle);
    offline_audio_play(start_wav);
}
/**
 * @param pvParameters 参数，未使用
 * @return 无
 */
static void audio_input_task(void *pvParameters) 
{
    (void)pvParameters;
    int16_t data[1600];
    while (1) { 
        uint32_t notify_value = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  
        if (notify_value > 0) { //被唤醒
            if (!is_wifi_connect) {
                offline_audio_play(net_wav);
                continue;
            }
            send_stt_firstreq();
            oled_show_hear();
            size_t bytes_read = 0; //当次读取的数据长度以及最后的数据总长
            size_t no_voice_total = 0, voice_count = 0, no_voice_pre = 0, no_voice_cur = 0; //判定说话是否结束的关键数据
            bool speaking = true; //讲话状态标志位：true-正在讲话；false-讲话结束
            while (speaking) { 
                no_voice_pre = esp_timer_get_time() / 1000; //获取开始时间ms
                i2s_read(I2S_IN_PORT, data, sizeof(data), &bytes_read, portMAX_DELAY); //读取当次数据
                uint32_t sum_data = 0; 
                //通过遍历当次读取的每个样本数据的绝对值大小来计算音量平均值
                for (int i = 0; i < bytes_read / 2; i++) {
                    sum_data += abs(data[i]);
                }
                sum_data = sum_data / bytes_read;
                no_voice_cur = esp_timer_get_time() / 1000; //获取当前时间ms
                if (sum_data < NOISE_THRESHOLD) { //如果音量平均值小于阈值则判定为无声
                    no_voice_total += no_voice_cur - no_voice_pre; //记录静音时长
                } else { //有声音输入则清除之前记录的静音时长，有效声计数+1
                    no_voice_total = 0;
                    voice_count++;
                    send_stt_audioreq(data, bytes_read, false);
                }
                //当静音总时长大于1000ms或者记录的数据总长接近超过缓冲区的大小则停止录音
                if (no_voice_total > NO_VOICE_TIMEOUT_MS) {
                    speaking = false; //退出循环，结束讲话     
                }
            }
            if (voice_count != 0) { //唤醒后如果有效声计数不为0，则表示有过讲话则会执行stt结束请求
                send_stt_audioreq(silence_audio, sizeof(silence_audio), true); //发送少量静音数据以填充结束请求                             
                is_stt_end = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    vTaskDelete(NULL); //删除任务
}
