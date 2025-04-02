# Chat-API
*本项目在此处分为两部分：ESPIDF源码、硬件原理图。*
*项目的演示效果可前往B站观看视频：https://www.bilibili.com/video/BV166ZtYaEBo/?spm_id_from=333.1387.homepage.video_card.click&vd_source=0ccc5a8ba2d40c3ceb0c79547fe5b693* 
## 一、实现功能
具备以下功能：蓝牙配网、电量监测、OLED显示、自定义唤醒词对话、静音检测、离线语音控制、AI大模型对话功能。
## 二、资源介绍
### Chat_API文件夹
内含ESP-IDF端源码，可直接下载使用，需注意在代码toy_websocket.c/toy_http.c中修改添加自己的API调用鉴权信息和密钥。
### Hardware文件夹
内含ZigBee模块扩展底板的电路原理图，直接复刻可适配上位机和ZigBee代码，如需自定义硬件，可能需要修改代码来适配。