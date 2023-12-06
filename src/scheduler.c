#include "scheduler_impl.h"
#include <zephyr/kernel.h>
#include "stdbool.h"
#include "gpio.h"
#include "usb.h"

// Index to indicate which task in the tasks array is active.
static int current_task = 0;

/// This is a very simple scheduler that demonstrates how the provided APIs can be used.
/// The function should call `set_active_task` to select which task should be running.
/// Then, it should return a sleep time in milliseconds, when the scheduler should next be executed.
k_timeout_t example(Task *tasks, int n, bool finished) {
    set_active_task(&tasks[current_task]);
    current_task = (current_task + 1) % n;
    return K_FOREVER;
}

static int start_time = -1;

/// In this function you should write a rate monotonic scheduler.
/// Read the description of the `schedule` function below to read about the properties of this function.
k_timeout_t rate_monotonic(Task *tasks, int n, bool finished) {
    int i;
    int current_time = k_uptime_get();
    // You can initialize the new fields of your tasks here. This only runs the first time.
    if (start_time == -1) {
        start_time = current_time;
        i = 0;
        while (i <= n - 1) {
            tasks[i].next_arrival_time = start_time + tasks[i].period;
            tasks[i].is_finished = false;
            i = i + 1;
        }
    } else if (finished == true) {
        tasks[current_task].is_finished = true;
    }

    i = 0;
    while (i <= n - 1) {
        if (tasks[i].next_arrival_time <= current_time) {
            tasks[i].next_arrival_time = tasks[i].next_arrival_time + tasks[i].period;
            tasks[i].is_finished = false;
        }
        i = i + 1;
    }

    // select the task with the least period
    int least_period = INT_MAX;
    int temp_task = -1;
    i = 0;
    while (i <= n - 1) {
        if (tasks[i].period <= least_period && tasks[i].is_finished == false) {
            least_period = tasks[i].period;
            temp_task = i;
        }
        i = i + 1;
    }

    i = 0;
    int scheduler_next_awake = INT_MAX;
    while (i <= n - 1) {
        if (temp_task == -1) {
            if (tasks[i].next_arrival_time < scheduler_next_awake) {
                scheduler_next_awake = tasks[i].next_arrival_time;
            }
        } else {
            if (tasks[i].next_arrival_time < scheduler_next_awake && tasks[i].period < tasks[temp_task].period) {
                scheduler_next_awake = tasks[i].next_arrival_time;
            }
        }
        i = i + 1;
    }

    if (temp_task != -1) {
        current_task = temp_task;
        set_active_task(&tasks[current_task]);
    } else {
        set_idle();
    }

    return K_MSEC(scheduler_next_awake - current_time);
}

/// In this function you should write an earliest deadline first scheduler.
/// Read the description of the `schedule` function below to read about the properties of this function.
k_timeout_t earliest_deadline_first(Task *tasks, int n, bool finished) {
    int i;
    int current_time = k_uptime_get();
    // You can initialize the new fields of your tasks here. This only runs the first time.
    if (start_time == -1) {
        start_time = current_time;
        i = 0;
        while (i <= n - 1) {
            tasks[i].next_arrival_time = start_time + tasks[i].period;
            tasks[i].is_finished = false;
            i = i + 1;
        }
    } else if (finished == true) {
        tasks[current_task].is_finished = true;
    }

    i = 0;
    while (i <= n - 1) {
        if (tasks[i].next_arrival_time <= current_time) {
            tasks[i].next_arrival_time = tasks[i].next_arrival_time + tasks[i].period;
            tasks[i].is_finished = false;
        }
        i = i + 1;
    }

    // select the task with the least period
    int earliest_deadline = INT_MAX;
    int temp_task = -1;
    i = 0;
    while (i <= n - 1) {
        if (tasks[i].next_arrival_time <= earliest_deadline && tasks[i].is_finished == false) {
            earliest_deadline = tasks[i].next_arrival_time;
            temp_task = i;
        }
        i = i + 1;
    }

    i = 0;
    int scheduler_next_awake = INT_MAX;
    while (i <= n - 1) {
        if (temp_task == -1) {
            if (tasks[i].next_arrival_time < scheduler_next_awake) {
                scheduler_next_awake = tasks[i].next_arrival_time;
            }
        } else {
            if (tasks[i].next_arrival_time < scheduler_next_awake &&
                tasks[i].next_arrival_time + tasks[i].period < tasks[temp_task].next_arrival_time) {
                scheduler_next_awake = tasks[i].next_arrival_time;
            }
        }
        i = i + 1;
    }

    if (temp_task != -1) {
        current_task = temp_task;
        set_active_task(&tasks[current_task]);
    } else {
        set_idle();
    }

    return K_MSEC(scheduler_next_awake - current_time);
}

/// @brief This function can be used to choose which scheduler should be used.
///        This function is invoked when a task is finished or when the scheduler sleep timer has ended.
///        The sleep time is the duration of the timeout which the function returns (see the `\@return` for more information).
///        The called scheduler function should set the next task by calling `set_active_task` or call `set_idle` when there are no pending tasks.
/// @param tasks an array of tasks containing the full task set to schedule.
/// @param n the amount of tasks.
/// @param finished states whether the scheduler was called because a task was finished (true)
///                 or because the scheduler sleep timer expired (false).
/// @return The delay for which the scheduler thread should sleep.
///         You should make the scheduler sleep until a higher priority task is ready (preemption of the current running task)
///         or until there is a task ready to be scheduled when there were no tasks pending.
///         You must calculate this delay and return a k_timeout_t value containing this delay. Learn more about this here: 
///         https://docs.zephyrproject.org/latest/kernel/services/timing/clocks.html. Some of the functions/macros that you might find useful are
///         `K_MSEC`, `K_USEC`, `sys_timepoint_calc` and `sys_timepoint_timeout`.
k_timeout_t schedule(Task *tasks, int n, bool finished) {
    return earliest_deadline_first(tasks, n, finished);
}