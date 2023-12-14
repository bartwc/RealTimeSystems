#include "audio.h"
#include "usb.h"
#include <errno.h>
#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define I2S_TX_NODE DT_NODELABEL(i2s_tx)
K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define AUDIO_CS DT_ALIAS(cs0)

static const struct gpio_dt_spec cs = GPIO_DT_SPEC_GET(AUDIO_CS, gpios);
const struct device *const i2s_dev_tx = DEVICE_DT_GET(I2S_TX_NODE);

static int read(uint8_t devaddr, uint8_t regaddr, uint8_t *regval) {
  int ret;

  const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

  if (!device_is_ready(dev)) {
    printuln("Device not ready");
    return -ENODEV;
  }

  ret = i2c_write_read(dev, devaddr, &regaddr, 1, regval, 1);
  if (ret) {
    printuln("Call `i2c_write_read` failed: %d", ret);
    return ret;
  }

  return 0;
}

static int write(uint8_t devaddr, uint8_t regaddr, uint8_t regval) {
  int ret;

  const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

  if (!device_is_ready(dev)) {
    printuln("Device not ready");
    return -ENODEV;
  }

  uint8_t buf[2] = {regaddr, regval};

  ret = i2c_write(dev, buf, 2, devaddr);
  if (ret) {
    printuln("Call `i2c_write` failed: %d", ret);
    return ret;
  }

  return 0;
}

void *allocBlock() {
  void *mem_block;
  int ret = k_mem_slab_alloc(&mem_slab, &mem_block, K_NO_WAIT);
  if (ret < 0) {
    printuln("Failed to allocate TX block: %d\n", ret);
    return nullptr;
  }

  // We'll eventually want to handle this return value
  ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_START);

  return mem_block;
}

int initAudio() {
  int ret;

  // Initialize CS gpio pin
  if (!gpio_is_ready_dt(&cs)) {
    printuln("CS gpio pin was not ready.");
    return ret;
  }
  ret = gpio_pin_configure_dt(&cs, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    printuln("Failed to configure CS gpio pin.");
    return ret;
  }
  ret = gpio_pin_set_dt(&cs, 1);
  if (ret < 0) {
    printuln("Failed to set CS gpio pin to 1.");
    return ret;
  }

  ret = 0;
  ret |= write(0x4A, 0x02, 0x01); // power save registers -> all on
  ret |= write(0x4A, 0x00, 0x99);
  ret |= write(0x4A, 0x47, 0x80);     // inits
  ret |= write(0x4A, 0x0d, 0x03);     // playback ctrl
  ret |= write(0x4A, 0x32, (1 << 7)); // vol
  ret |= write(0x4A, 0x32, (0 << 7)); // vol
  ret |= write(0x4A, 0x00, 0x00);     // inits
  ret |= write(0x4A, 0x04, 0xaf);     // power ctrl
  ret |= write(0x4A, 0x0d, 0x70);
  ret |=
      write(0x4A, 0x05,
            0x81); // clocking: auto speed is determined by the MCLK/LRCK ratio.
  ret |= write(0x4A, 0x06, 0x07); // DAC interface format, I²S 16 bit
  ret |= write(0x4A, 0x0a, 0x00);
  ret |= write(0x4A, 0x27, 0x00);
  ret |= write(0x4A, 0x80, 0x0a); // both channels on
  ret |= write(0x4A, 0x1f, 0x0f);
  ret |= write(0x4A, 0x22, 4 - 80);
  ret |= write(0x4A, 0x23, 4 - 80); // limit headphone volume
  ret |= write(0x4A, 0x02, 0x9e);
  if (ret < 0) {
    printuln("Failed to write config over i2c.");
    return ret;
  }

  ret = setVolume(80);
  if (ret < 0) {
    printuln("Failed to set initial volume");
  }

  struct i2s_config config;

  if (!device_is_ready(i2s_dev_tx)) {
    printuln("%s is not ready\n", i2s_dev_tx->name);
    return -1;
  }

  config.word_size = SAMPLE_BIT_WIDTH;
  config.channels = NUMBER_OF_CHANNELS;
  config.format = I2S_FMT_DATA_FORMAT_I2S;
  config.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
  config.frame_clk_freq = SAMPLE_FREQUENCY;
  config.mem_slab = &mem_slab;
  config.block_size = BLOCK_SIZE;
  config.timeout = TIMEOUT;

  ret = i2s_configure(i2s_dev_tx, I2S_DIR_TX, &config);
  if (ret < 0) {
    printuln("Failed to configure TX stream: %d\n", ret);
    return -1;
  }

  printuln("Audio driver initialization finished!");
  return 0;
}

int setVolume(uint8_t volumeValue) {
  int8_t vol;
  if (volumeValue > 100)
    volumeValue = 100;
  // strange mapping, see datasheet
  // vol=25; // -102 dB
  // vol=24  // +12dB

  vol = -90 + (float)80 * volumeValue / 100;

  int ret = 0;
  ret |= write(0x4A, 0x20, vol);
  ret |= write(0x4A, 0x21, vol);
  if (ret < 0) {
    printuln("Failed to set volume.");
    return -1;
  }
  return 0;
}

int writeBlock(void *mem_block) {
  int ret;

  // We'll eventually want to check this return value
  !i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_START);
  
  ret = i2s_write(i2s_dev_tx, mem_block, BLOCK_SIZE);
  if (ret < 0) {
    printuln("Failed to write block 0: %d", ret);
    if (ret == -5) {
      i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
      writeBlock(mem_block);
    }
    return 0;
  }
  return 0;
}