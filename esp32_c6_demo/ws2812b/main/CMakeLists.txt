idf_component_register(SRCS "main.c" "ws2812_led.c" "button_gpio.c"
                    INCLUDE_DIRS "."
                    REQUIRES led_strip driver)
