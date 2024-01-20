#include "Switch.hpp"
#include "usb.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

bool get_button(const struct gpio_dt_spec *button) {
    return gpio_pin_get_dt(button);
}

int ThreePosSwitch::initialize(const struct gpio_dt_spec *up,
                               const struct gpio_dt_spec *down,
                               sw3_callback callback) {
    _callback = callback;
    _up = up;
    _down = down;

    // Configure the first GPIO
    if (!gpio_is_ready_dt(_up)) {
        printuln("GPIO up was not ready.");
        return -1;
    }
    gpio_pin_configure_dt(_up, GPIO_INPUT | GPIO_PULL_UP);

    // Configure the second GPIO
    if (!gpio_is_ready_dt(_down)) {
        printuln("GPIO down was not ready.");
        return -1;
    }
    gpio_pin_configure_dt(_down, GPIO_INPUT | GPIO_PULL_UP);

    update();
    return 0;
}

void ThreePosSwitch::update() {
    // Get the new state
//    bool temp_up = gpio_pin_get_dt(_up);
//    bool temp_dn = gpio_pin_get_dt(_down);
//    int num_check = 0;
    // software debouncing
//    while(num_check <= 9){
//        if(temp_up == gpio_pin_get_dt(_up) && temp_dn == gpio_pin_get_dt(_down)){
//            num_check = num_check + 1;
//            k_usleep(20);
//        } else {
//            temp_up = gpio_pin_get_dt(_up);
//            temp_dn = gpio_pin_get_dt(_down);
//            num_check = 0;
//        }
//    }
    bool up = gpio_pin_get_dt(_up);
    bool dn = gpio_pin_get_dt(_down);

    if (up == dn) {
        _current_state = Neutral;
    } else if (up) {
        _current_state = Up;
    } else {
        _current_state = Down;
    }

    // If the state is different, call the callback
    if (_current_state != _previous && _callback != nullptr) {
        _callback(*this);
    }

    // Update the invariant
    _previous = _current_state;
}