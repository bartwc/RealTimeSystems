#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel/thread.h>
#include "gpio.h"
#include "task_main.h"
#include "scheduler_impl.h"
#include "scheduler.h"
#include "stdbool.h"

#define STACK_SIZE 512
#define MAX_THREADS 8

static int thread_counter = 0;
int scheduler_thread = 0;

K_THREAD_STACK_DEFINE(stack0, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack1, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack2, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack3, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack4, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack5, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack6, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack7, STACK_SIZE);

k_thread_stack_t *stacks[MAX_THREADS] = {&stack0, &stack1, &stack2, &stack3, &stack4, &stack5, &stack6, &stack7};
struct k_thread kts[MAX_THREADS];

/// @brief Creates a new instance of a `Task` struct.
/// @param execution_time the execution time in milliseconds
/// @param period the period of the task in milliseconds
/// @param pin the gpio pin assigned to this task, suitable pins can be found in gpio.h
/// @return The created task struct.
Task spawn_task(int execution_time, int period, const struct gpio_dt_spec *pin) {
    Task task;

    task.execution_time = execution_time;
    task.period = period;

    if (thread_counter == MAX_THREADS) {
        printk("Spawned too many threads!");
        return;
    }
    k_thread_stack_t *stack = stacks[thread_counter];
    struct k_thread *thread = &kts[thread_counter];

    k_tid_t tid = k_thread_create(thread, stack, STACK_SIZE, (void *) task_main, (void *) pin, execution_time, 0, 5, 0,
                                  K_NO_WAIT);
    task.task_id = tid;

    thread_counter++;

    k_thread_suspend(tid);

    return task;
}

static k_tid_t current_task = NULL;

/// @brief Dispatches the given task once the scheduler thread suspends. 
///        It suspends the thread of the task running before.
/// @param task The task to be dispatched.
void set_active_task(Task *task) {
    if (current_task != NULL) {
        k_thread_suspend(current_task);
    }
    current_task = task->task_id;
    k_thread_resume(current_task);
}

/// @brief Idles the system.
void set_idle() {
    if (current_task != NULL) {
        k_thread_suspend(current_task);
    }
    current_task = NULL;
}

/// @brief Run the scheduling algorithm on the given tasks.
/// @param tasks The task set to be scheduled as a fixed size array.
/// @param n The number of tasks.
void run_scheduler(Task *tasks, int n) {
    scheduler_thread = k_current_get();
    bool finished = false;
    while (true) {
        reset_as();
        k_timeout_t sleep = schedule(tasks, n, finished);
        finished = k_sleep(sleep) != 0;
    }
}