#include <stdbool.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

const struct gpio_dt_spec debug_led = GPIO_DT_SPEC_GET(DT_ALIAS(debug_led), gpios);
const struct gpio_dt_spec a0 = GPIO_DT_SPEC_GET(DT_ALIAS(a0), gpios);
const struct gpio_dt_spec a1 = GPIO_DT_SPEC_GET(DT_ALIAS(a1), gpios);
const struct gpio_dt_spec a2 = GPIO_DT_SPEC_GET(DT_ALIAS(a2), gpios);
const struct gpio_dt_spec a3 = GPIO_DT_SPEC_GET(DT_ALIAS(a3), gpios);
const struct gpio_dt_spec a4 = GPIO_DT_SPEC_GET(DT_ALIAS(a4), gpios);
const struct gpio_dt_spec a5 = GPIO_DT_SPEC_GET(DT_ALIAS(a5), gpios);
const struct gpio_dt_spec a6 = GPIO_DT_SPEC_GET(DT_ALIAS(a6), gpios);
const struct gpio_dt_spec a7 = GPIO_DT_SPEC_GET(DT_ALIAS(a7), gpios);
const struct gpio_dt_spec b0 = GPIO_DT_SPEC_GET(DT_ALIAS(b0), gpios);
const struct gpio_dt_spec *as[] = {&a0, &a1, &a2, &a3, &a4, &a5, &a6, &a7};

int init_pins() {
    init_pin(&debug_led);
    init_pin(&b0);
    for (int i = 0; i < 8; i++) {
        init_pin(as[i]);
    }
    return 0;
}

int init_pin(const struct gpio_dt_spec *pin) {
    if (!gpio_is_ready_dt(pin)) {
        printk("GPIO pin was not ready");
        return -1;
    }

    gpio_pin_configure_dt(pin, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set_dt(pin, false);
    return 0;
}

/// @brief Sets the pin to high
/// @param pin the gpio pin to set.
/// @return 0
int set_pin(const struct gpio_dt_spec *pin) {
    gpio_pin_set_dt(pin, true);
    return 0;
}

/// @brief Sets the pin to low
/// @param pin the gpio pin to unset
/// @return 0
int unset_pin(const struct gpio_dt_spec *pin) {
    gpio_pin_set_dt(pin, false);
    return 0;
}

/// @brief resets all pins to low.
void reset_as() {
    // Set pins correctly
    for (int i = 0; i < sizeof(as) / sizeof(as[0]); i++) {
        unset_pin(as[i]);
    }
}