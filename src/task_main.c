#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "gpio.h"
#include "scheduler_impl.h"

/// @brief The function the task executes, it set the pin high until it has exceeded its execution time, after which it will invoke the scheduler again.
/// In a realistic situation this function would execute application code and different tasks would execute different code.
/// @param pin The pin to set to high during the task execution.
/// @param execution_time The duration of the task execution.
void task_main(const struct gpio_dt_spec *pin, int execution_time) {
    while(true) {
        int64_t target = k_uptime_get() + execution_time;
        int64_t last = k_uptime_get();
        while (true) {
            // Set leds correctly
            set_pin(pin);

            int64_t current = k_uptime_get();

            // Task was preempted, we don't count this as execution time
            if (current - last > 1) {
                target += (current - last);
            }
            last = current;

            // If this task finished, break the loop
            if (current >= target) {
                break;
            }
        }

        unset_pin(pin);
        k_wakeup(scheduler_thread);
    }
}