#ifndef WS2812_LED_H
#define WS2812_LED_H

#include <stdint.h>
#include "esp_err.h"

esp_err_t ws2812_led_init(void);
esp_err_t ws2812_led_set_rgb(uint8_t red, uint8_t green, uint8_t blue);

#endif
