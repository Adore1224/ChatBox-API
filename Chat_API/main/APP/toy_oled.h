#ifndef __TOY_OLED_H
#define __TOY_OLED_H

/*****************************************INCLUDE************************************************/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
/*****************************************全局定义***********************************************/
//取模数据
static const uint8_t bmp_wifi[] = {
    0x00,0x20,0x10,0x88,0x44,0x24,0x24,0x24,0x24,0x24,0x24,0x44,0x88,0x10,0x20,0x00,
    0x00,0x00,0x00,0x00,0x00,0x02,0x01,0x09,0x09,0x01,0x02,0x00,0x00,0x00,0x00,0x00,
}; /* 16x16 */
static const uint8_t bmp_ble[] = {
    0x00,0x00,0x00,0x00,0x00,0x10,0x20,0x40,0xFE,0x44,0x28,0x10,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x04,0x02,0x01,0x3F,0x11,0x0A,0x04,0x00,0x00,0x00,0x00,
}; /* 16x16 */
static const uint8_t font_xiao[] = {
    0x00,0x00,0x00,0xE0,0x00,0x00,0x00,0xFE,0x00,0x00,0x00,0x20,0x40,0x80,0x00,0x00,
    0x08,0x04,0x03,0x00,0x00,0x40,0x80,0x7F,0x00,0x00,0x00,0x00,0x00,0x01,0x0E,0x00,/*"小"*/
}; /* 16x16 */
static const uint8_t font_xin[] = {
    0x40,0x44,0x54,0x65,0xC6,0x64,0x54,0x44,0x00,0xFC,0x44,0x44,0xC4,0x42,0x40,0x00,
    0x20,0x12,0x4A,0x82,0x7F,0x02,0x0A,0x92,0x60,0x1F,0x00,0x00,0xFF,0x00,0x00,0x00,/*"新"*/
}; /* 16x16 */
static const uint8_t font_ling[] = {
    0x02,0x02,0xFE,0x92,0x92,0xFE,0x42,0x22,0x10,0x0C,0x23,0xCC,0x10,0x20,0x40,0x00,
    0x08,0x18,0x0F,0x08,0x04,0xFF,0x04,0x01,0x09,0x11,0x21,0xD1,0x0D,0x03,0x00,0x00,/*"聆"*/
}; /* 16x16 */
static const uint8_t font_ting[] = {
    0x00,0xFC,0x04,0x04,0xFC,0x00,0x00,0xFC,0x44,0x44,0x44,0xC2,0x43,0x42,0x40,0x00,
    0x00,0x0F,0x04,0x04,0x0F,0x80,0x60,0x1F,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,/*"听"*/
}; /* 16x16 */
static const uint8_t font_hui[] = {
    0x00,0x00,0xFE,0x02,0x02,0xF2,0x12,0x12,0x12,0xF2,0x02,0x02,0xFE,0x00,0x00,0x00,
    0x00,0x00,0x7F,0x20,0x20,0x27,0x24,0x24,0x24,0x27,0x20,0x20,0x7F,0x00,0x00,0x00,/*"回"*/
}; /* 16x16 */
static const uint8_t font_fu[] = {
    0x20,0x10,0x08,0xF7,0x54,0x54,0x54,0x54,0x54,0x54,0x54,0xF4,0x04,0x04,0x00,0x00,
    0x80,0x90,0x90,0x49,0x4D,0x57,0x25,0x25,0x25,0x55,0x4D,0x45,0x80,0x80,0x80,0x00,/*"复"*/
}; /* 16x16 */
static const uint8_t font_si[] = {
    0x00,0x00,0xFE,0x92,0x92,0x92,0x92,0xFE,0x92,0x92,0x92,0x92,0xFE,0x00,0x00,0x00,
    0x40,0x38,0x01,0x00,0x3C,0x40,0x40,0x42,0x4C,0x40,0x40,0x70,0x05,0x08,0x30,0x00,/*"思"*/
}; /* 16x16 */
static const uint8_t font_kao[] = {
    0x20,0x20,0x24,0x24,0x24,0x24,0xBF,0x64,0x24,0x34,0x28,0x24,0x22,0x20,0x20,0x00,
    0x10,0x08,0x04,0x02,0x01,0x0D,0x0B,0x09,0x49,0x89,0x49,0x39,0x01,0x00,0x00,0x00,/*"考"*/
}; /* 16x16 */
static const uint8_t font_zhong[] = {
    0x00,0x00,0xF0,0x10,0x10,0x10,0x10,0xFF,0x10,0x10,0x10,0x10,0xF0,0x00,0x00,0x00,
    0x00,0x00,0x0F,0x04,0x04,0x04,0x04,0xFF,0x04,0x04,0x04,0x04,0x0F,0x00,0x00,0x00,/*"中"*/
}; /* 16x16 */
static const uint8_t font_ni[] = {
    0x00,0x80,0x60,0xF8,0x07,0x40,0x20,0x18,0x0F,0x08,0xC8,0x08,0x08,0x28,0x18,0x00,
    0x01,0x00,0x00,0xFF,0x00,0x10,0x0C,0x03,0x40,0x80,0x7F,0x00,0x01,0x06,0x18,0x00,/*"你"*/
}; /* 16x16 */
static const uint8_t font_hao[] = {
    0x10,0x10,0xF0,0x1F,0x10,0xF0,0x00,0x80,0x82,0x82,0xE2,0x92,0x8A,0x86,0x80,0x00,
    0x40,0x22,0x15,0x08,0x16,0x61,0x00,0x00,0x40,0x80,0x7F,0x00,0x00,0x00,0x00,0x00,/*"好"*/
}; /* 16x16 */
static const uint8_t font_ya[] = {
    0x00,0xFC,0x04,0x04,0xFC,0x00,0xC2,0xBA,0x82,0x82,0x82,0xFE,0x82,0x82,0x80,0x00,
    0x00,0x0F,0x04,0x04,0x0F,0x20,0x10,0x08,0x04,0x42,0x81,0x7F,0x00,0x00,0x00,0x00,/*"呀"*/
}; /* 16x16 */
static const uint8_t font_h[] = {
    0x10,0xF0,0x00,0x80,0x80,0x80,0x00,0x00,0x20,0x3F,0x21,0x00,0x00,0x20,0x3F,0x20,/*"h"*/
}; /* 8x16 */
static const uint8_t font_i[] = {
    0x00,0x80,0x98,0x98,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,/*"i"*/
}; /* 8x16 */
static const uint8_t font_dou[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x70,0x00,0x00,0x00,0x00,0x00,/*","*/
}; /* 8x16 */
static const uint8_t font_gantan[] = {
    0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x33,0x00,0x00,0x00,0x00,/*"!"*/
}; /* 8x16 */
static const uint8_t font_dian[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00,0x00,0x00,/*"."*/
}; /* 8x16 */
static const uint8_t font_percent[] = {
    0xF0,0x08,0xF0,0x80,0x60,0x18,0x00,0x00,0x00,0x31,0x0C,0x03,0x1E,0x21,0x1E,0x00,/*"%"*/
}; /* 8x16 */
//0~9数字字模数组
static const uint8_t font_digit[10][16] = {
    {0x00,0xE0,0x10,0x08,0x08,0x10,0xE0,0x00,0x00,0x0F,0x10,0x20,0x20,0x10,0x0F,0x00},/*"0"*/
    {0x00,0x00,0x10,0x10,0xF8,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00},/*"1"*/
    {0x00,0x70,0x08,0x08,0x08,0x08,0xF0,0x00,0x00,0x30,0x28,0x24,0x22,0x21,0x30,0x00},/*"2"*/
    {0x00,0x30,0x08,0x08,0x08,0x88,0x70,0x00,0x00,0x18,0x20,0x21,0x21,0x22,0x1C,0x00},/*"3"*/
    {0x00,0x00,0x80,0x40,0x30,0xF8,0x00,0x00,0x00,0x06,0x05,0x24,0x24,0x3F,0x24,0x24},/*"4"*/
    {0x00,0xF8,0x88,0x88,0x88,0x08,0x08,0x00,0x00,0x19,0x20,0x20,0x20,0x11,0x0E,0x00},/*"5"*/
    {0x00,0xE0,0x10,0x88,0x88,0x90,0x00,0x00,0x00,0x0F,0x11,0x20,0x20,0x20,0x1F,0x00},/*"6"*/
    {0x00,0x18,0x08,0x08,0x88,0x68,0x18,0x00,0x00,0x00,0x00,0x3E,0x01,0x00,0x00,0x00},/*"7"*/
    {0x00,0x70,0x88,0x08,0x08,0x88,0x70,0x00,0x00,0x1C,0x22,0x21,0x21,0x22,0x1C,0x00},/*"8"*/
    {0x00,0xF0,0x08,0x08,0x08,0x10,0xE0,0x00,0x00,0x01,0x12,0x22,0x22,0x11,0x0F,0x00},/*"9"*/
}; /* 8x16 */
/*****************************************全局函数**********************************************/
void oled_init(void);
void oled_show_wifi(void);
void oled_clear_wifi(void);
void oled_show_ble(void);
void oled_clear_ble(void);
void oled_show_battary(uint8_t x, uint8_t y, uint8_t battery_percentage);
void oled_show_name(void);
void oled_show_hear(void);
void oled_show_think(void);
void oled_show_speak(void);
void oled_show_welcome(void);

#endif
