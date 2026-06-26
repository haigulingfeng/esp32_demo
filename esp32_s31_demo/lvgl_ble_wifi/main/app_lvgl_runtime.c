/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_lv_adapter.h"
#include "lvgl.h"
#include "lvgl_component_menu.h"
#include "esp_board_manager_includes.h"
#include "app_lvgl_runtime.h"

static const char *TAG = "APP_LVGL_RUNTIME";

static esp_err_t lcd_lvgl_adapter_init(void)
{
    esp_lv_adapter_config_t adapter_config = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    adapter_config.task_priority = 4;
    adapter_config.task_stack_size = 6 * 1024;
    adapter_config.task_core_id = 1;
    adapter_config.tick_period_ms = 5;
    adapter_config.task_max_delay_ms = 500;

    return esp_lv_adapter_init(&adapter_config);
}

#if CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_DSI_SUPPORT || CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_RGB_SUPPORT || CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_RGB_3WIRE_SPI_SUPPORT
static esp_lv_adapter_rotation_t lcd_get_rotation(const dev_display_lcd_config_t *lcd_cfg)
{
    if (lcd_cfg->swap_xy) {
        return lcd_cfg->mirror_x ? ESP_LV_ADAPTER_ROTATE_90 : ESP_LV_ADAPTER_ROTATE_270;
    }

    return (lcd_cfg->mirror_x || lcd_cfg->mirror_y) ? ESP_LV_ADAPTER_ROTATE_180 : ESP_LV_ADAPTER_ROTATE_0;
}

static esp_lv_adapter_tear_avoid_mode_t lcd_get_tear_mode(uint8_t num_fbs)
{
    if (num_fbs >= 3) {
        return ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL;
    }
    if (num_fbs == 2) {
        return ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_PARTIAL;
    }
    return ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE;
}
#endif

static lv_display_t *lcd_lvgl_adapter_register_display(const dev_display_lcd_config_t *lcd_cfg,
                                                       const dev_display_lcd_handles_t *lcd_handles)
{
    esp_lv_adapter_rotation_t rotation = ESP_LV_ADAPTER_ROTATE_0;
    esp_lv_adapter_display_config_t disp_cfg;

    if (strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_DSI) == 0) {
#if CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_DSI_SUPPORT
        rotation = lcd_get_rotation(lcd_cfg);
        disp_cfg = ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(lcd_handles->panel_handle,
                                                               lcd_handles->io_handle,
                                                               lcd_cfg->lcd_width,
                                                               lcd_cfg->lcd_height,
                                                               rotation);
        disp_cfg.tear_avoid_mode = lcd_get_tear_mode(lcd_cfg->sub_cfg.dsi.dpi_config.num_fbs);
#else
        ESP_LOGE(TAG, "DSI support not enabled");
        return NULL;
#endif
    } else if (strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_RGB) == 0 ||
               strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_RGB_3WIRE_SPI) == 0) {
#if CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_RGB_SUPPORT || CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_RGB_3WIRE_SPI_SUPPORT
        rotation = lcd_get_rotation(lcd_cfg);
        disp_cfg = ESP_LV_ADAPTER_DISPLAY_RGB_DEFAULT_CONFIG(lcd_handles->panel_handle,
                                                              lcd_handles->io_handle,
                                                              lcd_cfg->lcd_width,
                                                              lcd_cfg->lcd_height,
                                                              rotation);
        if (strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_RGB_3WIRE_SPI) == 0) {
#if CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_RGB_3WIRE_SPI_SUPPORT
            disp_cfg.tear_avoid_mode = lcd_get_tear_mode(lcd_cfg->sub_cfg.rgb_3wire_spi.rgb_panel_config.num_fbs);
#endif
        } else {
#if CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_RGB_SUPPORT
            disp_cfg.tear_avoid_mode = lcd_get_tear_mode(lcd_cfg->sub_cfg.rgb.panel_config.num_fbs);
#endif
        }
#else
        ESP_LOGE(TAG, "RGB support not enabled");
        return NULL;
#endif
    } else if (strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_SPI) == 0 ||
               strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_I80) == 0 ||
               strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_PARLIO) == 0) {
#ifdef CONFIG_SPIRAM
        disp_cfg = ESP_LV_ADAPTER_DISPLAY_SPI_WITH_PSRAM_DEFAULT_CONFIG(lcd_handles->panel_handle,
                                                                         lcd_handles->io_handle,
                                                                         lcd_cfg->lcd_width,
                                                                         lcd_cfg->lcd_height,
                                                                         rotation);
#else
        disp_cfg = ESP_LV_ADAPTER_DISPLAY_SPI_WITHOUT_PSRAM_DEFAULT_CONFIG(lcd_handles->panel_handle,
                                                                            lcd_handles->io_handle,
                                                                            lcd_cfg->lcd_width,
                                                                            lcd_cfg->lcd_height,
                                                                            rotation);
#endif
    } else {
        ESP_LOGE(TAG, "Unknown LCD sub_type: %s", lcd_cfg->sub_type);
        return NULL;
    }

    ESP_LOGI(TAG, "Register LCD: sub_type=%s, size=%dx%d", lcd_cfg->sub_type, lcd_cfg->lcd_width, lcd_cfg->lcd_height);
    return esp_lv_adapter_register_display(&disp_cfg);
}

esp_err_t app_lvgl_runtime_start(void)
{
    esp_err_t ret;
    lv_display_t *disp = NULL;
    lv_indev_t *touch_indev = NULL;

    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "display_lcd init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_TOUCH);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "lcd_touch init failed: %s", esp_err_to_name(ret));
    }

    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "lcd_brightness init failed: %s", esp_err_to_name(ret));
    }

    ret = lcd_lvgl_adapter_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LVGL adapter init failed: %s", esp_err_to_name(ret));
        goto fail;
    }

    void *lcd_handle = NULL;
    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, &lcd_handle);
    if (ret != ESP_OK || !lcd_handle) {
        ESP_LOGE(TAG, "LCD handle not found");
        ret = ESP_FAIL;
        goto fail;
    }

    dev_display_lcd_config_t *lcd_cfg = NULL;
    ret = esp_board_manager_get_device_config(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, (void **)&lcd_cfg);
    if (ret != ESP_OK || !lcd_cfg) {
        ESP_LOGE(TAG, "LCD config not found");
        ret = ESP_FAIL;
        goto fail;
    }

    disp = lcd_lvgl_adapter_register_display(lcd_cfg, (dev_display_lcd_handles_t *)lcd_handle);
    if (!disp) {
        ret = ESP_FAIL;
        goto fail;
    }

#ifdef CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT
    void *touch_handle = NULL;
    if (esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_LCD_TOUCH, &touch_handle) == ESP_OK && touch_handle) {
        dev_lcd_touch_handles_t *touch_handles = (dev_lcd_touch_handles_t *)touch_handle;
        const esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, touch_handles->touch_handle);
        touch_indev = esp_lv_adapter_register_touch(&touch_cfg);
        if (!touch_indev) {
            ESP_LOGW(TAG, "Touch register failed");
        }
    }
#endif

    ret = esp_lv_adapter_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LVGL adapter start failed: %s", esp_err_to_name(ret));
        goto fail;
    }

    lvgl_component_menu_start();
    return ESP_OK;

fail:
    if (touch_indev) {
        esp_lv_adapter_unregister_touch(touch_indev);
    }
    if (disp) {
        esp_lv_adapter_unregister_display(disp);
    }
    esp_lv_adapter_deinit();
    esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS);
    esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD);
    esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_TOUCH);
    return ret;
}
