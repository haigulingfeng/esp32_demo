#include <stdint.h>
#include <stdbool.h>
#include "esp_log.h"
#include "led_strip.h"
#include "ws2812_led.h"

#define WS2812_GPIO 8
#define WS2812_LED_COUNT 1

static const char *TAG = "ws2812_led";
static led_strip_handle_t s_led_strip;
// 防止重复初始化驱动，避免重复申请硬件资源。
static bool s_initialized;

esp_err_t ws2812_led_init(void)
{
    // 灯带基础参数：GPIO、灯珠数量、芯片型号和颜色顺序。
    led_strip_config_t strip_config = {
        .strip_gpio_num = WS2812_GPIO,
        .max_leds = WS2812_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        },
    };

    // RMT 是 ESP32 常用的精确时序外设，WS2812 依赖精确时序输出。
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = false,
        },
    };

    if (s_initialized) {
        return ESP_OK;
    }

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_led_strip);
    if (ret != ESP_OK) {
        return ret;
    }

    // 初始化后先清屏，避免上电随机颜色残留。
    ret = led_strip_clear(s_led_strip);
    if (ret != ESP_OK) {
        return ret;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "WS2812 initialized on GPIO%d", WS2812_GPIO);

    return ESP_OK;
}

esp_err_t ws2812_led_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    // 未初始化时拒绝设置颜色，提醒调用顺序应先 init 再 set。
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = led_strip_set_pixel(s_led_strip, 0, red, green, blue);
    if (ret != ESP_OK) {
        return ret;
    }

    return led_strip_refresh(s_led_strip);
}
