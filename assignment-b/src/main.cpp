/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Switch.hpp"
#include "audio.h"
#include "key.hpp"
#include "leds.h"
#include "peripherals.h"
#include "synth.hpp"
#include "usb.h"
#include <math.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#define STACK_SIZE 2048

void task_update_peripherals(void *, void *, void *);

void task_check_keyboard(void *, void *, void *);

void task_write_audio(void *, void *, void *);

void task_make_audio(void *, void *, void *);

K_THREAD_STACK_DEFINE(stack0,
STACK_SIZE);
K_THREAD_STACK_DEFINE(stack1,
STACK_SIZE);
K_THREAD_STACK_DEFINE(stack2,
STACK_SIZE);
K_THREAD_STACK_DEFINE(stack3,
STACK_SIZE);

struct k_thread my_thread_data_0;
struct k_thread my_thread_data_1;
struct k_thread my_thread_data_2;
struct k_thread my_thread_data_3;



static struct gpio_callback sw0_cb;
static struct gpio_callback sw1_cb;
static struct gpio_callback sw2_cb;
static struct gpio_callback sw3_cb;
static struct gpio_callback sw4_cb;
static struct gpio_callback sw5_cb;
static struct gpio_callback sw6_cb;
static struct gpio_callback sw7_cb;
static struct gpio_callback rot_cb;
void switch_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void attach_interrupt_switch(void);

K_MUTEX_DEFINE(mutex_keys);
K_MUTEX_DEFINE(mutex_rotary);
K_SEM_DEFINE(sem_mem0, 0, 1);
K_SEM_DEFINE(sem_mem1, 0, 1);
K_SEM_DEFINE(sem_peripherals, 0, 1);

K_TIMER_DEFINE(sync_timer_task1, NULL, NULL);
K_TIMER_DEFINE(sync_timer_task2, NULL, NULL);
K_TIMER_DEFINE(sync_timer_task3, NULL, NULL);
K_TIMER_DEFINE(sync_timer_task4, NULL, NULL);

Synthesizer synth;

void check_keyboard() {
    // data race Keys[] in thread task_check_keyboard(main.cpp)
    char character;
    while (usbRead(&character, 1)) {
        auto key = Key::char_to_key(character);
        bool key_pressed = false;
        for (int i = 0; i < MAX_KEYS; i++) {
            if (key == keys[i].key && keys[i].state != IDLE) {
                keys[i].state = PRESSED;
                keys[i].hold_time = sys_timepoint_calc(K_MSEC(500));
                keys[i].release_time = sys_timepoint_calc(K_MSEC(500));
                key_pressed = true;
            }
        }
        // The second loop is necessary to avoid selecting an IDLE key when a
        // PRESSED or RELEASED key is located further away on the array
        if (!key_pressed) {
            for (int i = 0; i < MAX_KEYS; i++) {
                if (keys[i].state == IDLE) {
                    keys[i].key = key;
                    keys[i].state = PRESSED;
                    keys[i].hold_time = sys_timepoint_calc(K_MSEC(500));
                    keys[i].release_time = sys_timepoint_calc(K_MSEC(500));
                    keys[i].phase1 = 0;
                    keys[i].phase2 = 0;
                    break;
                }
            }
        }
    }
}

int main(void) {
    initUsb();
    waitForUsb();

    printuln("== Initializing... ==");

    init_leds();
    initAudio();
    init_peripherals();

    synth.initialize();

    void *mem_block = allocBlock();

    //attach_interrupt_switch();

    printuln("== Finished initialization ==");

    k_tid_t my_tid_0 = k_thread_create(&my_thread_data_0, stack0,
                                       K_THREAD_STACK_SIZEOF(stack0),
                                       task_update_peripherals,
                                       NULL, NULL, NULL,
                                       -1, 0, K_NO_WAIT);
    k_tid_t my_tid_1 = k_thread_create(&my_thread_data_1, stack1,
                                       K_THREAD_STACK_SIZEOF(stack1),
                                       task_check_keyboard,
                                       NULL, NULL, NULL,
                                       2, 0, K_NO_WAIT);
    k_tid_t my_tid_2 = k_thread_create(&my_thread_data_2, stack2,
                                       K_THREAD_STACK_SIZEOF(stack2),
                                       task_make_audio,
                                       NULL, NULL, mem_block,
                                       3, 0, K_NO_WAIT);
    k_tid_t my_tid_3 = k_thread_create(&my_thread_data_3, stack3,
                                       K_THREAD_STACK_SIZEOF(stack3),
                                       task_write_audio,
                                       NULL, NULL, mem_block,
                                       4, 0, K_NO_WAIT);
    attach_interrupt_switch();
    k_thread_suspend(k_current_get());
    return 0;
}

void task_update_peripherals(void *p1, void *p2, void *p3) {
    //k_timer_start(&sync_timer_task1, K_MSEC(5), K_MSEC(5));
    while (1) {
        // Check the peripherals input
        if (k_sem_take(&sem_peripherals, K_FOREVER) == 0) {
            set_led(&debug_led0);
            //k_mutex_lock(&mutex_rotary, K_FOREVER);
            peripherals_update();
            //k_mutex_unlock(&mutex_rotary);
            reset_led(&debug_led0);
        }


        //k_timer_status_sync(&sync_timer_task1);
    }
}

void task_check_keyboard(void *p1, void *p2, void *p3) {
    k_timer_start(&sync_timer_task2, K_MSEC(50), K_MSEC(50));
    while (1) {
        // Get user input from the keyboard
        set_led(&debug_led1);
        //k_mutex_lock(&mutex_keys, K_FOREVER);
        check_keyboard();
        //k_mutex_unlock(&mutex_keys);
        reset_led(&debug_led1);

        k_timer_status_sync(&sync_timer_task2);
    }
}

void task_make_audio(void *p1, void *p2, void *mem_block) {
    k_timer_start(&sync_timer_task3, K_MSEC(BLOCK_GEN_PERIOD_MS), K_MSEC(BLOCK_GEN_PERIOD_MS));
    bool is_mem0 = true;
    while (1) {
        // Make synth sound
        set_led(&debug_led2);
        if (is_mem0) {
            k_mutex_lock(&mutex_keys, K_FOREVER);
            k_mutex_lock(&mutex_rotary, K_FOREVER);
            synth.makesynth((uint8_t *) mem_block);
            k_mutex_unlock(&mutex_rotary);
            k_mutex_unlock(&mutex_keys);
            k_sem_give(&sem_mem0);
            is_mem0 = false;
        } else {
            k_mutex_lock(&mutex_keys, K_FOREVER);
            k_mutex_lock(&mutex_rotary, K_FOREVER);
            synth.makesynth(((uint8_t *) mem_block) + int (BLOCK_SIZE));
            k_mutex_unlock(&mutex_rotary);
            k_mutex_unlock(&mutex_keys);
            k_sem_give(&sem_mem1);
            is_mem0 = true;
        }
        reset_led(&debug_led2);
        k_timer_status_sync(&sync_timer_task3);
    }
}

void task_write_audio(void *p1, void *p2, void *mem_block) {
    k_timer_start(&sync_timer_task4, K_MSEC(50), K_MSEC(50));
    while (1) {
        set_led(&debug_led3);
        if (k_sem_take(&sem_mem0, K_NO_WAIT) == 0){
            writeBlock(mem_block);
        }
        if (k_sem_take(&sem_mem1, K_NO_WAIT) == 0) {
            writeBlock(((uint8_t *) mem_block) + int (BLOCK_SIZE));
        }
        reset_led(&debug_led3);
        k_timer_status_sync(&sync_timer_task4);
    }
}

void attach_interrupt_switch(void) {
    int ret0 = gpio_pin_interrupt_configure_dt(&sw_osc_dn, GPIO_INT_EDGE_BOTH);
    int ret1 = gpio_pin_interrupt_configure_dt(&sw_osc_up, GPIO_INT_EDGE_BOTH);
    int ret2 = gpio_pin_interrupt_configure_dt(&sw1_dn, GPIO_INT_EDGE_BOTH);
    int ret3 = gpio_pin_interrupt_configure_dt(&sw1_up, GPIO_INT_EDGE_BOTH);
    int ret4 = gpio_pin_interrupt_configure_dt(&sw2_dn, GPIO_INT_EDGE_BOTH);
    int ret5 = gpio_pin_interrupt_configure_dt(&sw2_up, GPIO_INT_EDGE_BOTH);
    int ret6 = gpio_pin_interrupt_configure_dt(&sw3_dn, GPIO_INT_EDGE_BOTH);
    int ret7 = gpio_pin_interrupt_configure_dt(&sw3_up, GPIO_INT_EDGE_BOTH);
    int ret8 = gpio_pin_interrupt_configure_dt(&rot_int, GPIO_INT_EDGE_FALLING);

    if (ret0 != 0 || ret1 != 0 || ret2 != 0 || ret3 != 0 || ret4 != 0 || ret5 != 0 || ret6 != 0 || ret7 != 0 || ret8 != 0) {
        printuln("failed to configure interrupt on switches.\n");
        return;
    }

    // initialize callback structure for button interrupt
    gpio_init_callback(&sw0_cb, switch_pressed, BIT(sw_osc_dn.pin));
    gpio_init_callback(&sw1_cb, switch_pressed, BIT(sw_osc_up.pin));
    gpio_init_callback(&sw2_cb, switch_pressed, BIT(sw1_up.pin));
    gpio_init_callback(&sw3_cb, switch_pressed, BIT(sw2_up.pin));
    gpio_init_callback(&sw4_cb, switch_pressed, BIT(sw3_up.pin));
    gpio_init_callback(&sw5_cb, switch_pressed, BIT(sw1_dn.pin));
    gpio_init_callback(&sw6_cb, switch_pressed, BIT(sw2_dn.pin));
    gpio_init_callback(&sw7_cb, switch_pressed, BIT(sw3_dn.pin));
    gpio_init_callback(&rot_cb, switch_pressed, BIT(rot_int.pin));

    // attach callback function to button interrupt
    gpio_add_callback(sw_osc_dn.port, &sw0_cb);
    gpio_add_callback(sw_osc_up.port, &sw1_cb);
    gpio_add_callback(sw1_up.port, &sw2_cb);
    gpio_add_callback(sw2_up.port, &sw3_cb);
    gpio_add_callback(sw3_up.port, &sw4_cb);
    gpio_add_callback(sw1_dn.port, &sw5_cb);
    gpio_add_callback(sw2_dn.port, &sw6_cb);
    gpio_add_callback(sw3_dn.port, &sw7_cb);
    gpio_add_callback(rot_int.port, &rot_cb);
}

void switch_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    k_sem_give(&sem_peripherals);
    return;
}