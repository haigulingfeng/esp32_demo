#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_err.h"
#include "ws2812_led.h"
#include "button_gpio.h"
#include "ble_adv.h"

static const char* TAG = "main";

#define BRIGHTNESS_10_PCT 12

static TaskHandle_t s_button_task_handle;

static void IRAM_ATTR button_isr_callback(void *arg)
{
    (void)arg;

    if (s_button_task_handle != NULL) {
        BaseType_t x_higher_priority_task_woken = pdFALSE;
        vTaskNotifyGiveFromISR(s_button_task_handle, &x_higher_priority_task_woken);
        portYIELD_FROM_ISR(x_higher_priority_task_woken);
    }
}

static void ws2812_task(void *pvParameters)
{
    (void)pvParameters;

    ESP_LOGI(TAG, "Set WS2812B to blue at 10%% brightness");
    ESP_ERROR_CHECK(ws2812_led_init());
    ESP_ERROR_CHECK(ws2812_led_set_rgb(0, 0, BRIGHTNESS_10_PCT));

    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void button_event_task(void *pvParameters)
{
    (void)pvParameters;

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // 消抖处理：延时20ms后再次判断按键是否仍然按下
        vTaskDelay(pdMS_TO_TICKS(150));
        if (button_gpio_is_pressed()) {
            ESP_LOGI(TAG, "GPIO2 button有效按下(消抖后)");
            // 这里写你的业务逻辑
        }
    }
}

static void system_heartbeat_task(void *pvParameters)
{
    (void)pvParameters;

    while (1) {
        ESP_LOGI(TAG, "system alive: still running");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    xTaskCreate(button_event_task, "button_event_task", 3072, NULL, 6, &s_button_task_handle);
    ESP_ERROR_CHECK(button_gpio_init(button_isr_callback, NULL));
    xTaskCreate(ws2812_task, "ws2812_task", 4096, NULL, 5, NULL);
    xTaskCreate(system_heartbeat_task, "system_heartbeat_task", 3072, NULL, 4, NULL);

    // BLE 启动入口：内部会创建 BLE 任务并在初始化完成后开始广播设备名 Kiana。
    esp_err_t ble_ret = ble_adv_start_task();
    if ((ble_ret != ESP_OK) && (ble_ret != ESP_ERR_NOT_SUPPORTED)) {
        ESP_ERROR_CHECK(ble_ret);
    }
}