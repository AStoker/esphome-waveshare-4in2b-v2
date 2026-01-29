// SPDX-License-Identifier: MIT
// Custom Waveshare 4.2inch e-Paper (B) V2 driver for ESPHome
// Based on official Waveshare Arduino library:
// https://github.com/waveshareteam/e-Paper/tree/master/Arduino/epd4in2b_V2
//
// This driver properly detects and handles both OLD and NEW hardware versions
// of the 4.2inch e-Paper (B) V2 display:
// - OLD version: Uses commands 0x10/0x13, BUSY is LOW when busy
// - NEW version: Uses commands 0x24/0x26, BUSY is HIGH when busy
//
// Hardware version is auto-detected based on BUSY pin state after reset.

#include "waveshare_4in2b_v2.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace waveshare_4in2b_v2 {

static const char *const TAG = "waveshare_4in2b_v2";

void Waveshare4In2BV2::setup() {
  ESP_LOGD(TAG, "Setting up Waveshare 4.2in (B) V2...");

  this->dc_pin_->setup();
  this->reset_pin_->setup();
  this->busy_pin_->setup();

  this->spi_setup();

  // Allocate buffer: two bits per pixel (black/white + red)
  // Buffer is split: first half for black, second half for red
  this->init_internal_(this->get_buffer_length_());
  memset(this->buffer_, 0xFF, this->get_buffer_length_());

  // Initialize display with version detection
  this->init_display_();

  ESP_LOGD(TAG, "Setup complete. Version: %s", this->is_old_version_ ? "OLD" : "NEW");
}

void Waveshare4In2BV2::init_display_() {
  // Reset the display
  this->reset_();

  // Auto-detect hardware version using the official Waveshare method
  // This reads a specific register to determine which command set to use
  this->is_old_version_ = this->detect_version_();

  if (this->is_old_version_) {
    ESP_LOGI(TAG, "Detected OLD hardware version (using 0x10/0x13 commands)");
    this->init_old_();
  } else {
    ESP_LOGI(TAG, "Detected NEW hardware version (using 0x24/0x26 commands)");
    this->init_new_();
  }
}

bool Waveshare4In2BV2::detect_version_() {
  // Version detection based on busy pin behavior after reset
  // From official Waveshare library (epd4in2b_V2.cpp):
  // NEW version: BUSY is HIGH when busy, LOW when idle (flag=0)
  // OLD version: BUSY is LOW when busy, HIGH when idle (flag=1)
  
  // After a reset, we check the initial busy state
  // If busy is LOW after reset (display idle), it's the NEW version
  // If busy is HIGH after reset (display idle), it's the OLD version
  
  delay(10);
  bool busy_state = this->busy_pin_->digital_read();
  ESP_LOGD(TAG, "Initial busy state after reset: %d", busy_state);
  
  // Return true for old version (busy HIGH when idle)
  // Return false for new version (busy LOW when idle)
  return busy_state;
}

void Waveshare4In2BV2::init_new_() {
  // NEW version initialization sequence
  // From official Waveshare epd4in2b_V2.cpp Init_new()
  
  this->wait_until_idle_();
  
  this->send_command_(0x12);  // Software reset
  this->wait_until_idle_();
  
  this->send_command_(0x3C);  // BorderWaveform
  this->send_data_(0x05);
  
  this->send_command_(0x18);  // Read built-in temperature sensor
  this->send_data_(0x80);
  
  this->send_command_(0x11);  // Data entry mode
  this->send_data_(0x03);     // X increment, Y increment
  
  // Set RAM X address start/end position
  this->send_command_(0x44);
  this->send_data_(0x00);
  this->send_data_(EPD_WIDTH / 8 - 1);
  
  // Set RAM Y address start/end position
  this->send_command_(0x45);
  this->send_data_(0x00);
  this->send_data_(0x00);
  this->send_data_((EPD_HEIGHT - 1) & 0xFF);
  this->send_data_((EPD_HEIGHT - 1) >> 8);
  
  // Set RAM X address counter
  this->send_command_(0x4E);
  this->send_data_(0x00);
  
  // Set RAM Y address counter
  this->send_command_(0x4F);
  this->send_data_(0x00);
  this->send_data_(0x00);
  
  this->wait_until_idle_();
}

void Waveshare4In2BV2::init_old_() {
  // OLD version initialization sequence
  // From official Waveshare epd4in2b_V2.cpp Init_old()
  
  this->send_command_(0x04);  // Power on
  this->wait_until_idle_();
  
  this->send_command_(0x00);  // Panel setting
  this->send_data_(0x0F);     // LUT from OTP
}

void Waveshare4In2BV2::reset_() {
  this->reset_pin_->digital_write(true);
  delay(200);
  this->reset_pin_->digital_write(false);
  delay(2);
  this->reset_pin_->digital_write(true);
  delay(200);
}

void Waveshare4In2BV2::wait_until_idle_() {
  // 3-color e-paper displays can take 15-20+ seconds to refresh
  const uint32_t timeout_ms = 25000;
  
  if (this->is_old_version_) {
    // Old version: wait while busy is LOW (busy LOW = busy, HIGH = idle)
    uint32_t start = millis();
    while (this->busy_pin_->digital_read() == false) {
      if (millis() - start > timeout_ms) {
        ESP_LOGW(TAG, "Timeout waiting for display (old version)!");
        return;
      }
      delay(10);
      App.feed_wdt();
    }
  } else {
    // New version: wait while busy is HIGH (busy HIGH = busy, LOW = idle)
    uint32_t start = millis();
    while (this->busy_pin_->digital_read() == true) {
      if (millis() - start > timeout_ms) {
        ESP_LOGW(TAG, "Timeout waiting for display (new version)!");
        return;
      }
      delay(10);
      App.feed_wdt();
    }
  }
}

void Waveshare4In2BV2::send_command_(uint8_t command) {
  this->dc_pin_->digital_write(false);
  this->enable();
  this->write_byte(command);
  this->disable();
}

void Waveshare4In2BV2::send_data_(uint8_t data) {
  this->dc_pin_->digital_write(true);
  this->enable();
  this->write_byte(data);
  this->disable();
}

void Waveshare4In2BV2::update() {
  this->do_update_();
  this->display_frame_();
}

void Waveshare4In2BV2::display_frame_() {
  const uint32_t buf_half = this->get_buffer_length_() / 2;

  if (this->is_old_version_) {
    // OLD version: use 0x10 for black, 0x13 for red
    
    // Send black data
    this->send_command_(0x10);
    for (uint32_t i = 0; i < buf_half; i++) {
      this->send_data_(this->buffer_[i]);
    }
    
    // Send red data
    this->send_command_(0x13);
    for (uint32_t i = 0; i < buf_half; i++) {
      this->send_data_(this->buffer_[buf_half + i]);
    }
    
    // Refresh display
    this->send_command_(0x12);
    delay(100);
    this->wait_until_idle_();
    
  } else {
    // NEW version: use 0x24 for black, 0x26 for red
    
    // Send black data
    this->send_command_(0x24);
    for (uint32_t i = 0; i < buf_half; i++) {
      this->send_data_(this->buffer_[i]);
    }
    
    // Send red data (inverted, as per official library)
    this->send_command_(0x26);
    for (uint32_t i = 0; i < buf_half; i++) {
      this->send_data_(~this->buffer_[buf_half + i]);
    }
    
    // Refresh display
    this->send_command_(0x22);
    this->send_data_(0xF7);
    this->send_command_(0x20);
    this->wait_until_idle_();
  }
  
  ESP_LOGD(TAG, "Display refresh complete");
}

size_t Waveshare4In2BV2::get_buffer_length_() {
  // Two buffers: one for black/white, one for red
  return (EPD_WIDTH * EPD_HEIGHT / 8) * 2;
}

void Waveshare4In2BV2::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT) {
    return;
  }

  const uint32_t buf_half = this->get_buffer_length_() / 2;
  const uint32_t pos = (x + y * EPD_WIDTH) / 8;
  const uint8_t subpos = 0x80 >> (x & 0x07);

  // Color logic for 3-color display:
  // Black: black buffer = 0 (pixel on), red buffer = 1 (pixel off)
  // White: black buffer = 1 (pixel off), red buffer = 1 (pixel off)
  // Red:   black buffer = 1 (pixel off), red buffer = 0 (pixel on)

  // Determine if this is red, black, or white
  const bool is_red = (color.red > 127 && color.green < 127 && color.blue < 127);
  const bool is_black = (color.red < 127 && color.green < 127 && color.blue < 127);

  if (is_black) {
    // Black pixel: set black buffer bit to 0, red to 1
    this->buffer_[pos] &= ~subpos;           // Black buffer: clear bit (0 = black)
    this->buffer_[buf_half + pos] |= subpos; // Red buffer: set bit (1 = no red)
  } else if (is_red) {
    // Red pixel: set black buffer bit to 1, red to 0
    this->buffer_[pos] |= subpos;             // Black buffer: set bit (1 = white)
    this->buffer_[buf_half + pos] &= ~subpos; // Red buffer: clear bit (0 = red)
  } else {
    // White pixel: both buffers have bit set to 1
    this->buffer_[pos] |= subpos;            // Black buffer: set bit (1 = white)
    this->buffer_[buf_half + pos] |= subpos; // Red buffer: set bit (1 = no red)
  }
}

void Waveshare4In2BV2::dump_config() {
  LOG_DISPLAY("", "Waveshare 4.2in (B) V2 Custom", this);
  ESP_LOGCONFIG(TAG, "  Hardware Version: %s", this->is_old_version_ ? "OLD" : "NEW");
  LOG_PIN("  DC Pin: ", this->dc_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_PIN("  Busy Pin: ", this->busy_pin_);
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace waveshare_4in2b_v2
}  // namespace esphome
