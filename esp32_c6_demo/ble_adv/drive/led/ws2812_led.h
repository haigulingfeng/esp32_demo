#ifndef WS2812_LED_H
#define WS2812_LED_H

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief 初始化 WS2812 灯带驱动（当前工程默认 1 颗灯珠）。
 *
 * @return
 *      - ESP_OK: 初始化成功或已初始化。
 *      - 其他值: RMT 或灯带驱动初始化失败。
 */
esp_err_t ws2812_led_init(void);

/**
 * @brief 设置第 1 颗灯珠的 RGB 颜色并刷新输出。
 *
 * @param red 红色分量亮度，范围 0-255。
 * @param green 绿色分量亮度，范围 0-255。
 * @param blue 蓝色分量亮度，范围 0-255。
 *
 * @return
 *      - ESP_OK: 设置并刷新成功。
 *      - ESP_ERR_INVALID_STATE: 驱动尚未初始化。
 *      - 其他值: 底层灯带接口调用失败。
 */
esp_err_t ws2812_led_set_rgb(uint8_t red, uint8_t green, uint8_t blue);

#endif
