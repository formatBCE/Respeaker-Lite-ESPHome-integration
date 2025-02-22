#include "respeaker_lite.h"

#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include <cinttypes>

namespace esphome {
namespace respeaker_lite {

static const char *const TAG = "respeaker_lite";
bool initialized = false;

void RespeakerLite::setup() {
  ESP_LOGCONFIG(TAG, "Setting up RespeakerLite...");
  // Reset device using the reset pin
  this->reset_pin_->setup();
  this->reset_pin_->digital_write(true);
  delay(1);
  this->reset_pin_->digital_write(false);
  // Wait for XMOS to boot...
  this->set_timeout(3000, [this]() {
    if (!this->dfu_get_version_()) {
      ESP_LOGE(TAG, "Communication with Respeaker Lite failed");
      this->mark_failed();
    } else if (!this->versions_match_() && this->firmware_bin_is_valid_()) {
      ESP_LOGW(TAG, "Expected XMOS version: %u.%u.%u; found: %u.%u.%u. Updating...", this->firmware_bin_version_major_,
               this->firmware_bin_version_minor_, this->firmware_bin_version_patch_, this->firmware_version_major_,
               this->firmware_version_minor_, this->firmware_version_patch_);
      this->start_dfu_update();
    } else {
      initialized = true;
    }
  });
}

void RespeakerLite::dump_config() {
  ESP_LOGCONFIG(TAG, "Respeaker Lite:");
  LOG_I2C_DEVICE(this);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  if (this->firmware_version_major_ || this->firmware_version_minor_ || this->firmware_version_patch_) {
    ESP_LOGCONFIG(TAG, "  XMOS firmware version: %u.%u.%u", this->firmware_version_major_,
                  this->firmware_version_minor_, this->firmware_version_patch_);
  }
}

void RespeakerLite::loop() {
  switch (this->dfu_update_status_) {
    case UPDATE_IN_PROGRESS:
    case UPDATE_REBOOT_PENDING:
    case UPDATE_VERIFY_NEW_VERSION:
      this->dfu_update_status_ = this->dfu_update_send_block_();
      break;

    case UPDATE_COMMUNICATION_ERROR:
    case UPDATE_TIMEOUT:
    case UPDATE_FAILED:
    case UPDATE_BAD_STATE:
#ifdef USE_RESPEAKER_LITE_STATE_CALLBACK
      this->state_callback_.call(DFU_ERROR, this->bytes_written_ * 100.0f / this->firmware_bin_length_,
                                 this->dfu_update_status_);
#endif
      this->mark_failed();
      break;

    default:
      if (initialized) {
        this->get_mic_mute_state_();
      }
      break;
  }
}

uint8_t RespeakerLite::read_vnr() {
  const uint8_t vnr_req[] = {CONFIGURATION_SERVICER_RESID,
                             CONFIGURATION_SERVICER_RESID_VNR_VALUE | CONFIGURATION_COMMAND_READ_BIT, 2};
  uint8_t vnr_resp[2];

  auto error_code = this->write(vnr_req, sizeof(vnr_req));
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Request status failed");
    return 0;
  }
  error_code = this->read(vnr_resp, sizeof(vnr_resp));
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Failed to read VNR");
    return 0;
  }
  return vnr_resp[1];
}

void RespeakerLite::start_dfu_update() {
  if (this->firmware_bin_ == nullptr || !this->firmware_bin_length_) {
    ESP_LOGE(TAG, "Firmware invalid");
    return;
  }

  ESP_LOGI(TAG, "Starting update from %u.%u.%u...", this->firmware_version_major_, this->firmware_version_minor_,
           this->firmware_version_patch_);
#ifdef USE_RESPEAKER_LITE_STATE_CALLBACK
  this->state_callback_.call(DFU_START, 0, UPDATE_OK);
#endif

  if (!this->dfu_set_alternate_()) {
    ESP_LOGE(TAG, "Set alternate request failed");
    this->dfu_update_status_ = UPDATE_COMMUNICATION_ERROR;
    return;
  }

  this->bytes_written_ = 0;
  this->last_progress_ = 0;
  this->last_ready_ = millis();
  this->update_start_time_ = millis();
  this->dfu_update_status_ = this->dfu_update_send_block_();
}

RespeakerLiteUpdaterStatus RespeakerLite::dfu_update_send_block_() {
  i2c::ErrorCode error_code = i2c::NO_ERROR;
  uint8_t dfu_dnload_req[MAX_XFER + 6] = {240, 1, 130,  // resid, cmd_id, payload length,
                                          0, 0};        // additional payload length (set below)
                                                        // followed by payload data with null terminator
  if (millis() > this->last_ready_ + DFU_TIMEOUT_MS) {
    ESP_LOGE(TAG, "DFU timed out");
    return UPDATE_TIMEOUT;
  }

  if (this->bytes_written_ < this->firmware_bin_length_) {
    if (!this->dfu_check_if_ready_()) {
      return UPDATE_IN_PROGRESS;
    }

    // read a maximum of MAX_XFER bytes into buffer (real read size is returned)
    auto bufsize = this->load_buf_(&dfu_dnload_req[5], MAX_XFER, this->bytes_written_);
    ESP_LOGVV(TAG, "size = %u, bytes written = %u, bufsize = %u", this->firmware_bin_length_, this->bytes_written_,
              bufsize);

    if (bufsize > 0 && bufsize <= MAX_XFER) {
      // write bytes to XMOS
      dfu_dnload_req[3] = (uint8_t) bufsize;
      error_code = this->write(dfu_dnload_req, sizeof(dfu_dnload_req) - 1);
      if (error_code != i2c::ERROR_OK) {
        ESP_LOGE(TAG, "DFU download request failed");
        return UPDATE_COMMUNICATION_ERROR;
      }
      this->bytes_written_ += bufsize;
    }

    uint32_t now = millis();
    if ((now - this->last_progress_ > 1000) or (this->bytes_written_ == this->firmware_bin_length_)) {
      this->last_progress_ = now;
      float percentage = this->bytes_written_ * 100.0f / this->firmware_bin_length_;
      ESP_LOGD(TAG, "Progress: %0.1f%%", percentage);
#ifdef USE_RESPEAKER_LITE_STATE_CALLBACK
      this->state_callback_.call(DFU_IN_PROGRESS, percentage, UPDATE_IN_PROGRESS);
#endif
    }
    return UPDATE_IN_PROGRESS;
  } else {  // writing the main payload is done; work out what to do next
    switch (this->dfu_update_status_) {
      case UPDATE_IN_PROGRESS:
        if (!this->dfu_check_if_ready_()) {
          return UPDATE_IN_PROGRESS;
        }
        memset(&dfu_dnload_req[3], 0, MAX_XFER + 2);
        // send empty download request to conclude DFU download
        error_code = this->write(dfu_dnload_req, sizeof(dfu_dnload_req) - 1);
        if (error_code != i2c::ERROR_OK) {
          ESP_LOGE(TAG, "Final DFU download request failed");
          return UPDATE_COMMUNICATION_ERROR;
        }
        return UPDATE_REBOOT_PENDING;

      case UPDATE_REBOOT_PENDING:
        if (!this->dfu_check_if_ready_()) {
          return UPDATE_REBOOT_PENDING;
        }
        ESP_LOGI(TAG, "Done in %.0f seconds -- rebooting XMOS SoC...",
                 float(millis() - this->update_start_time_) / 1000);
        if (!this->dfu_reboot_()) {
          return UPDATE_COMMUNICATION_ERROR;
        }
        this->last_progress_ = millis();
        return UPDATE_VERIFY_NEW_VERSION;

      case UPDATE_VERIFY_NEW_VERSION:
        if (millis() > this->last_progress_ + 200) {
          this->last_progress_ = millis();
          if (!this->dfu_get_version_()) {
            return UPDATE_VERIFY_NEW_VERSION;
          }
        } else {
          return UPDATE_VERIFY_NEW_VERSION;
        }
        if (!this->versions_match_()) {
          ESP_LOGE(TAG, "Update failed");
          return UPDATE_FAILED;
        }
        ESP_LOGI(TAG, "Update complete");
#ifdef USE_RESPEAKER_LITE_STATE_CALLBACK
        this->state_callback_.call(DFU_COMPLETE, 100.0f, UPDATE_OK);
#endif
        return UPDATE_OK;

      default:
        ESP_LOGW(TAG, "Unknown state");
        return UPDATE_BAD_STATE;
    }
  }
  return UPDATE_BAD_STATE;
}

uint32_t RespeakerLite::load_buf_(uint8_t *buf, const uint8_t max_len, const uint32_t offset) {
  if (offset > this->firmware_bin_length_) {
    ESP_LOGE(TAG, "Invalid offset");
    return 0;
  }

  uint32_t buf_len = this->firmware_bin_length_ - offset;
  if (buf_len > max_len) {
    buf_len = max_len;
  }

  for (uint8_t i = 0; i < max_len; i++) {
    buf[i] = this->firmware_bin_[offset + i];
  }
  return buf_len;
}

bool RespeakerLite::version_read_() {
  return this->firmware_version_major_ || this->firmware_version_minor_ || this->firmware_version_patch_;
}

bool RespeakerLite::versions_match_() {
  return this->firmware_bin_version_major_ == this->firmware_version_major_ &&
         this->firmware_bin_version_minor_ == this->firmware_version_minor_ &&
         this->firmware_bin_version_patch_ == this->firmware_version_patch_;
}

bool RespeakerLite::dfu_get_status_() {
  const uint8_t status_req[] = {DFU_CONTROLLER_SERVICER_RESID,
                                DFU_CONTROLLER_SERVICER_RESID_DFU_GETSTATUS | DFU_COMMAND_READ_BIT, 6};
  uint8_t status_resp[6];

  auto error_code = this->write(status_req, sizeof(status_req));
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Request status failed");
    return false;
  }

  error_code = this->read(status_resp, sizeof(status_resp));
  if (error_code != i2c::ERROR_OK || status_resp[0] != CTRL_DONE) {
    ESP_LOGE(TAG, "Read status failed");
    return false;
  }
  this->status_last_read_ms_ = millis();
  this->dfu_status_next_req_delay_ = encode_uint24(status_resp[4], status_resp[3], status_resp[2]);
  this->dfu_state_ = status_resp[5];
  this->dfu_status_ = status_resp[1];
  ESP_LOGVV(TAG, "status_resp: %u %u - %ums", status_resp[1], status_resp[5], this->dfu_status_next_req_delay_);
  return true;
}

bool RespeakerLite::dfu_get_version_() {
  const uint8_t version_req[] = {DFU_CONTROLLER_SERVICER_RESID,
                                 DFU_CONTROLLER_SERVICER_RESID_DFU_GETVERSION | DFU_COMMAND_READ_BIT, 4};
  uint8_t version_resp[4];

  auto error_code = this->write(version_req, sizeof(version_req));
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Request version failed");
    return false;
  }

  error_code = this->read(version_resp, sizeof(version_resp));
  if (error_code != i2c::ERROR_OK || version_resp[0] != CTRL_DONE) {
    ESP_LOGW(TAG, "Read version failed");
    return false;
  }

  std::string version = str_sprintf("%u.%u.%u", version_resp[1], version_resp[2], version_resp[3]);
  ESP_LOGI(TAG, "DFU version: %s", version.c_str());
  this->firmware_version_major_ = version_resp[1];
  this->firmware_version_minor_ = version_resp[2];
  this->firmware_version_patch_ = version_resp[3];
  this->firmware_version_->publish_state(version);

  return true;
}

bool RespeakerLite::dfu_reboot_() {
  const uint8_t reboot_req[] = {DFU_CONTROLLER_SERVICER_RESID, DFU_CONTROLLER_SERVICER_RESID_DFU_REBOOT, 1};

  auto error_code = this->write(reboot_req, 4);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Reboot request failed");
    return false;
  }
  return true;
}

bool RespeakerLite::dfu_set_alternate_() {
  const uint8_t setalternate_req[] = {DFU_CONTROLLER_SERVICER_RESID, DFU_CONTROLLER_SERVICER_RESID_DFU_SETALTERNATE, 1,
                                      DFU_INT_ALTERNATE_UPGRADE};  // resid, cmd_id, payload length, payload data

  auto error_code = this->write(setalternate_req, sizeof(setalternate_req));
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "SetAlternate request failed");
    return false;
  }
  return true;
}

bool RespeakerLite::dfu_check_if_ready_() {
  if (millis() >= this->status_last_read_ms_ + this->dfu_status_next_req_delay_) {
    if (!this->dfu_get_status_()) {
      return false;
    }
    ESP_LOGVV(TAG, "DFU state: %u, status: %u, delay: %" PRIu32, this->dfu_state_, this->dfu_status_,
              this->dfu_status_next_req_delay_);

    if ((this->dfu_state_ == DFU_INT_DFU_IDLE) || (this->dfu_state_ == DFU_INT_DFU_DNLOAD_IDLE) ||
        (this->dfu_state_ == DFU_INT_DFU_MANIFEST_WAIT_RESET)) {
      this->last_ready_ = millis();
      return true;
    }
  }
  return false;
}

void RespeakerLite::get_mic_mute_state_() {
  uint8_t mute_req[3] = {CONFIGURATION_SERVICER_RESID, 0x81, 1};
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
  uint8_t mute_req[4] = {CONFIGURATION_SERVICER_RESID, 0x10, 1, 0};

  auto error_code = this->write(mute_req, sizeof(mute_req));
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Mute speaker failed");
  }
}

void RespeakerLite::unmute_speaker() {
  uint8_t unmute_req[4] = {CONFIGURATION_SERVICER_RESID, 0x10, 1, 1};

  auto error_code = this->write(unmute_req, sizeof(unmute_req));
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Unmute speaker failed");
  }
}

}  // namespace respeaker_lite
}  // namespace esphome