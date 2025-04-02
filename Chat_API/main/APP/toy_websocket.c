/*****************************************INCLUDE************************************************/
#include "toy_websocket.h"
/******************************************宏定义************************************************/

/*****************************************全局参数***********************************************/
extern bool is_wifi_connect; //wifi连接标志
extern const char wscert_start[] asm("_binary_ws_ca_cert_pem_start"); //CA证书信息起始地址
bool is_stt_end; //stt语音识别完成标志
QueueHandle_t stt_queue; //stt识别结果的传输队列
double volume_ratio = 0.5; //在线语音合成音频音量大小，与离线音频保持一致
/*****************************************局部参数***********************************************/
const char *listen_wav = "/storage/listen.wav"; //"对不起，我没听清，可以再说一次吗"
static const char AppID[] = ""; //控制台获取的应用的APP ID
static const char AccessToken[] = ""; //控制台获取的应用的Access Token
static esp_websocket_client_config_t ws_stt_cfg; 
static esp_websocket_client_config_t ws_tts_cfg;
static esp_websocket_client_handle_t stt_client = NULL;
static esp_websocket_client_handle_t tts_client = NULL;
static TaskHandle_t stt_stop_handle = NULL; //stt_stop任务句柄
static TaskHandle_t tts_stop_handle = NULL; //tts_stop任务句柄
char audio_uid[12]; //存储随机生成的uid
char end_text[1024] = {0}; //存储stt识别文本
static bool re_end = false; //tts合成完毕标志位
static uint8_t *audio_buffer = NULL; //用于存储tts生成音频
static size_t audio_buffer_size = 1024*1024; //tts音频数据缓冲区的总大小
static size_t audio_buffer_write_pos = 0; //tts音频数据缓冲区的当前写入位置
int16_t silence[32] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
}; //16位单声道静音PCM数据
/***************************************局部函数声明*********************************************/
void generate_uuid(char *reqid);
void get_wedsocket_header(uint8_t protocol_version, uint8_t header_size, uint8_t message_type,uint8_t flags, 
                                uint8_t serialization_method, uint8_t compression, uint8_t *header_bytes);
char *get_stt_fc_request(void);
static void process_stt_re(const char *data);
void play_tts_audio(void);
static void stt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void stt_stop_task(void *pvParameters);
char *get_tts_fc_request(char *text);
static void process_tts_re(const char *data, size_t len);
static void tts_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void tts_stop_task(void *pvParameters);
/*****************************************函数实现**********************************************/
/** 生成随机的uuid
 * @param 无
 * @return *reqid-生成的uuid;
*/
void generate_uuid(char *reqid) 
{
    uint8_t random[16];
    esp_fill_random(random, sizeof(random)); 
    //格式化十六进制字符串，去除连字符，每个字节转换为2位十六进制数
    int offset = 0;
    for (int i = 0; i < 16; i++) {
        offset += sprintf(reqid + offset, "%02x", random[i]);
    }
}
/** 获取mac地址并转为字符串
 * @param 无
 * @return *uid-生成的mac;
*/
void get_mac_address(char *uid)
{
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    int offset = 0;
    for (int i = 0; i < 6; i++) {
        offset += sprintf(uid + offset, "%02X", mac[i]);
    }
}
/** 构建WebSocket数据包的公共header部分
 * @param protocol_version-协议版本;eader_size-Header大小;message_type-消息类型;serialization_method-序列化方法;
 *        flags-消息类型补充信息; compression-压缩方法;
 * @return *header_bytes 构建好的header数据（大端格式）
 */
void get_wedsocket_header(uint8_t protocol_version, uint8_t header_size, uint8_t message_type,
                      uint8_t flags, uint8_t serialization_method, uint8_t compression, uint8_t *header_bytes) 
{
    //构建第1字节
    header_bytes[0] = (protocol_version << 4) | (header_size & 0x0F);
    //构建第2字节
    header_bytes[1] = (message_type << 4) | (flags & 0x0F);
    //构建第3字节
    header_bytes[2] = (serialization_method << 4) | (compression & 0x0F);
    //构建第4字节
    header_bytes[3] = 0x00;
}
/** 构建stt数据包的full_client_request请求的payload部分
 * @param 无
 * @return json_string json字符串
 */
char *get_stt_fc_request(void) 
{
    char stt_reqid[32]; 
    generate_uuid(stt_reqid); //每次调用重新动态生成stt
    //创建根对象
    cJSON *root = cJSON_CreateObject(); 
    if (root == NULL) {
        return NULL;
    }
    //构建"app"节点
    cJSON *app = cJSON_AddObjectToObject(root, "app");
    if (app == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddStringToObject(app, "appid", AppID);
    cJSON_AddStringToObject(app, "token", AccessToken); 
    cJSON_AddStringToObject(app, "cluster", "volcengine_input_common");
    //构建"user"节点
    cJSON *user = cJSON_AddObjectToObject(root, "user");
    if (user == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddStringToObject(user, "uid", audio_uid);
    //构建"audio"节点
    cJSON *audio = cJSON_AddObjectToObject(root, "audio");
    if (audio == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddStringToObject(audio, "format", "raw");
    cJSON_AddNumberToObject(audio, "rate", 16000);
    cJSON_AddNumberToObject(audio, "bits", 16);
    cJSON_AddNumberToObject(audio, "channel", 1);
    cJSON_AddStringToObject(audio, "language", "zh-CN");
    //构建"request"节点
    cJSON *request = cJSON_AddObjectToObject(root, "request");
    if (request == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddStringToObject(request, "reqid", stt_reqid);
    cJSON_AddStringToObject(request, "workflow", "audio_in,resample,partition,vad,fe,decode");
    cJSON_AddNumberToObject(request, "sequence", 1);
    cJSON_AddNumberToObject(request, "nbest", 1);
    cJSON_AddBoolToObject(request, "show_utterances", false);
    //序列化JSON数据为字符串
    char *json_string = cJSON_PrintUnformatted(root);
    if (json_string == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    //删除JSON对象，释放内存
    cJSON_Delete(root);
    return json_string;
}
/** websocket发送到stt的full_client_request请求
 * @param 无
 * @return 无
*/
void send_stt_firstreq(void)
{
    uint8_t audio_header[4];
    get_wedsocket_header(0b0001, 0b0001, 0b0001, 0b0000, 0b0001, 0b0000, audio_header);
    char *stt_full_payload = get_stt_fc_request();
    //计算payload size并转换为大端字节序
    uint32_t size = strlen(stt_full_payload);
    uint8_t payload_size[4];
    payload_size[0] = (size >> 24) & 0xff;
    payload_size[1] = (size >> 16) & 0xff;
    payload_size[2] = (size >> 8) & 0xff;
    payload_size[3] = size & 0xff;
    //构建数据包
    uint8_t *data_pkt = malloc(8 + size);
    memcpy(data_pkt, audio_header, 4);
    memcpy(data_pkt + 4, payload_size, 4);
    memcpy(data_pkt + 4 + 4, stt_full_payload, size);
    //发送数据包
    if (stt_client != NULL) {
        esp_websocket_client_send_bin(stt_client, (const char *)data_pkt, 8 + size, portMAX_DELAY);
    }
    //清理内存
    free(data_pkt);
}
/** websocket发送到stt的audio only request请求
 * @param *data-发送的数据地址；bytes-数据大小；is_last-是否是最后请求
 * @return 无
*/
void send_stt_audioreq(int16_t *data, size_t bytes, bool is_last) 
{
    uint8_t stt_header[4];
    uint8_t flags = is_last ? 0b0010 : 0b0000;  //根据is_last设置flags
    get_wedsocket_header(0b0001, 0b0001, 0b0010, flags, 0b0000, 0b0000, stt_header);
    //构建 payload 大小
    uint8_t payload_size[4];
    payload_size[0] = (bytes >> 24) & 0xff;
    payload_size[1] = (bytes >> 16) & 0xff;
    payload_size[2] = (bytes >> 8) & 0xff;
    payload_size[3] = bytes & 0xff;
    //分配临时缓冲区
    uint8_t *temp = malloc(8 + bytes);
    if (temp == NULL) {
        return;
    }
    //构建数据包
    memcpy(temp, stt_header, 4);         
    memcpy(temp + 4, payload_size, 4);   
    memcpy(temp + 8, data, bytes);  
    //发送 STT 数据包
    if (stt_client != NULL) {
        esp_websocket_client_send_bin(stt_client, (const char *)temp, 8 + bytes, portMAX_DELAY);
    }
    //释放临时缓冲区
    free(temp);
}
/** websocket处理stt server返回的响应内容
 * @param *data-接收到的数据地址
 * @return 无
*/
static void process_stt_re(const char *data) 
{
    //检查数据指针和长度
    if (data == NULL) {
        return;
    }
    //解析Header的各个字段
    /* 暂时无用的字段
    uint8_t message_type = (data[1] & 0xF0) >> 4;
    uint8_t compression_method = data[2] & 0x0F;
    ESP_LOGI(TAG, "sttMessage Type: 0x%X", message_type);
    ESP_LOGI(TAG, "sttCompression Method: 0x%X", compression_method); 
    */
    //提取JSON Payload起始位置和长度
    uint32_t payload_size = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
    const char *payload_data = (const char *)(data + 8);
    //解析JSON数据
    cJSON *root = cJSON_ParseWithLength(payload_data, payload_size);
    if (root == NULL) {
        return;
    }
    //提取"code"字段
    cJSON *code_json = cJSON_GetObjectItem(root, "code");
    if (code_json == NULL || !cJSON_IsNumber(code_json)) {
        cJSON_Delete(root);
        return;
    }
    int code = code_json->valueint;
    //如果请求成功，提取"result"字段
    if (code == 1000) {
        cJSON *result_json = cJSON_GetObjectItem(root, "result");
        if (result_json && cJSON_IsArray(result_json)) {
            int result_size = cJSON_GetArraySize(result_json);
            for (int i = 0; i < result_size; i++) {
                cJSON *item = cJSON_GetArrayItem(result_json, i);
                if (item) {
                    cJSON *text_json = cJSON_GetObjectItem(item, "text");
                    if (text_json && cJSON_IsString(text_json)) {
                        const char *stt_text = text_json->valuestring;
                        strncpy(end_text, stt_text, 512 - 1);
                        end_text[512 - 1] = '\0'; //确保字符串以'\0'结尾
                    } 
                }
            }
        } 
    }
    //释放JSON资源
    cJSON_Delete(root);
}
/** websocket事件处理函数，主要处理与stt服务通信的状态事件
 * @param 事件结构体
 * @return 无
*/
static void stt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *sdata = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI("stt", "stt connected");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        break;
    case WEBSOCKET_EVENT_CLOSED:
        break; 
    case WEBSOCKET_EVENT_DATA:
        if (sdata->op_code == 0x2) { 
            if(is_stt_end) {    
                process_stt_re(sdata->data_ptr);
                if (end_text[0] != '\0') { //存放识别文本缓冲区非空
                    ESP_LOGI("stt", "Recognized text: %s", end_text);
                    oled_show_think();
                    //转发chat服务
                    xQueueSend(stt_queue, end_text, portMAX_DELAY);
                    memset(end_text, 0, 512); //清空文本
                } else {
                    oled_show_welcome();
                    offline_audio_play(listen_wav);
                }
                is_stt_end = false;
                xTaskNotifyGive(stt_stop_handle); //通知stt_stop任务
            }
        } 
        break;
    default: break;
    }
}
/** 构建tts数据包的full_client_request请求的payload部分
 * @param *text-待转换文本
 * @return json_string json字符串
 */
char *get_tts_fc_request(char *text)
{
    char tts_reqid[32]; 
    generate_uuid(tts_reqid); //每次调用重新动态生成tts
    //创建根对象
    cJSON *root = cJSON_CreateObject(); 
    if (root == NULL) {
        return NULL;
    }
    //构建"app"节点
    cJSON *app = cJSON_AddObjectToObject(root, "app");
    if (app == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddStringToObject(app, "appid", AppID);
    cJSON_AddStringToObject(app, "token", "access_token");
    cJSON_AddStringToObject(app, "cluster", "volcano_tts");
    //构建"user"节点
    cJSON *user = cJSON_AddObjectToObject(root, "user");
    if (user == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddStringToObject(user, "uid", audio_uid);
    //构建"audio"节点
    cJSON *audio = cJSON_AddObjectToObject(root, "audio");
    if (audio == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddStringToObject(audio, "voice_type", "BV700_streaming"); //音色
    cJSON_AddStringToObject(audio, "encoding", "wav");
    cJSON_AddNumberToObject(audio, "rate", 16000);
    cJSON_AddNumberToObject(audio, "volume_ratio", volume_ratio); //音量
    cJSON_AddStringToObject(audio, "emotion", "lovey-dovey"); //语气风格
    cJSON_AddStringToObject(audio, "language", "cn");
    //构建"request"节点
    cJSON *request = cJSON_AddObjectToObject(root, "request");
    if (request == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddStringToObject(request, "reqid", tts_reqid);
    cJSON_AddStringToObject(request, "text", text);
    cJSON_AddStringToObject(request, "operation", "submit");
    cJSON_AddNumberToObject(request, "silence_duration", 15);
    //序列化JSON数据为字符串
    char *json_string = cJSON_PrintUnformatted(root);
    if (json_string == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    //删除JSON对象，释放内存
    cJSON_Delete(root);
    return json_string;
}
/** websocket发送到tts的full_client_request请求
 * @param *text 需要tts合成的文本数据地址
 * @return 无
*/
void send_tts_req(char *text)
{
    //初始化stt服务配置
    uint8_t audio_header[4];
    get_wedsocket_header( 0b0001, 0b0001, 0b0001, 0b0000, 0b0001, 0b0000, audio_header);
    char *tts_full_payload = get_tts_fc_request(text);
    //计算payload size并转换为大端字节序
    uint32_t size = strlen(tts_full_payload);
    uint8_t payload_size[4];
    payload_size[0] = (size >> 24) & 0xff;
    payload_size[1] = (size >> 16) & 0xff;
    payload_size[2] = (size >> 8) & 0xff;
    payload_size[3] = size & 0xff;
    //构建数据包
    uint8_t *data_pkt = malloc(sizeof(audio_header) + sizeof(payload_size) + size);
    //写入header
    memcpy(data_pkt, audio_header, sizeof(audio_header));
    //写入payload size
    memcpy(data_pkt + sizeof(audio_header), payload_size, sizeof(payload_size));
    //写入payload
    memcpy(data_pkt + sizeof(audio_header) + sizeof(payload_size), tts_full_payload, size);
    //发送数据包
    esp_websocket_client_send_bin(tts_client, (const char *)data_pkt, sizeof(audio_header) + sizeof(payload_size) + size, portMAX_DELAY);
    oled_show_speak();
    //清理内存
    free(data_pkt);
}
/** websocket处理tts server返回的响应内容
 * @param *data-tts服务器返回的数据；len-数据长度
 * @return 无
*/
static void process_tts_re(const char *data, size_t len) 
{
    if (audio_buffer_write_pos + len > audio_buffer_size) {
        len = audio_buffer_size - audio_buffer_write_pos; //截断数据
    }
    memcpy(audio_buffer + audio_buffer_write_pos, data, len);
    audio_buffer_write_pos += len;
    if (re_end!=true) {
        re_end=true;
    }
} 
/** 播放tts返回的音频数据
 * @param 无
 * @return 无
*/
void play_tts_audio(void)
{
    if (!audio_buffer || audio_buffer_write_pos == 0) {
        ESP_LOGW("tts", "No audio data to play");
        return;
    }
    size_t remaining_data = audio_buffer_write_pos; //缓冲区中剩余的数据量
    size_t offset = 0; //当前数据偏移量
    size_t chunk_size = 4096; //每次发送的块大小
    while (remaining_data > 0) {
        size_t bytes_to_send = (remaining_data > chunk_size) ? chunk_size : remaining_data;
        size_t bytes_written = 0;
        i2s_write(I2S_OUT_PORT, audio_buffer + offset, bytes_to_send, &bytes_written, portMAX_DELAY);
        offset += bytes_written;
        remaining_data -= bytes_written;
    }
    i2s_zero_dma_buffer(I2S_OUT_PORT);
    //播放完成后清空缓冲区
    audio_buffer_write_pos = 0;
}
/** websocket事件处理函数，主要处理与tts服务通信的状态事件
 * @param 事件结构体
 * @return 无
*/
static void tts_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *tdata = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI("tts", "tts connected");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            break;
        case WEBSOCKET_EVENT_CLOSED:
            break; 
        case WEBSOCKET_EVENT_DATA:
            if (tdata->op_code == 0x2) { 
                size_t bytes_written = 0;
                i2s_write(I2S_OUT_PORT, tdata->data_ptr, tdata->data_len, &bytes_written, portMAX_DELAY);
                re_end = true; //重置标志位
            } 
            else { 
                if (re_end) { //接收结束标志
                    i2s_zero_dma_buffer(I2S_OUT_PORT); //清空DMA缓冲区
                    oled_show_welcome();
                    re_end = false;
                    xTaskNotifyGive(tts_stop_handle); //通知tts_stop任务 
                }
            }
            /*
            if (tdata->op_code == 0x2) { 
                process_tts_re(tdata->data_ptr, tdata->data_len);
            } else {
                if (re_end) {
                    oled_show_speak();
                    play_tts_audio();
                    oled_show_welcome();
                    re_end = false;
                    xTaskNotifyGive(tts_stop_handle); //通知tts_stop任务       
                }      
            }*/
            break;
        default: break;
    }
}
/** tts服务的websocket client客户端初始化
 * @param 无
 * @return 无
*/
void stt_client_init(void)
{
    //如果已存在stt客户端，销毁旧实例
    if (stt_client != NULL) {
        esp_websocket_client_stop(stt_client);
        esp_websocket_client_destroy(stt_client);
        stt_client = NULL;
    }
    stt_client = esp_websocket_client_init(&ws_stt_cfg);
    //为websocket建立连接时发出的http get响应添加包含鉴权信息的请求头
    esp_websocket_client_append_header(stt_client, "Authorization", "Bearer;填入你的access token");
    //注册ws_stt事件处理函数
    esp_websocket_register_events(stt_client, WEBSOCKET_EVENT_ANY, stt_event_handler, (void *)stt_client);
    //启动websocket尝试与stt server建立连接
    esp_websocket_client_start(stt_client);
}
/** tts服务的websocket client客户端初始化
 * @param 无
 * @return 无
*/
void tts_client_init(void)
{
    //如果已存在tts客户端，销毁旧实例
    if (tts_client != NULL) {
        esp_websocket_client_stop(tts_client);
        esp_websocket_client_destroy(tts_client);
        tts_client = NULL;
    }
    tts_client = esp_websocket_client_init(&ws_tts_cfg);
    //为websocket建立连接时发出的http get响应添加包含鉴权信息的请求头
    esp_websocket_client_append_header(tts_client, "Authorization", "Bearer;填入你的access token");
    //注册ws_tts事件处理函数
    esp_websocket_register_events(tts_client, WEBSOCKET_EVENT_ANY, tts_event_handler, (void *)tts_client);
    //启动websocket尝试与tts server建立连接
    esp_websocket_client_start(tts_client);
}
/** websocket应用依赖参数的初始化
 * @param 无
 * @return 无
*/
void ws_depend_init(void)
{
    is_stt_end = false; //标志发送到stt服务的请求是否是最后一个
    stt_queue = xQueueCreate(4, 512); //语音识别转换结果的传递载体
    audio_buffer = heap_caps_malloc(1024*1024*2, MALLOC_CAP_SPIRAM);
    get_mac_address(audio_uid); //生成一次性的mac地址作为uid
    ws_stt_cfg = (esp_websocket_client_config_t ) {
        .uri = "wss://openspeech.bytedance.com/api/v2/asr",
        .reconnect_timeout_ms = 10000,
        .network_timeout_ms = 10000,
        .cert_pem = wscert_start, //CA证书
    };
    ws_tts_cfg = (esp_websocket_client_config_t) { 
        .uri = "wss://openspeech.bytedance.com/api/v1/tts/ws_binary",
        .reconnect_timeout_ms = 10000,
        .network_timeout_ms = 10000,
        .cert_pem = wscert_start, //CA证书
    };
    //初始化stt、tts服务
    stt_client_init();
    tts_client_init();
    xTaskCreate(stt_stop_task, "stt_stop_task", 2600, NULL, 6, &stt_stop_handle);
    xTaskCreate(tts_stop_task, "tts_stop_task", 2600, NULL, 6, &tts_stop_handle);
}
/**  刷新stt服务
 * @param pvParameters 参数，未使用
 * @return 无
 */
static void stt_stop_task(void *pvParameters) 
{
    (void)pvParameters;
    while (1) {
        //等待任务通知
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        //停止并销毁websocket客户端
        if (stt_client) {
            esp_websocket_client_stop(stt_client);
            esp_websocket_client_destroy(stt_client);
            stt_client = NULL;
            stt_client_init();
        }
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
    vTaskDelete(NULL); //删除任务
}
/**  刷新tts服务
 * @param pvParameters 参数，未使用
 * @return 无
 */
static void tts_stop_task(void *pvParameters) 
{
    (void)pvParameters;
    while (1) {
        //等待任务通知
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        //停止并销毁websocket客户端
        if (tts_client) {
            esp_websocket_client_stop(tts_client);
            esp_websocket_client_destroy(tts_client);
            tts_client = NULL;
            tts_client_init();
        }
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
    vTaskDelete(NULL); //删除任务
}

/* 录制离线音频保存到芯片psram中
static void tts_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *tdata = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
        case WEBSOCKET_EVENT_BEGIN:
            break;
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI("tts", "tts client connected to server");
            char *buf = "我在呢"; //录制的音频文本
            send_tts_req(buf);
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI("tts", "tts client disconnected to server");
            break;
        case WEBSOCKET_EVENT_DATA:
            FILE *file = NULL;
            if (tdata->op_code == 0x2) { 
                    //打开文件，追加模式
                    file = fopen("/storage/wake.wav", "ab"); //保存录制音频的文件路径
                    if (file == NULL) {
                        ESP_LOGE("tts", "Failed to open file for writing");
                        break;
                    }
                    //写入数据
                    size_t written = fwrite(tdata->data_ptr, 1, tdata->data_len, file);
                    if (written != tdata->data_len) {
                        ESP_LOGE("tts", "Failed to write all data to file");
                    } else {
                        ESP_LOGI("tts", "Data written to file successfully");
                    }
                    fclose(file);
                    file = NULL; //避免重复关闭
            } else {
                //如果不是二进制数据，关闭文件
                if (file != NULL) {
                    fclose(file);
                    file = NULL;
                }
                ESP_LOGI("tts", "File closed");
            }
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGI("tts", "tts client error");
            break;
        case WEBSOCKET_EVENT_FINISH:
            ESP_LOGI("tts", "tts client work finish");
            break;
        default: break;
    }
}*/
