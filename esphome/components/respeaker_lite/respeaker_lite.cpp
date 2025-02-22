#include "respeaker_lite.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace respeaker_lite {

static const char* const TAG = "respeaker_lite";
bool initialized = false;

void RespeakerLite::setup() {
  ESP_LOGI(TAG, "Setting up RespeakerLite...");
  // Reset device using the reset pin
  this->reset_pin_->setup();
  this->reset_pin_->digital_write(true);
  delay(1);
  this->reset_pin_->digital_write(false);
  // Wait for XMOS to boot...
  this->set_timeout(3000, [this]() {
    if (this->get_firmware_version_()) {
      initialized = true;
    } else {
      ESP_LOGE(TAG, "Failed to initialize Respeaker");
      this->mark_failed();
    }
  });
}

bool RespeakerLite::get_firmware_version_() {
  if (this->firmware_version_ != nullptr) {
    ESP_LOGI(TAG, "Reading firmware version");
    const uint8_t version_req[] = {0xF0, 0xD8, 4};
    uint8_t version_resp[4];

    auto error_code = this->write(version_req, sizeof(version_req));
    if (error_code != i2c::ERROR_OK) {
      ESP_LOGW(TAG, "Request version failed");
      return false;
    }

    error_code = this->read(version_resp, sizeof(version_resp));
    if (error_code != i2c::ERROR_OK || version_resp[0] != 0) {
      ESP_LOGW(TAG, "Read version failed");
      return false;
    }
    
    std::string version = str_sprintf("%u.%u.%u", version_resp[1], version_resp[2], version_resp[3]);
    ESP_LOGI(TAG, "Firmware version: %s", version.c_str());
    this->firmware_version_->publish_state(version);
  } else {
    ESP_LOGI(TAG, "Firmware version - skipping");
  }
  return true;
}

void RespeakerLite::get_mic_mute_state_() {
  uint8_t mute_req[3] = {0xF1, 0x81, 1};
  uint8_t mute_resp[2];

  auto error_code = this->write(mute_req, sizeof(mute_req));
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Request mic mute state failed");
    return;
  }

  error_code = this->read(mute_resp, sizeof(mute_resp));
  if (error_code != i2c::ERROR_OK || mute_resp[0] != 0) {
    ESP_LOGW(TAG, "Read mic mute state failed");
    return;
  }

  bool new_mute_state = mute_resp[1] == 0x01;
  if (this->mute_state_ != nullptr) {
    if (!this->mute_state_->has_state() || (this->mute_state_->state != new_mute_state)) {
      ESP_LOGI(TAG, "Mic mute state: %d", new_mute_state);
      this->mute_state_->publish_state(new_mute_state);
    }
  }
}

void RespeakerLite::mute_speaker() {
  uint8_t mute_req[4] = {0xF1, 0x10, 1, 0};

  auto error_code = this->write(mute_req, sizeof(mute_req));
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Mute speaker failed");
  }
}

void RespeakerLite::unmute_speaker() {
  uint8_t unmute_req[4] = {0xF1, 0x10, 1, 1};

  auto error_code = this->write(unmute_req, sizeof(unmute_req));
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Unmute speaker failed");
  }
}

void RespeakerLite::loop() {
  if (initialized) {
    this->get_mic_mute_state_();
  }
}

}  // namespace respeaker_lite
}  // namespace esphome