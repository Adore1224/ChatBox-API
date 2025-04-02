/*****************************************INCLUDE************************************************/
#include "toy_wifi.h"
/******************************************宏定义************************************************/
#define NVS_WIFI_NAMESPACE_NAME         "PROD_WIFI"   //命名空间：所用NVS分区的空间名称
#define NVS_SSID_KEY                    "ssid"        //NVS存储的键值对的键名，键值为保存的wifi名称
#define NVS_PASSWORD_KEY                "password"    //NVS存储的键值对的键名，键值为保存的wifi密码
/*****************************************全局参数***********************************************/
wifi_config_t wifi_config;
bool is_wifi_connect = false;
/*****************************************局部参数***********************************************/
static char ssid_value[33] = {0};
static char password_value[65] = {0};
/***************************************局部函数声明*********************************************/
static void wifi_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data);
/*****************************************函数实现**********************************************/
/** 从NVS中读取SSID
 * @param ssid 读到的ssid
 * @param maxlen 外部存储ssid数组的最大值
 * @return 读取到的字节数
*/
size_t read_nvs_ssid(char* ssid,int maxlen)
{
    nvs_handle_t nvs_handle; //创建操作目标NVS区块的句柄
    esp_err_t ret = ESP_FAIL; //用来接收系统操作产生的错误类型，默认为ESP_FAIL
    size_t required_size = 0; //定义一个无符号且大小跟随系统地址空间大小的变量
    //操作NVS前先利用nvs_open获取句柄后才能操作nvs_open（nvs命名空间，读写权限，句柄）
    ESP_ERROR_CHECK(nvs_open(NVS_WIFI_NAMESPACE_NAME, NVS_READWRITE, &nvs_handle)); 
    ret = nvs_get_str(nvs_handle, NVS_SSID_KEY, NULL, &required_size); //获取键名NVS_SSID_KEY对应键值value的数据大小
    if(ret == ESP_OK && required_size <= maxlen) { //如果上一步函数返回ESP_OK且读到的键值数据大小不超过最大值
        nvs_get_str(nvs_handle,NVS_SSID_KEY,ssid,&required_size); //就从NVS中读取保存的wifi名称值放在ssid变量中
    } else
        required_size = 0; //失败则返回0,表示NVS中还没有存储wifi名
    nvs_close(nvs_handle); //关闭并释放该句柄资源
    return required_size; 
}
/** 写入SSID到NVS中
 * @param ssid 需写入的ssid
 * @return ESP_OK or ESP_FAIL
*/
esp_err_t write_nvs_ssid(char* ssid)
{
    nvs_handle_t nvs_handle; //创建操作目标NVS区块的句柄
    esp_err_t ret; //用来接收系统操作产生的错误类型
    ESP_ERROR_CHECK(nvs_open(NVS_WIFI_NAMESPACE_NAME, NVS_READWRITE, &nvs_handle));
    //利用nvs_set_str()向NVS中目标键的键值中写入指定的数据ssid
    ret = nvs_set_str(nvs_handle, NVS_SSID_KEY, ssid);
    nvs_commit(nvs_handle); //将上述对NVS保存内容的更改永久保存到NVS中，对NVS写入后必须执行该函数
    nvs_close(nvs_handle); //关闭并释放该句柄资源
    return ret; 
}
/** 从NVS中读取PASSWORD
 * @param ssid 读到的password
 * @param maxlen 外部存储password数组的最大值
 * @return 读取到的字节数
*/
size_t read_nvs_password(char* pwd,int maxlen)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret_val = ESP_FAIL;
    size_t required_size = 0;
    ESP_ERROR_CHECK(nvs_open(NVS_WIFI_NAMESPACE_NAME, NVS_READWRITE, &nvs_handle));
    ret_val = nvs_get_str(nvs_handle, NVS_PASSWORD_KEY, NULL, &required_size);
    if(ret_val == ESP_OK && required_size <= maxlen) {
        nvs_get_str(nvs_handle,NVS_SSID_KEY,pwd,&required_size);
    } else 
        required_size = 0;
    nvs_close(nvs_handle);
    return required_size;
}
/** 写入PASSWORD到NVS中
 * @param pwd 需写入的password
 * @return ESP_OK or ESP_FAIL
*/
esp_err_t write_nvs_password(char* pwd)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    ESP_ERROR_CHECK(nvs_open(NVS_WIFI_NAMESPACE_NAME, NVS_READWRITE, &nvs_handle));
    ret = nvs_set_str(nvs_handle, NVS_PASSWORD_KEY, pwd);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    return ret;
}
/** wifi相关的事件处理函数
 * @param arg 自定义参数
 * @param event_base 事件类型
 * @param event_id 事件标识ID，不同的事件类型都有不同的实际标识ID
 * @param event_data 事件携带的数据
 * @return 无
*/
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if(ssid_value[0] != 0) //如果保存wifi名称的变量不为空，即存在可连接wifi网络，尝试连接         
            esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) { 
        is_wifi_connect = false;
        esp_wifi_connect();
        oled_clear_wifi();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) { 
        is_wifi_connect = true;
        oled_show_wifi();
    }
}
/** wifi相关的初始化
 * @param 无
 * @return 无
*/
void wifi_init(void)
{
    esp_netif_init(); //初始化网卡
    esp_netif_create_default_wifi_sta(); //创建一个默认的STA网络接口,用于连接到AP
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg); //使用默认配置初始化WiFi驱动。
    //为WIFI_EVENT类型事件注册所有事件号,为IP_EVENT类型事件注册IP_EVENT_STA_GOT_IP这一个事件号
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
    //设置wifi工作模式为STA站点模式
    esp_wifi_set_mode(WIFI_MODE_STA);
    //从NVS中读出SSID
    read_nvs_ssid(ssid_value,32);
    //从NVS中读取PASSWORD
    read_nvs_password(password_value,64);   
    wifi_config = (wifi_config_t) {
            .sta = { 
                .threshold.authmode = WIFI_AUTH_WPA2_PSK, //配置认证模式
                .pmf_cfg = { //默认配置
                    .capable = true,
                    .required = false
                },
            },
        };
    if(ssid_value[0] != 0) {  //通过SSID第一个字节是否是0，判断是否读取成功，然后设置wifi_config_t
        snprintf((char*)wifi_config.sta.ssid,32,"%s",ssid_value); //格式化并保存到wifi配置参数中
        snprintf((char*)wifi_config.sta.password,64,"%s",password_value);  
    }
    //启动wifi连接
    esp_wifi_start();
}
