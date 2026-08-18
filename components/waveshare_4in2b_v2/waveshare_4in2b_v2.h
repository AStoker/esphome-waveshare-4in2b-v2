#pragma once

// SPDX-License-Identifier: MIT
// Driver for the Waveshare 4.2inch e-Paper Module (B) V2 (400x300, black/white/red).
//
// The panel shipped with two different controllers under the same product name:
//   OLD  UC8176-style  - 0x10/0x13 plane data, 0x12 refresh, BUSY low while busy
//   NEW  SSD168x-style - 0x24/0x26 plane data, 0x22+0x20 refresh, BUSY high while busy
//
// Based on the official Waveshare Arduino library:
// https://github.com/waveshareteam/e-Paper/tree/master/Arduino/epd4in2b_V2

#include "esphome/components/display/display_buffer.h"
#include "esphome/components/spi/spi.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace waveshare_4in2b_v2 {

static constexpr uint16_t EPD_WIDTH = 400;
static constexpr uint16_t EPD_HEIGHT = 300;
/// Bytes in one colour plane. The frame buffer holds the black plane followed by the red plane.
static constexpr size_t EPD_PLANE_SIZE = static_cast<size_t>(EPD_WIDTH) * EPD_HEIGHT / 8;
static constexpr size_t EPD_BUFFER_SIZE = EPD_PLANE_SIZE * 2;

/// Which controller the attached panel uses. Resolved to OLD or NEW during setup.
enum Model : uint8_t {
  MODEL_AUTO = 0,
  MODEL_OLD,
  MODEL_NEW,
};

/// What a Color means on a three-colour panel.
enum class Ink : uint8_t { WHITE, BLACK, RED };

class Waveshare4In2BV2 : public display::DisplayBuffer,
                         public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                               spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_4MHZ> {
 public:
  void setup() override;
  void update() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

  void set_dc_pin(GPIOPin *dc_pin) { this->dc_pin_ = dc_pin; }
  void set_reset_pin(GPIOPin *reset_pin) { this->reset_pin_ = reset_pin; }
  void set_busy_pin(GPIOPin *busy_pin) { this->busy_pin_ = busy_pin; }
  void set_model(Model model) { this->model_ = model; }

  void fill(Color color) override;
  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_COLOR; }

 protected:
  int get_width_internal() override { return EPD_WIDTH; }
  int get_height_internal() override { return EPD_HEIGHT; }
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  void set_ink_(size_t pos, uint8_t mask, Ink ink);

  // Wire protocol.
  void command_(uint8_t command);
  void data_(uint8_t data);
  void write_plane_(uint8_t command, const uint8_t *plane, bool invert);

  // Panel lifecycle. Every refresh is a full reset -> init -> planes -> refresh -> sleep cycle.
  // The 15-20s refresh is awaited from loop() rather than blocked on, so update() returns in
  // well under a second and the rest of the device stays responsive while the panel settles.
  void reset_();
  Model detect_model_();
  void init_();
  void start_refresh_();
  void sleep_();
  bool wait_until_idle_();

  GPIOPin *dc_pin_{nullptr};
  GPIOPin *reset_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};

  Model model_{MODEL_AUTO};
  bool auto_detected_{false};
  /// The BUSY level that means "busy". The two controllers drive it in opposite directions.
  bool busy_level_{true};
  /// Set once the refresh command has been issued, cleared by loop() when the panel goes idle.
  bool refresh_pending_{false};
  uint32_t refresh_started_{0};
};

}  // namespace waveshare_4in2b_v2
}  // namespace esphome
