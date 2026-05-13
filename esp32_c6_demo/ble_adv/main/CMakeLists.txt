idf_component_register(SRCS "main.c" "../drive/led/ws2812_led.c" "../drive/button/button_gpio.c" "../drive/ble/ble_adv.c"
                    INCLUDE_DIRS "." "../drive/ble" "../drive/led" "../drive/button"
                    REQUIRES led_strip driver bt nvs_flash)
