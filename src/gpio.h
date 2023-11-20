#include <zephyr/drivers/gpio.h>

int init_pins();

int init_pin(const struct gpio_dt_spec *pin);

int set_pin(const struct gpio_dt_spec *pin);

int unset_pin(const struct gpio_dt_spec *pin);

void reset_as();

extern const struct gpio_dt_spec debug_led;
extern const struct gpio_dt_spec a0;
extern const struct gpio_dt_spec a1;
extern const struct gpio_dt_spec a2;
extern const struct gpio_dt_spec a3;
extern const struct gpio_dt_spec a4;
extern const struct gpio_dt_spec a5;
extern const struct gpio_dt_spec a6;
extern const struct gpio_dt_spec a7;
extern const struct gpio_dt_spec b0;
extern const struct gpio_dt_spec *as[8];