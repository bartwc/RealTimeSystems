/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "gpio.h"
#include "task_main.h"
#include "scheduler_impl.h"
#include "scheduler.h"
#include "usb.h"

int main() {
    init_pins();

    // The provided task set:
    Task tasks[] = {
            spawn_task(2, 20, &a0),
            spawn_task(10, 40, &a1),
            spawn_task(30, 60, &a2),
    };
    run_scheduler(tasks, sizeof(tasks) / sizeof(tasks[0]));

    k_thread_suspend(k_current_get());

    return 0;
}
