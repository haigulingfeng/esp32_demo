#ifndef BLE_ADV_H
#define BLE_ADV_H

#include "esp_err.h"

/**
 * @brief 创建并启动 BLE 广播任务。
 *
 * 行为说明：
 * 1) 若已在 sdkconfig 中开启蓝牙，函数会创建 FreeRTOS 任务并开始广播。
 * 2) 若蓝牙未开启，函数返回 ESP_ERR_NOT_SUPPORTED。
 */
esp_err_t ble_adv_start_task(void);

#endif
