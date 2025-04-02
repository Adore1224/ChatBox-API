/*****************************************INCLUDE************************************************/
#include "toy_oled.h"
/******************************************宏定义************************************************/
#define OLED_SCL_PIN           GPIO_NUM_6 //引脚定义
#define OLED_SDA_PIN           GPIO_NUM_7
#define OLED_SCL_Clr()         gpio_set_level(OLED_SCL_PIN, 0)  //定义GPIO操作
#define OLED_SCL_Set()         gpio_set_level(OLED_SCL_PIN, 1)
#define OLED_SDA_Clr()         gpio_set_level(OLED_SDA_PIN, 0)
#define OLED_SDA_Set()         gpio_set_level(OLED_SDA_PIN, 1)
#define OLED_CMD        0  //写命令
#define OLED_DATA       1  //写数据
/*****************************************全局参数***********************************************/

/*****************************************局部参数***********************************************/
uint8_t OLED_GRAM[144][8]; //定义屏幕显存
/***************************************局部函数声明*********************************************/
static void i2c_delay(void);
static void i2c_start(void);
static void i2c_stop(void);
static void i2c_waitAck(void);
static void i2c_send(uint8_t dat);
static void ssd1309_write_byte(uint8_t dat,uint8_t mode);
static void ssd1309_refresh(void);
static void oled_clear(void);
static void ssd1309_clear_area(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
static void ssd1309_draw_bitmap(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t width, uint8_t height);
static void ssd1309_display_digit(uint8_t x, uint8_t y, uint8_t digit);
static void ssd1309_display_number(uint8_t x, uint8_t y, uint8_t number);
static void ssd1309_init(void);
/*****************************************函数实现**********************************************/
/** i2c延时函数
 * @param 无
 * @return 无
 */	
static void i2c_delay(void)
{
    uint8_t t=3;
    while (t--);
}
/** i2c发送开始信号
 * @param 无
 * @return 无
 */	
static void i2c_start(void)
{
    OLED_SDA_Set();
    OLED_SCL_Set();
    i2c_delay();
    OLED_SDA_Clr();
    i2c_delay();
    OLED_SCL_Clr();
    i2c_delay();
}
/** i2c发送结束信号
 * @param 无
 * @return 无
 */	
static void i2c_stop(void)
{
    OLED_SDA_Clr();
    OLED_SCL_Set();
    i2c_delay();
    OLED_SDA_Set();
}
/** i2c等待ack响应
 * @param 无
 * @return 无
 */	
static void i2c_waitAck(void) 
{
    OLED_SDA_Set();
    i2c_delay();
    OLED_SCL_Set();
    i2c_delay();
    OLED_SCL_Clr();
    i2c_delay();
}
/** i2c发送一个字节
 * @param dat 数据
 * @return 无
 */	
static void i2c_send(uint8_t dat)
{
    uint8_t i;
    for (i=0; i<8; i++) {
        if (dat & 0x80) { //将dat的8位从最高位依次写入
            OLED_SDA_Set();
        } else {
            OLED_SDA_Clr();
        }
        i2c_delay();
        OLED_SCL_Set();
        i2c_delay();
        OLED_SCL_Clr(); //将时钟信号设置为低电平
        dat<<=1;
    }
}
/** ssd1309写入字节
 * @param  dat 数据; mode 0-表示命令;1-表示数据;
 * @return 无
 */
static void ssd1309_write_byte(uint8_t dat,uint8_t mode)
{
    i2c_start();
    i2c_send(0x78);
    i2c_waitAck();
    if (mode) {
        i2c_send(0x40);
    } else {
        i2c_send(0x00);
    }
    i2c_waitAck();
    i2c_send(dat);
    i2c_waitAck();
    i2c_stop();
}
/** oled更新显示
 * @param 无
 * @return 无
 */	
static void ssd1309_refresh(void)
{
    uint8_t i,n;
    for (i=0; i<8; i++) {
        ssd1309_write_byte(0xb0+i,OLED_CMD); //设置行起始地址
        ssd1309_write_byte(0x00,OLED_CMD); //设置低列起始地址
        ssd1309_write_byte(0x10,OLED_CMD); //设置高列起始地址
        i2c_start();
        i2c_send(0x78);
        i2c_waitAck();
        i2c_send(0x40);
        i2c_waitAck();
        for (n=0; n<128; n++) {
            i2c_send(OLED_GRAM[n][i]);
            i2c_waitAck();
        }
        i2c_stop();
    }
}
/** oled清屏
 * @param 无
 * @return 无
 */
static void oled_clear(void)
{
    uint8_t i,n;
    for (i=0; i<8; i++) {
        for(n=0; n<128; n++) {
            OLED_GRAM[n][i]=0;//清除所有数据
        }
    }
    ssd1309_refresh(); //更新显示
}
/** 清除第0页指定区域
 * @param x-起始x坐标;y-起始y坐标;width-区域宽度;height-区域高度
 * @return 无
 */
static void ssd1309_clear_area(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    uint8_t page, col;
    for (page = 0; page < height / 8; page++) {
        for (col = 0; col < width; col++) {
            if (x + col < 128 && y + page < 8) {
                OLED_GRAM[x + col][y + page] = 0;
            }
        }
    }
    ssd1309_refresh(); //更新显示
}
/** oled绘制位图显示
 * @param x-起始x坐标;y-起始y坐标;*bitmap-位图数据地址;width-区域宽度;height-区域高度
 * @return 无
 */
static void ssd1309_draw_bitmap(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t width, uint8_t height)
{
    uint8_t page_start = y / 8;
    uint8_t page_end = (y + height - 1) / 8;
    uint8_t y_offset = y % 8;
    for (uint8_t page = page_start; page <= page_end; page++) {
        uint8_t current_y_offset = (page == page_start) ? y_offset : 0;
        uint16_t byte_index = (page - page_start) * width;
        for (uint8_t col = 0; col < width; col++) {
            if (x + col >= 128) break; //防止越界
            uint8_t data = 0;
            if (byte_index < width * height / 8) {
                data = bitmap[byte_index] >> current_y_offset;
            }
            if (current_y_offset > 0 && byte_index + width < width * height / 8) {
                data |= bitmap[byte_index + width] << (8 - current_y_offset);
            }
            OLED_GRAM[x + col][page] = data; //更新数据
            byte_index++;
        }
    }
    ssd1309_refresh(); //更新显示
}
/** oled绘制单个数字
 * @param x-起始x坐标;y-起始y坐标;digit-待显示数字
 * @return 无
 */
static void ssd1309_display_digit(uint8_t x, uint8_t y, uint8_t digit)
{
    if (digit > 9) return; //确保输入的数字在0~9范围内
    uint8_t page_start = y / 8;
    uint8_t page_end = (y + 16 - 1) / 8; //数字字模高度为16
    uint8_t y_offset = y % 8;
    for (uint8_t page = page_start; page <= page_end; page++) {
        uint8_t current_y_offset = (page == page_start) ? y_offset : 0;
        uint16_t byte_index = (page - page_start) * 8;
        for (uint8_t col = 0; col < 8; col++) {
            if (x + col >= 128) break; //防止越界
            uint8_t data = 0;
            if (byte_index < 16) {
                data = font_digit[digit][byte_index] >> current_y_offset;
            }
            if (current_y_offset > 0 && byte_index + 8 < 16) {
                data |= font_digit[digit][byte_index + 8] << (8 - current_y_offset);
            }
            OLED_GRAM[x + col][page] = data; //更新数据
            byte_index++;
        }
    }
}
/** oled绘制多位数字
 * @param x-起始x坐标;y-起始y坐标;number-待显示数字
 * @return 无
 */
static void ssd1309_display_number(uint8_t x, uint8_t y, uint8_t number)
{
    char num_str[10];
    sprintf(num_str, "%hhu", number); //将数字转换为字符串
    uint8_t digit_width = 8; 
    uint8_t offset = 0;
    for (uint8_t i = 0; num_str[i] != '\0'; i++) {
        uint8_t digit = num_str[i] - '0'; //将字符转换为数字
        ssd1309_display_digit(x + offset, y, digit);
        offset += digit_width; //偏移到下一个数字的位置
    }
}
/** oled内部芯片ssd1309,gpio初始化
 * @param 无
 * @return 无
 */
static void ssd1309_init(void)
{
    //显示屏所用GPIO初始化
    gpio_config_t io_conf; 
    io_conf.intr_type = GPIO_INTR_DISABLE; //禁用gpio中断
    io_conf.mode = GPIO_MODE_OUTPUT; //输出模式
    io_conf.pin_bit_mask = (1ULL << OLED_SCL_PIN) | (1ULL << OLED_SDA_PIN); //设置位掩码
    io_conf.pull_down_en = 0; //禁用下拉电阻
    io_conf.pull_up_en = 0; //禁用上拉电阻
    gpio_config(&io_conf);
    //ssd1309初始化
    ssd1309_write_byte(0xFD,OLED_CMD);
    ssd1309_write_byte(0x12,OLED_CMD);
    ssd1309_write_byte(0xAE,OLED_CMD); //--turn off oled panel
    ssd1309_write_byte(0xd5,OLED_CMD); //--set display clock divide ratio/oscillator frequency
    ssd1309_write_byte(0xA0,OLED_CMD);
    ssd1309_write_byte(0xA8,OLED_CMD); //--set multiplex ratio(1 to 64)
    ssd1309_write_byte(0x3f,OLED_CMD); //--1/64 duty
    ssd1309_write_byte(0xD3,OLED_CMD); //-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
    ssd1309_write_byte(0x00,OLED_CMD); //-not offset
    ssd1309_write_byte(0x40,OLED_CMD); //--set start line address  Set Mapping RAM Display start Line (0x00~0x3F)
    ssd1309_write_byte(0xA1,OLED_CMD); //--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
    ssd1309_write_byte(0xC8,OLED_CMD); //Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
    ssd1309_write_byte(0xDA,OLED_CMD); //--set com pins hardware configuration
    ssd1309_write_byte(0x12,OLED_CMD);
    ssd1309_write_byte(0x81,OLED_CMD); //--set contrast control register
    ssd1309_write_byte(0x7F,OLED_CMD); // Set SEG Output Current Brightness
    ssd1309_write_byte(0xD9,OLED_CMD); //--set pre-charge period
    ssd1309_write_byte(0x82,OLED_CMD); //Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    ssd1309_write_byte(0xDB,OLED_CMD); //--set vcomh
    ssd1309_write_byte(0x34,OLED_CMD); //Set VCOM Deselect Level
    ssd1309_write_byte(0xA4,OLED_CMD); // Disable Entire Display On (0xa4/0xa5)
    ssd1309_write_byte(0xA6,OLED_CMD); // Disable Inverse Display On (0xa6/a7)
    oled_clear();
    ssd1309_write_byte(0xAF,OLED_CMD);
}
/** oled初始化
 * @param 无
 * @return 无
 */
void oled_init(void) 
{
    //初始化ssd1309芯片
    ssd1309_init(); 
    //设置显示模式
    ssd1309_write_byte(0xA6,OLED_CMD);
    ssd1309_write_byte(0xC8,OLED_CMD); 
    ssd1309_write_byte(0xA1,OLED_CMD);
    //开屏默认显示信息
    oled_show_name(); //玩具名称
    oled_show_welcome(); //欢迎词
}
/** oled显示wifi标志
 * @param 无
 * @return 无
 */
void oled_show_wifi(void) 
{
    ssd1309_draw_bitmap(0, 0, bmp_wifi, 16, 16);
}
/** oled清除wifi标志
 * @param 无
 * @return 无
 */
void oled_clear_wifi(void) 
{
    ssd1309_clear_area(0, 0, 16, 16);
}
/** oled显示ble标志
 * @param 无
 * @return 无
 */
void oled_show_ble(void) 
{
    ssd1309_draw_bitmap(20, 0, bmp_ble, 16, 16);
}
/** oled清除ble标志
 * @param 无
 * @return 无
 */
void oled_clear_ble(void) 
{
    ssd1309_clear_area(20, 0, 16, 16);
}
/** oled显示当前电池电量
 * @param x-起始x坐标;y-起始y坐标;battery_percentage-电量数值
 * @return 无
 */
void oled_show_battary(uint8_t x, uint8_t y, uint8_t battery_percentage)
{
    //显示电量数值
    ssd1309_display_number(x, y, battery_percentage);
    //计算数字的位数
    uint8_t digit_count = (battery_percentage >= 100)? 3 : (battery_percentage >= 10)? 2 : 1;
    //每个数字宽度为8，数字间隔为0
    uint8_t digit_width = 8;
    uint8_t digit_spacing = 0;
    //计算百分号的起始位置
    uint8_t percent_x = x + digit_count * (digit_width + digit_spacing);
    uint8_t page_start = y / 8;
    uint8_t page_end = (y + 16 - 1) / 8; 
    //写入百分号
    for (uint8_t page = page_start; page <= page_end; page++) {
        for (uint8_t col = 0; col < 8; col++) {
            if (percent_x + col < 128) {
                OLED_GRAM[percent_x + col][page] = font_percent[(page - page_start) * 8 + col];
            }
        }
    }
    ssd1309_refresh(); //更新显示
}
/** oled显示昵称
 * @param 无
 * @return 无
 */
void oled_show_name(void) 
{
    ssd1309_draw_bitmap(48, 0, font_xiao, 16, 16);
    ssd1309_draw_bitmap(64, 0, font_xin, 16, 16);
}
/** oled显示聆听状态
 * @param 无
 * @return 无
 */
void oled_show_hear(void) 
{
    ssd1309_draw_bitmap(32, 32, font_ling, 16, 16);
    ssd1309_draw_bitmap(48, 32, font_ting, 16, 16);
    ssd1309_draw_bitmap(64, 32, font_zhong, 16, 16);
    ssd1309_draw_bitmap(80, 32, font_dian, 8, 16);
    ssd1309_draw_bitmap(88, 32, font_dian, 8, 16);
    ssd1309_draw_bitmap(96, 32, font_dian, 8, 16);
}
/** oled显示思考状态
 * @param 无
 * @return 无
 */
void oled_show_think(void) 
{
    ssd1309_draw_bitmap(32, 32, font_si, 16, 16);
    ssd1309_draw_bitmap(48, 32, font_kao, 16, 16);
    ssd1309_draw_bitmap(64, 32, font_zhong, 16, 16);
    ssd1309_draw_bitmap(80, 32, font_dian, 8, 16);
    ssd1309_draw_bitmap(88, 32, font_dian, 8, 16);
    ssd1309_draw_bitmap(96, 32, font_dian, 8, 16);
}
/** oled显示回复状态
 * @param 无
 * @return 无
 */
void oled_show_speak(void) 
{
    ssd1309_draw_bitmap(32, 32, font_hui, 16, 16);
    ssd1309_draw_bitmap(48, 32, font_fu, 16, 16);
    ssd1309_draw_bitmap(64, 32, font_zhong, 16, 16);
    ssd1309_draw_bitmap(80, 32, font_dian, 8, 16);
    ssd1309_draw_bitmap(88, 32, font_dian, 8, 16);
    ssd1309_draw_bitmap(96, 32, font_dian, 8, 16);
}
/** oled显示欢迎状态
 * @param 无
 * @return 无
 */
void oled_show_welcome(void)
{
    ssd1309_draw_bitmap(32, 32, font_h, 8, 16); 
    ssd1309_draw_bitmap(40, 32, font_i, 8, 16);
    ssd1309_draw_bitmap(48, 32, font_dou, 8, 16);
    ssd1309_draw_bitmap(56, 32, font_ni, 16, 16);
    ssd1309_draw_bitmap(72, 32, font_hao, 16, 16);
    ssd1309_draw_bitmap(88, 32, font_ya, 16, 16);
    ssd1309_draw_bitmap(104, 32, font_gantan, 8, 16);
}
