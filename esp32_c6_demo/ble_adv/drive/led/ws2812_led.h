#ifndef WS2812_LED_H
#define WS2812_LED_H

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief 初始化 WS2812 灯带驱动（当前工程默认 1 颗灯珠）。
 */
esp_err_t ws2812_led_init(void);

/**
 * @brief 设置第 1 颗灯珠的 RGB 颜色并刷新输出。
 */
esp_err_t ws2812_led_set_rgb(uint8_t red, uint8_t green, uint8_t blue);

#endif
