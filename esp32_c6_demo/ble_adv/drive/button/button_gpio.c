#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "button_gpio.h"

#define BUTTON_GPIO_NUM GPIO_NUM_2

// 缓存用户注册的回调及参数，供中断服务程序调用。
static button_gpio_callback_t s_button_callback;
static void *s_button_callback_arg;
static bool s_isr_service_installed;
static bool s_button_initialized;

static void IRAM_ATTR button_gpio_isr_handler(void *arg)
{
    (void)arg;

    // 中断上下文中只做轻量动作：转发通知到上层回调。
    if (s_button_callback != NULL) {
        s_button_callback(s_button_callback_arg);
    }
}

esp_err_t button_gpio_init(button_gpio_callback_t callback, void *callback_arg)
{
    // GPIO2 配置为输入+上拉，并在下降沿触发中断（按下瞬间）。
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
        // 全局 ISR 服务只需安装一次。
        ret = gpio_install_isr_service(0);
        if ((ret != ESP_OK) && (ret != ESP_ERR_INVALID_STATE)) {
            return ret;
        }
        s_isr_service_installed = true;
    }

    if (s_button_initialized) {
        // 重复初始化时先移除旧回调，避免重复注册。
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
    // 直接返回硬件电平：1=高，0=低。
    return gpio_get_level(BUTTON_GPIO_NUM);
}

bool button_gpio_is_pressed(void)
{
    // 当前按键电路采用上拉输入，按下时拉低到 0。
    return button_gpio_get_level() == 0;
}