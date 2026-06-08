#ifndef BUTTON_GPIO_H
#define BUTTON_GPIO_H

#include <stdbool.h>
#include "esp_err.h"

// 按键中断回调类型：按键触发后会在中断上下文中调用。
typedef void (*button_gpio_callback_t)(void *arg);

/**
 * @brief 初始化按键 GPIO 与中断，并注册回调函数。
 *
 * @param callback 按键中断触发后的用户回调，可传 NULL。
 * @param callback_arg 回调私有参数，会原样传递给 callback。
 *
 * @return
 *      - ESP_OK: 初始化成功。
 *      - 其他值: 底层 GPIO 配置或中断注册失败。
 */
esp_err_t button_gpio_init(button_gpio_callback_t callback, void *callback_arg);

/**
 * @brief 读取按键引脚当前电平。
 *
 * @return
 *      - 1: 高电平。
 *      - 0: 低电平。
 */
int button_gpio_get_level(void);

/**
 * @brief 判断按键是否按下（当前电路为低电平按下）。
 *
 * @return true 表示按下，false 表示未按下。
 */
bool button_gpio_is_pressed(void);

#endif