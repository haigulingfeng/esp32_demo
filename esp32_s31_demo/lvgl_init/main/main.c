/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "app_lvgl_runtime.h"

static const char *TAG = "BMGR_DISPLAY_LVGL";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting app runtime");
    esp_err_t ret = app_lvgl_runtime_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Runtime start failed: %s", esp_err_to_name(ret));
        return;
    }

    /* Keep app_main alive while LVGL task drives the UI */
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
