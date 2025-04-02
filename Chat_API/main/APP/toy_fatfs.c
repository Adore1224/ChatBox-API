/*****************************************INCLUDE************************************************/
#include "toy_fatfs.h"
/******************************************宏定义************************************************/
#define MOUNT_POINT      "/storage"   //文件系统根目录
/*****************************************全局参数***********************************************/

/*****************************************局部参数***********************************************/
static const char *TAG = "fatfs";
static wl_handle_t wl_handle = WL_INVALID_HANDLE; //文件系统的操作句柄
/***************************************局部函数声明*********************************************/

/*****************************************函数实现**********************************************/
/** FAT文件系统初始化
 * @param 无
 * @return 无
*/ 
void fatfs_init(void)
{
    //挂载配置
    esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 5, //同时打开的最大文件数
        .format_if_mount_failed = true, //如果挂载失败则格式化
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE //扇区大小
    };
    //挂载文件系统
    esp_err_t ret = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_POINT, "storage", &mount_config, &wl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS");
        return;
    }
    /* 删除保存在芯片psram中的离线文件
    if (remove("/storage/wake.wav") == 0) {
        ESP_LOGI(TAG, "wav deleted successfully");
    } else {
        ESP_LOGE(TAG, "Failed to delete file");
    } 
    */
}
