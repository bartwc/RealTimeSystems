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

K_MUTEX_DEFINE(mutex_keys);
K_MUTEX_DEFINE(mutex_rotary);
K_MUTEX_DEFINE(mutex_mem1);
K_MUTEX_DEFINE(mutex_mem0);

Synthesizer synth;

void check_keyboard() {
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

    printuln("== Finished initialization ==");

    k_tid_t my_tid_0 = k_thread_create(&my_thread_data_0, stack0,
                                       K_THREAD_STACK_SIZEOF(stack0),
                                       task_update_peripherals,
                                       NULL, NULL, NULL,
                                       4, 0, K_NO_WAIT);
    k_tid_t my_tid_1 = k_thread_create(&my_thread_data_1, stack1,
                                       K_THREAD_STACK_SIZEOF(stack1),
                                       task_check_keyboard,
                                       NULL, NULL, NULL,
                                       4, 0, K_NO_WAIT);
    k_tid_t my_tid_2 = k_thread_create(&my_thread_data_2, stack2,
                                       K_THREAD_STACK_SIZEOF(stack2),
                                       task_make_audio,
                                       NULL, NULL, mem_block,
                                       5, 0, K_NO_WAIT);
    k_tid_t my_tid_3 = k_thread_create(&my_thread_data_3, stack3,
                                       K_THREAD_STACK_SIZEOF(stack3),
                                       task_write_audio,
                                       NULL, NULL, mem_block,
                                       5, 0, K_NO_WAIT);

    k_thread_suspend(k_current_get());
    return 0;
}

void task_update_peripherals(void *p1, void *p2, void *p3) {
    int64_t time;
    while (1) {
        time = k_uptime_get();
        // Check the peripherals input
        set_led(&debug_led0);
        k_mutex_lock(&mutex_rotary, K_FOREVER);
        peripherals_update();
        k_mutex_unlock(&mutex_rotary);
        reset_led(&debug_led0);

        k_sleep(K_MSEC(BLOCK_GEN_PERIOD_MS - (k_uptime_get() - time)));
    }
}

void task_check_keyboard(void *p1, void *p2, void *p3) {
    int64_t time;
    while (1) {
        time = k_uptime_get();
        // Get user input from the keyboard
        set_led(&debug_led1);
        k_mutex_lock(&mutex_keys, K_FOREVER);
        check_keyboard();
        k_mutex_unlock(&mutex_keys);
        reset_led(&debug_led1);

        k_sleep(K_MSEC(BLOCK_GEN_PERIOD_MS - (k_uptime_get() - time)));
    }
}

void task_make_audio(void *p1, void *p2, void *mem_block) {
    int64_t time;
    bool is_mem0 = true;
    while (1) {
        time = k_uptime_get();
        // Make synth sound
        set_led(&debug_led2);
        if (is_mem0) {
            k_mutex_lock(&mutex_mem0, K_FOREVER);
            k_mutex_lock(&mutex_keys, K_FOREVER);
            k_mutex_lock(&mutex_rotary, K_FOREVER);
            synth.makesynth((uint8_t *) mem_block);
            k_mutex_unlock(&mutex_rotary);
            k_mutex_unlock(&mutex_keys);
            k_mutex_unlock(&mutex_mem0);
            is_mem0 = false;
        } else {
            k_mutex_lock(&mutex_mem1, K_FOREVER);
            k_mutex_lock(&mutex_keys, K_FOREVER);
            k_mutex_lock(&mutex_rotary, K_FOREVER);
            synth.makesynth(((uint8_t *) mem_block) + int (BLOCK_SIZE));
            k_mutex_unlock(&mutex_rotary);
            k_mutex_unlock(&mutex_keys);
            k_mutex_unlock(&mutex_mem1);
            is_mem0 = true;
        }
        reset_led(&debug_led2);
        k_sleep(K_MSEC(BLOCK_GEN_PERIOD_MS - (k_uptime_get() - time)));
    }
}

void task_write_audio(void *p1, void *p2, void *mem_block) {
    int64_t time;
    bool is_mem0 = false;
    bool is_first = true;
    while (1) {
        time = k_uptime_get();
        set_led(&debug_led3);
        if (is_first) {
            is_first = false;
        } else {
            // Write audio block
            if (is_mem0){
                k_mutex_lock(&mutex_mem0, K_FOREVER);
                writeBlock(mem_block);
                k_mutex_unlock(&mutex_mem0);
                is_mem0 = false;
            } else {
                k_mutex_lock(&mutex_mem1, K_FOREVER);
                writeBlock(((uint8_t *) mem_block) + int (BLOCK_SIZE));
                k_mutex_unlock(&mutex_mem1);
                is_mem0 = true;
            }

        }
        reset_led(&debug_led3);
        k_sleep(K_MSEC(BLOCK_GEN_PERIOD_MS - (k_uptime_get() - time)));
    }
}