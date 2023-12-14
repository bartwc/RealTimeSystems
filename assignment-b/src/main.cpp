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

  // Buffer for writing to audio driver
  void *mem_block = allocBlock();

  printuln("== Finished initialization ==");

  int64_t time = k_uptime_get();

  int state = 0;
  while (1) {
    // Run the superloop slightly faster than once every 50 ms
    if (k_uptime_get() - time > BLOCK_GEN_PERIOD_MS-1) {
      time = k_uptime_get();

      if (state) {
        set_led(&status_led0);
      } else {
        reset_led(&status_led0);
      }
      state = !state;

      // Check the peripherals input
      set_led(&debug_led0);
      peripherals_update();
      reset_led(&debug_led0);

      // Get user input from the keyboard
      set_led(&debug_led1);
      check_keyboard();
      reset_led(&debug_led1);

      // Make synth sound
      set_led(&debug_led2);
      synth.makesynth((uint8_t *)mem_block);
      reset_led(&debug_led2);

      // Write audio block
      set_led(&debug_led3);
      writeBlock(mem_block);
      reset_led(&debug_led3);
    }
  }

  return 0;
}
