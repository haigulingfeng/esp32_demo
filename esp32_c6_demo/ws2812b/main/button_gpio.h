#ifndef BUTTON_GPIO_H
#define BUTTON_GPIO_H

#include <stdbool.h>
#include "esp_err.h"

typedef void (*button_gpio_callback_t)(void *arg);

esp_err_t button_gpio_init(button_gpio_callback_t callback, void *callback_arg);
int button_gpio_get_level(void);
bool button_gpio_is_pressed(void);

#endif