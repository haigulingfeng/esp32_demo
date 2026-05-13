#ifndef BUTTON_GPIO_H
#define BUTTON_GPIO_H

#include <stdbool.h>
#include "esp_err.h"

// 按键中断回调类型：按键触发后会在中断上下文中调用。
typedef void (*button_gpio_callback_t)(void *arg);

/**
 * @brief 初始化按键 GPIO 与中断，并注册回调函数。
 */
esp_err_t button_gpio_init(button_gpio_callback_t callback, void *callback_arg);

/**
 * @brief 读取按键引脚当前电平。
 */
int button_gpio_get_level(void);

/**
 * @brief 判断按键是否按下（当前电路为低电平按下）。
 */
bool button_gpio_is_pressed(void);

#endif