#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "button_gpio.h"

#define BUTTON_GPIO_NUM GPIO_NUM_2

static button_gpio_callback_t s_button_callback;
static void *s_button_callback_arg;
static bool s_isr_service_installed;
static bool s_button_initialized;

static void IRAM_ATTR button_gpio_isr_handler(void *arg)
{
    (void)arg;

    if (s_button_callback != NULL) {
        s_button_callback(s_button_callback_arg);
    }
}

esp_err_t button_gpio_init(button_gpio_callback_t callback, void *callback_arg)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO_NUM,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        return ret;
    }

    if (!s_isr_service_installed) {
        ret = gpio_install_isr_service(0);
        if ((ret != ESP_OK) && (ret != ESP_ERR_INVALID_STATE)) {
            return ret;
        }
        s_isr_service_installed = true;
    }

    if (s_button_initialized) {
        ret = gpio_isr_handler_remove(BUTTON_GPIO_NUM);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    s_button_callback = callback;
    s_button_callback_arg = callback_arg;

    ret = gpio_isr_handler_add(BUTTON_GPIO_NUM, button_gpio_isr_handler, NULL);
    if (ret != ESP_OK) {
        return ret;
    }

    s_button_initialized = true;

    return ESP_OK;
}

int button_gpio_get_level(void)
{
    return gpio_get_level(BUTTON_GPIO_NUM);
}

bool button_gpio_is_pressed(void)
{
    return button_gpio_get_level() == 0;
}