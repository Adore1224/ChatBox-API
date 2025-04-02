/*****************************************INCLUDE************************************************/
#include "toy_bat.h"
/******************************************宏定义************************************************/
#define BAT_TASK_PRIO          5           /* 任务优先级 */
#define BAT_STK_SIZE           1024         /* 任务堆栈大小 */
#define BAT_SCL_PIN            GPIO_NUM_10
#define BAT_SDA_PIN            GPIO_NUM_9
#define MAX17048_I2C_ADDRESS   0x36
#define I2C1_FREQ_HZ           100000

#define MAX17048_REG_VCELL         0x02  //电池电压寄存器
#define MAX17048_REG_SOC           0x04  //电量百分比寄存器
/*****************************************全局参数***********************************************/

/*****************************************局部参数***********************************************/

/***************************************局部函数声明*********************************************/
static uint16_t max17048_read_register(uint8_t reg);
static float max17048_read_voltage(void);
static float max17048_read_soc(void);
static void bat_detect_task(void *pvParameters);
/*****************************************函数实现**********************************************/
/** 读取芯片寄存器
 * @param reg 目标寄存器地址
 * @return 读取到的寄存器值
 */
static uint16_t max17048_read_register(uint8_t reg) 
{
    uint8_t data[2] = {0};

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX17048_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX17048_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return (data[0] << 8) | data[1];
}
/** 读取电池当前的电压值
 * @param 无
 * @return 电压值
 */
static float max17048_read_voltage(void) 
{
    uint16_t vcell = max17048_read_register(MAX17048_REG_VCELL);
    return (float)vcell * 78.125 / 1000000; //转换为电压值v
}
/** 读取电池当前的电量百分比
 * @param 无
 * @return 电量百分比
 */
static float max17048_read_soc(void) 
{
    uint16_t soc = max17048_read_register(MAX17048_REG_SOC);
    return (float)soc / 256; //转换为百分比
}
/** 初始化电池监测芯片max17048,硬件i2c通信
 * @param 无
 * @return 无
 */
void bat_ic_init(void) 
{
    //i2c通信配置
    i2c_config_t i2c1_cfg = {
        .mode = I2C_MODE_MASTER,             //主机模式
        .sda_io_num = BAT_SDA_PIN,           //SDA引脚
        .scl_io_num = BAT_SCL_PIN,           //SCL引脚
        .sda_pullup_en = GPIO_PULLUP_ENABLE, //SDA默认上拉
        .scl_pullup_en = GPIO_PULLUP_ENABLE, //SCL默认上拉
        .master.clk_speed = I2C1_FREQ_HZ,    //通信时钟频率
    };
    //配置并安装i2c1驱动
    i2c_param_config(I2C_NUM_1, &i2c1_cfg);
    i2c_driver_install(I2C_NUM_1, i2c1_cfg.mode, 0, 0, 0);
    //为RTOS创建bat_detect任务
    xTaskCreate(bat_detect_task, "bat_detect_task", BAT_STK_SIZE, NULL, BAT_TASK_PRIO, NULL);
}
/**
 * @brief bat_detect任务，实时监测电量
 * @param pvParameters 参数，未使用
 * @return 无
 */
static void bat_detect_task(void *pvParameters)
{
    (void)pvParameters;
    while(1) {
        float soc = max17048_read_soc(); //读取剩余电量数值
        uint8_t bat = (uint8_t)soc; //保留整数部分
        oled_show_battary(96, 0, bat);
        //float voltage = max17048_read_voltage(); //读取电池电压值
        //ESP_LOGI("bat", "Voltage: %.2f V, SOC: %.2f%%", voltage, soc);
        vTaskDelay(pdMS_TO_TICKS(10000)); //每10秒读取一次
    }
    vTaskDelete(NULL); //删除本任务
}
