// SPDX-License-Identifier: MIT
// Waveshare 4.2inch e-Paper (B) V2 driver for ESPHome. See waveshare_4in2b_v2.h.

#include "waveshare_4in2b_v2.h"

#include <algorithm>
#include <cstring>

#include "esphome/core/application.h"
#include "esphome/core/log.h"

namespace esphome {
namespace waveshare_4in2b_v2 {

static const char *const TAG = "waveshare_4in2b_v2";

/// A three-colour refresh takes 15-20s; allow generous headroom before giving up.
static const uint32_t IDLE_TIMEOUT_MS = 30000;
/// Plane data is streamed in chunks so the red plane can be inverted without a second frame buffer.
static const size_t CHUNK_SIZE = 512;

static const char *model_name(Model model) {
  switch (model) {
    case MODEL_OLD:
      return "old";
    case MODEL_NEW:
      return "new";
    default:
      return "unknown";
  }
}

/// Map an ESPHome Color onto one of the three colours the panel can actually render.
static Ink ink_for(Color color) {
  if (color.red > 127 && color.green < 127 && color.blue < 127)
    return Ink::RED;
  if (color.red < 127 && color.green < 127 && color.blue < 127)
    return Ink::BLACK;
  return Ink::WHITE;
}

void Waveshare4In2BV2::setup() {
  this->dc_pin_->setup();
  this->reset_pin_->setup();
  this->busy_pin_->setup();
  this->spi_setup();

  this->init_internal_(EPD_BUFFER_SIZE);
  if (this->buffer_ == nullptr) {
    ESP_LOGE(TAG, "Could not allocate the %u byte frame buffer", (unsigned) EPD_BUFFER_SIZE);
    this->mark_failed();
    return;
  }
  // init_internal_() clears to COLOR_OFF, which is black ink here. Start blank instead.
  this->fill(Color(255, 255, 255));

  this->reset_();
  if (this->model_ == MODEL_AUTO) {
    this->model_ = this->detect_model_();
    this->auto_detected_ = true;
    ESP_LOGI(TAG, "Auto-detected the %s panel revision", model_name(this->model_));
  }
  this->busy_level_ = this->model_ == MODEL_NEW;

  // The panel stays unpowered until the first refresh.
}

void Waveshare4In2BV2::update() {
  if (this->is_failed())
    return;
  this->do_update_();

  // Deep sleep is only left by a hardware reset, so each refresh re-runs the whole cycle. The
  // reset and init cost well under a second against a 15-20s refresh, and in exchange the panel
  // never sits in its high-voltage state between updates -- which is what Waveshare's own
  // examples do, and what the panel is rated for.
  this->reset_();
  this->init_();

  const uint8_t *black = this->buffer_;
  const uint8_t *red = this->buffer_ + EPD_PLANE_SIZE;
  if (this->model_ == MODEL_NEW) {
    this->write_plane_(0x24, black, false);
    this->write_plane_(0x26, red, true);  // the new controller reads the red plane inverted
  } else {
    this->write_plane_(0x10, black, false);
    this->write_plane_(0x13, red, false);
  }

  this->refresh_();
  this->sleep_();
}

Model Waveshare4In2BV2::detect_model_() {
  // The official library probes register 0x2F by reading a byte back over DIN. The ESP32 driver
  // board wires no read-back path, so the best available signal is the BUSY level once the panel
  // has settled after a reset: the old controller idles high, the new one idles low. Set `model:`
  // explicitly in YAML if this guesses wrong.
  delay(10);
  const bool busy = this->busy_pin_->digital_read();
  ESP_LOGD(TAG, "BUSY reads %s after reset", ONOFF(busy));
  return busy ? MODEL_OLD : MODEL_NEW;
}

void Waveshare4In2BV2::init_() {
  if (this->model_ == MODEL_NEW) {
    this->wait_until_idle_();
    this->command_(0x12);  // software reset
    this->wait_until_idle_();

    this->command_(0x3C);  // border waveform
    this->data_(0x05);

    this->command_(0x18);  // use the built-in temperature sensor
    this->data_(0x80);

    this->command_(0x11);  // data entry mode
    this->data_(0x03);     // X increment, Y increment

    this->command_(0x44);  // RAM X window
    this->data_(0x00);
    this->data_(EPD_WIDTH / 8 - 1);

    this->command_(0x45);  // RAM Y window
    this->data_(0x00);
    this->data_(0x00);
    this->data_((EPD_HEIGHT - 1) & 0xFF);
    this->data_((EPD_HEIGHT - 1) >> 8);

    this->command_(0x4E);  // RAM X counter
    this->data_(0x00);

    this->command_(0x4F);  // RAM Y counter
    this->data_(0x00);
    this->data_(0x00);

    this->wait_until_idle_();
  } else {
    this->command_(0x04);  // power on
    this->wait_until_idle_();

    this->command_(0x00);  // panel setting
    this->data_(0x0F);     // LUT from OTP
  }
}

void Waveshare4In2BV2::refresh_() {
  if (this->model_ == MODEL_NEW) {
    this->command_(0x22);  // display update control
    this->data_(0xF7);
    this->command_(0x20);  // trigger update
  } else {
    this->command_(0x12);  // display refresh
    delay(100);            // NOLINT
  }
  if (this->wait_until_idle_())
    ESP_LOGD(TAG, "Display refresh complete");
}

void Waveshare4In2BV2::sleep_() {
  if (this->model_ == MODEL_NEW) {
    this->command_(0x10);  // deep sleep
    this->data_(0x01);
  } else {
    this->command_(0x50);  // VCOM and data interval setting
    this->data_(0xF7);     // border floating
    this->command_(0x02);  // power off
    this->wait_until_idle_();
    this->command_(0x07);  // deep sleep
    this->data_(0xA5);     // check code
  }
}

void Waveshare4In2BV2::reset_() {
  this->reset_pin_->digital_write(true);
  delay(200);  // NOLINT
  this->reset_pin_->digital_write(false);
  delay(2);
  this->reset_pin_->digital_write(true);
  delay(200);  // NOLINT
}

bool Waveshare4In2BV2::wait_until_idle_() {
  const uint32_t start = millis();
  while (this->busy_pin_->digital_read() == this->busy_level_) {
    if (millis() - start > IDLE_TIMEOUT_MS) {
      ESP_LOGE(TAG, "Panel stayed busy for %ums; is the %s revision correct?", (unsigned) IDLE_TIMEOUT_MS,
               model_name(this->model_));
      this->status_set_warning();
      return false;
    }
    delay(10);
    App.feed_wdt();
  }
  this->status_clear_warning();
  return true;
}

void Waveshare4In2BV2::command_(uint8_t command) {
  this->dc_pin_->digital_write(false);
  this->enable();
  this->write_byte(command);
  this->disable();
}

void Waveshare4In2BV2::data_(uint8_t data) {
  this->dc_pin_->digital_write(true);
  this->enable();
  this->write_byte(data);
  this->disable();
}

void Waveshare4In2BV2::write_plane_(uint8_t command, const uint8_t *plane, bool invert) {
  this->command_(command);

  uint8_t chunk[CHUNK_SIZE];
  this->dc_pin_->digital_write(true);
  this->enable();
  for (size_t offset = 0; offset < EPD_PLANE_SIZE; offset += CHUNK_SIZE) {
    const size_t length = std::min(CHUNK_SIZE, EPD_PLANE_SIZE - offset);
    if (invert) {
      for (size_t i = 0; i < length; i++)
        chunk[i] = ~plane[offset + i];
      this->write_array(chunk, length);
    } else {
      this->write_array(plane + offset, length);
    }
    App.feed_wdt();
  }
  this->disable();
}

void Waveshare4In2BV2::fill(Color color) {
  if (this->buffer_ == nullptr)
    return;
  const Ink ink = ink_for(color);
  memset(this->buffer_, ink == Ink::BLACK ? 0x00 : 0xFF, EPD_PLANE_SIZE);
  memset(this->buffer_ + EPD_PLANE_SIZE, ink == Ink::RED ? 0x00 : 0xFF, EPD_PLANE_SIZE);
}

void HOT Waveshare4In2BV2::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT)
    return;
  const size_t pos = (static_cast<size_t>(x) + static_cast<size_t>(y) * EPD_WIDTH) / 8u;
  this->set_ink_(pos, 0x80 >> (x & 0x07), ink_for(color));
}

void HOT Waveshare4In2BV2::set_ink_(size_t pos, uint8_t mask, Ink ink) {
  // A cleared bit is ink on either plane; a pixel is white when neither plane claims it.
  if (ink == Ink::BLACK) {
    this->buffer_[pos] &= ~mask;
  } else {
    this->buffer_[pos] |= mask;
  }
  if (ink == Ink::RED) {
    this->buffer_[EPD_PLANE_SIZE + pos] &= ~mask;
  } else {
    this->buffer_[EPD_PLANE_SIZE + pos] |= mask;
  }
}

void Waveshare4In2BV2::dump_config() {
  LOG_DISPLAY("", "Waveshare 4.2in e-Paper (B) V2", this);
  ESP_LOGCONFIG(TAG, "  Revision: %s%s", model_name(this->model_), this->auto_detected_ ? " (auto-detected)" : "");
  LOG_PIN("  DC Pin: ", this->dc_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_PIN("  Busy Pin: ", this->busy_pin_);
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace waveshare_4in2b_v2
}  // namespace esphome
