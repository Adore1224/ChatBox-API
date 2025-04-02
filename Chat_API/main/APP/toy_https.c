/*****************************************INCLUDE************************************************/
#include "toy_https.h"
/******************************************宏定义************************************************/
#define CHAT_TASK_PRIO          5          /* 任务优先级 */
#define CHAT_STK_SIZE           4096       /* 任务堆栈大小 */
/*****************************************全局参数***********************************************/
extern QueueHandle_t stt_queue;
extern const char chatcert_start[] asm("_binary_chat_ca_cert_pem_start"); //CA证书信息起始地址
/*****************************************局部参数***********************************************/
static const char *TAG = "https";
static esp_http_client_handle_t chat_client;
const char *prompt = "你是我的好朋友，你性格开朗，说话风趣幽默，回答简洁，字数控制在25字内"; //预设信息
const char model[] = ""; //对话推理点名称
static char chat_buffer[4096]; //存储chat对话文本
static size_t chat_buf_len = 0;
/***************************************局部函数声明*********************************************/
static char *get_post_payload(char *text);
static esp_err_t chat_https_handler(esp_http_client_event_t *evt);
static void process_chat_re(char *response_data);
static void chat_task(void *pvParameters);
/*****************************************函数实现**********************************************/
/** 构建数据包请求
 * @param *text-对话的输入文本
 * @return json_str json字符串
 */
static char *get_post_payload(char *text) 
{
    cJSON *root = cJSON_CreateObject(); 
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON data");
    }
    size_t len = strlen(prompt) + strlen(text) + 1;
    char *result = (char *)malloc(len);
    snprintf(result, len, "%s%s", prompt, text);
    cJSON_AddStringToObject(root, "model", model);
    cJSON *messages = cJSON_CreateArray();
    cJSON *user_message = cJSON_CreateObject();
    cJSON_AddStringToObject(user_message, "role", "user");
    cJSON_AddStringToObject(user_message, "content", result);
    cJSON_AddItemToArray(messages, user_message);
    cJSON_AddItemToObject(root, "messages", messages);
    cJSON_AddBoolToObject(root, "stream", false);
    cJSON_AddNumberToObject(root, "max_tokens", 2000);
    //序列化JSON数据为字符串
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        ESP_LOGE(TAG, "Failed to serialize JSON object");
        cJSON_Delete(root);
        return NULL;
    }
    //释放内存
    free(result);
    //删除JSON对象，释放内存
    cJSON_Delete(root);
    return json_str;
}
/** https处理chat server返回的响应内容
 * @param *response_data-对话模型返回的数据
 * @return 无
*/
static void process_chat_re(char *response_data)
{
    //解析JSON数据
    cJSON *root = cJSON_Parse(response_data);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON response data");
        return;
    }
    //处理choices数组
    cJSON *choices_json = cJSON_GetObjectItem(root, "choices");
    if (choices_json!= NULL && cJSON_IsArray(choices_json)) {
        int choices_size = cJSON_GetArraySize(choices_json);
        for (int i = 0; i < choices_size; i++) {
            cJSON *choice = cJSON_GetArrayItem(choices_json, i);
            if (choice!= NULL) {
                //提取message字段中的content
                cJSON *message_json = cJSON_GetObjectItem(choice, "message");
                if (message_json!= NULL && cJSON_IsObject(message_json)) {
                    cJSON *content_json = cJSON_GetObjectItem(message_json, "content");
                    if (content_json!= NULL && cJSON_IsString(content_json)) {
                        ESP_LOGI(TAG, "choices[%d].message.content: %s", i, content_json->valuestring);
                        char *re_text = content_json->valuestring;
                        send_tts_req(re_text);
                    }
                }
            }
        }
    }
    cJSON_Delete(root);
}
/** chat http事件处理函数，主要处理http响应和http状态
 * @param 事件结构体
 * @return 无
*/
static esp_err_t chat_https_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ON_CONNECTED:
            break;
        case HTTP_EVENT_ON_DATA:
            //检查缓冲区是否有足够空间来存储新的数据
            if (chat_buf_len + evt->data_len < sizeof(chat_buffer)) {
                //将新收到的数据复制到缓冲区中
                memcpy(chat_buffer + chat_buf_len, evt->data, evt->data_len);
                chat_buf_len += evt->data_len;
            } else {
                memset(chat_buffer, 0, sizeof(chat_buffer));
                chat_buf_len = 0;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            //请求完成后检查状态码以及处理数据
            if(esp_http_client_get_status_code(evt->client) == 200) {
                process_chat_re(chat_buffer); //处理响应   
                memset(chat_buffer, 0, sizeof(chat_buffer)); //清零缓冲区
                chat_buf_len = 0;
            }
            break;
        case HTTP_EVENT_DISCONNECTED:
            break;
        case HTTP_EVENT_ERROR:
            break;
        default: break;
    }
    return ESP_OK;
}
/** https初始化函数
 * @param 无
 * @return 无
*/
void https_init(void)
{
    esp_http_client_config_t chat_cfg = {
        .url =  "https://ark.cn-beijing.volces.com/api/v3/chat/completions",
        .event_handler = chat_https_handler,
        .cert_pem = chatcert_start, //CA证书
        .method = HTTP_METHOD_POST,
    };
    //创建HTTP客户端句柄
    chat_client = esp_http_client_init(&chat_cfg);
    //设置请求头
    esp_http_client_set_header(chat_client, "Content-Type", "application/json");
    esp_http_client_set_header(chat_client, "Authorization", "Bearer <填入你的access token后去掉<>>");
    xTaskCreate(chat_task, "chat_task", CHAT_STK_SIZE, NULL, CHAT_TASK_PRIO, NULL);
}

/** chat任务
 * @param pvParameters 参数，未使用
 * @return 无
*/
static void chat_task(void *pvParameters)
{
    (void)pvParameters;
    char pre_text[512] = {0};
    while (1) {
        if (xQueueReceive(stt_queue, pre_text, portMAX_DELAY) == pdPASS) {
            //执行chat Post请求
            char *text_json = get_post_payload(pre_text);
            esp_http_client_set_post_field(chat_client, text_json, strlen(text_json));
            esp_http_client_perform(chat_client);
            memset(pre_text, 0, 512); //清空文本
        }
    }
    vTaskDelete(NULL);
}
