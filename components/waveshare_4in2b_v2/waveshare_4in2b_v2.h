#pragma once

// SPDX-License-Identifier: MIT
// Custom driver for Waveshare 4.2inch e-Paper (B) V2
// Supports both OLD and NEW hardware revisions with automatic detection
// Based on official Waveshare Arduino library:
// https://github.com/waveshareteam/e-Paper/tree/master/Arduino/epd4in2b_V2

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/display/display_buffer.h"

namespace esphome {
namespace waveshare_4in2b_v2 {

// Display resolution
static const uint16_t EPD_WIDTH = 400;
static const uint16_t EPD_HEIGHT = 300;

class Waveshare4In2BV2 : public display::DisplayBuffer,
                         public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                               spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_2MHZ> {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

  void set_dc_pin(GPIOPin *dc_pin) { dc_pin_ = dc_pin; }
  void set_reset_pin(GPIOPin *reset_pin) { reset_pin_ = reset_pin; }
  void set_busy_pin(GPIOPin *busy_pin) { busy_pin_ = busy_pin; }
  void set_full_update_every(uint32_t full_update_every) { full_update_every_ = full_update_every; }

  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_COLOR; }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  int get_width_internal() override { return EPD_WIDTH; }
  int get_height_internal() override { return EPD_HEIGHT; }
  size_t get_buffer_length_();

  void init_display_();
  void send_command_(uint8_t command);
  void send_data_(uint8_t data);
  void reset_();
  void wait_until_idle_();
  bool detect_version_();
  void init_new_();
  void init_old_();
  void display_frame_();

  GPIOPin *dc_pin_{nullptr};
  GPIOPin *reset_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};

  uint32_t full_update_every_{1};
  uint32_t at_update_{0};

  // Flag to determine which command set to use
  // false = new version (commands 0x24, 0x26)
  // true = old version (commands 0x10, 0x13)
  bool is_old_version_{false};
};

}  // namespace waveshare_4in2b_v2
}  // namespace esphome
