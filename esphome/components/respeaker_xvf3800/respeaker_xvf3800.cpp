#include "respeaker_xvf3800.h"

#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace esphome {
namespace respeaker_xvf3800 {

static const char *const TAG = "respeaker_xvf3800";

void RespeakerXVF3800::setup() {
  ESP_LOGCONFIG(TAG, "Setting up RespeakerXVF3800...");

  uint8_t test_data;
  i2c::ErrorCode err = this->read(&test_data, 1);
  if (err != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Could not communicate with XVF3800 at configured I2C address");
    this->mark_failed();
    return;
  }

  // Wait for XMOS to boot...
  this->set_timeout(3000, [this]() {
    if (!this->dfu_get_version_()) {
      ESP_LOGE(TAG, "Communication with Respeaker XVF3800 failed");
      this->mark_failed();
    } else if (!this->versions_match_() && this->firmware_bin_is_valid_()) {
      ESP_LOGW(TAG, "Expected XMOS version: %u.%u.%u; found: %u.%u.%u. Updating...", this->firmware_bin_version_major_,
               this->firmware_bin_version_minor_, this->firmware_bin_version_patch_, this->firmware_version_major_,
               this->firmware_version_minor_, this->firmware_version_patch_);
      this->start_dfu_update();
    } else {
      this->apply_audio_tuning_();
    }
  });
}

void RespeakerXVF3800::dump_config() {
  ESP_LOGCONFIG(TAG, "Respeaker XVF3800:");
  LOG_I2C_DEVICE(this);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  if (this->firmware_version_major_ || this->firmware_version_minor_ || this->firmware_version_patch_) {
    ESP_LOGCONFIG(TAG, "  XMOS firmware version: %u.%u.%u", this->firmware_version_major_,
                  this->firmware_version_minor_, this->firmware_version_patch_);
  }
}

void RespeakerXVF3800::loop() {
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
#ifdef USE_RESPEAKER_XVF3800_STATE_CALLBACK
      this->state_callback_.call(DFU_ERROR, this->bytes_written_ * 100.0f / this->firmware_bin_length_,
                                 this->dfu_update_status_);
#endif
      this->mark_failed();
      break;

    default:
      // Normal operation - no additional logic needed here
      // Mute state is handled by the MuteSwitch component using GPO methods
      break;
  }
}

uint8_t RespeakerXVF3800::read_vnr() {
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

void RespeakerXVF3800::start_dfu_update() {
  if (this->firmware_bin_ == nullptr || !this->firmware_bin_length_) {
    ESP_LOGE(TAG, "Firmware invalid");
    return;
  }

  ESP_LOGI(TAG, "Starting update from %u.%u.%u...", this->firmware_version_major_, this->firmware_version_minor_,
           this->firmware_version_patch_);
#ifdef USE_RESPEAKER_XVF3800_STATE_CALLBACK
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

RespeakerXVF3800UpdaterStatus RespeakerXVF3800::dfu_update_send_block_() {
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
#ifdef USE_RESPEAKER_XVF3800_STATE_CALLBACK
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
        if (millis() > this->last_progress_ + 500) {
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
        // The XMOS rebooted with default settings, so re-apply the tuning.
        this->apply_audio_tuning_();
#ifdef USE_RESPEAKER_XVF3800_STATE_CALLBACK
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

uint32_t RespeakerXVF3800::load_buf_(uint8_t *buf, const uint8_t max_len, const uint32_t offset) {
  if (offset > this->firmware_bin_length_) {
    ESP_LOGE(TAG, "Invalid offset");
    return 0;
  }

  uint32_t buf_len = this->firmware_bin_length_ - offset;
  if (buf_len > max_len) {
    buf_len = max_len;
  }

  for (uint32_t i = 0; i < buf_len; i++) {
    buf[i] = this->firmware_bin_[offset + i];
  }
  return buf_len;
}

bool RespeakerXVF3800::version_read_() {
  return this->firmware_version_major_ || this->firmware_version_minor_ || this->firmware_version_patch_;
}

bool RespeakerXVF3800::versions_match_() {
  return this->firmware_bin_version_major_ == this->firmware_version_major_ &&
         this->firmware_bin_version_minor_ == this->firmware_version_minor_ &&
         this->firmware_bin_version_patch_ == this->firmware_version_patch_;
}

bool RespeakerXVF3800::chip_version_at_least_(uint8_t major, uint8_t minor, uint8_t patch) {
  if (this->firmware_version_major_ != major)
    return this->firmware_version_major_ > major;
  if (this->firmware_version_minor_ != minor)
    return this->firmware_version_minor_ > minor;
  return this->firmware_version_patch_ >= patch;
}

bool RespeakerXVF3800::dfu_get_status_() {
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

bool RespeakerXVF3800::dfu_get_version_() {
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

  // Three uint8_t values need at most "255.255.255" plus the null terminator.
  char version[12];
  std::snprintf(version, sizeof(version), "%u.%u.%u", static_cast<unsigned>(version_resp[1]),
                static_cast<unsigned>(version_resp[2]), static_cast<unsigned>(version_resp[3]));
  ESP_LOGI(TAG, "DFU version: %s", version);
  this->firmware_version_major_ = version_resp[1];
  this->firmware_version_minor_ = version_resp[2];
  this->firmware_version_patch_ = version_resp[3];
  // The LED ring command depends on the version, so send the next frame again.
  this->led_frame_valid_ = false;
  if (this->firmware_version_ != nullptr) {
    this->firmware_version_->publish_state(version);
  }

  return true;
}

bool RespeakerXVF3800::dfu_reboot_() {
  const uint8_t reboot_req[] = {DFU_CONTROLLER_SERVICER_RESID, DFU_CONTROLLER_SERVICER_RESID_DFU_REBOOT, 1, 0};

  auto error_code = this->write(reboot_req, sizeof(reboot_req));
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Reboot request failed");
    return false;
  }
  return true;
}

bool RespeakerXVF3800::dfu_set_alternate_() {
  const uint8_t setalternate_req[] = {DFU_CONTROLLER_SERVICER_RESID, DFU_CONTROLLER_SERVICER_RESID_DFU_SETALTERNATE, 1,
                                      DFU_INT_ALTERNATE_UPGRADE};  // resid, cmd_id, payload length, payload data

  auto error_code = this->write(setalternate_req, sizeof(setalternate_req));
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "SetAlternate request failed");
    return false;
  }
  return true;
}

bool RespeakerXVF3800::dfu_check_if_ready_() {
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

bool RespeakerXVF3800::read_gpo_values(uint8_t *buffer, uint8_t *status) {
  const uint8_t request[] = {GPO_SERVICER_RESID, 
                            GPO_SERVICER_RESID_GPO_READ_VALUES | 0x80, 
                            GPO_GPO_READ_NUM_BYTES + 1};

  uint8_t data[6] = {0};
  i2c::ErrorCode err = this->write_read(request, sizeof(request), data, sizeof(data));
  
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Failed to read GPO statuses, error=%d", (int)err);
    return false;
  }

  *status = data[0];
  for (uint8_t i = 0; i < GPO_GPO_READ_NUM_BYTES; i++) {
    buffer[i] = data[i + 1];
  }

  return true;
}

bool RespeakerXVF3800::read_mute_status() {
  uint8_t gpo_values[5] = {0};
  uint8_t status = 0xFF;
  
  if (this->read_gpo_values(gpo_values, &status)) {
    ESP_LOGD(TAG, "GPO Status: %02X, GPO data: %02X %02X %02X %02X %02X", 
             status, gpo_values[0], gpo_values[1], gpo_values[2], gpo_values[3], gpo_values[4]);
    
    bool gpio30 = (gpo_values[1] & 0x01) != 0;
    ESP_LOGD(TAG, "GPIO30 (mute): %s", gpio30 ? "MUTED" : "UNMUTED");
    return gpio30;
  }
  return false;
}

void RespeakerXVF3800::write_mute_status(bool value) {
  uint8_t payload[] = {GPO_SERVICER_RESID, GPO_SERVICER_RESID_GPO_WRITE_VALUE, 2, 30, (uint8_t)(value ? 1 : 0)};
  
  ESP_LOGD(TAG, "Writing mute status %s to GPIO 30: [0x%02X, 0x%02X, 0x%02X, %d, %d]", 
           value ? "MUTE" : "UNMUTE", payload[0], payload[1], payload[2], payload[3], payload[4]);
  
  i2c::ErrorCode err = this->write(payload, sizeof(payload));

  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Error writing mute status to GPIO 30. Error code: %d", (int)err);
  }
}

bool RespeakerXVF3800::read_mute_button(bool &pressed) {
  const uint8_t request[] = {IO_CONFIG_SERVICER_RESID, IO_CONFIG_GPI_VALUE_ALL_CMD | 0x80,
                             5};  // 4 bytes (uint32) + 1 status byte
  uint8_t response[5];

  // Single attempt, like the azimuth read: the next poll is the retry.
  if (this->write_read(request, sizeof(request), response, sizeof(response)) != i2c::ERROR_OK) {
    ESP_LOGV(TAG, "Failed to read GPI values");
    return false;
  }
  if (response[0] != CTRL_DONE) {
    ESP_LOGV(TAG, "GPI read returned status %u", response[0]);
    return false;
  }

  uint32_t levels;
  memcpy(&levels, &response[1], sizeof(levels));
  pressed = (levels & GPI_MUTE_BUTTON_BIT) == 0;  // the bit is low while pressed
  return true;
}

bool RespeakerXVF3800::read_azimuth_radians_(float &out_radians, uint8_t beam_index) {
  if (beam_index > 3) {
    ESP_LOGW(TAG, "read_azimuth_radians_: invalid beam index %u", beam_index);
    return false;
  }

  const uint8_t aec_req[] = {AEC_SERVICER_RESID,
                             AEC_AZIMUTH_VALUES_CMD | 0x80,
                             17};  // 16 bytes (4 floats) + 1 status byte

  uint8_t aec_resp[17];

  // Single attempt. The XMOS transport protocol can return CTRL_WAIT (1) when the
  // servicer is busy, and the host is expected to retry — but this runs in the
  // ESPHome main loop, so the 10 Hz beam poll is the retry mechanism. A blocking
  // retry loop here stalls every other component's loop().
  i2c::ErrorCode err = this->write_read(aec_req, sizeof(aec_req), aec_resp, sizeof(aec_resp));
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Failed to read AEC azimuth values, error=%d", (int)err);
    return false;
  }

  uint8_t status = aec_resp[0];
  if (status == CTRL_DONE) {
    // 4 floats follow at bytes [1..16]: beam 1, beam 2, free-running, auto-select.
    const uint8_t offset = 1 + beam_index * sizeof(float);
    float radians;
    memcpy(&radians, &aec_resp[offset], sizeof(float));
    ESP_LOGD(TAG, "AEC azimuth (beam %u, raw radians): %f", beam_index, radians);
    out_radians = radians;
    // Cache only the auto-select beam. That is the slot lock_beam() wants, and
    // the pinned slots would otherwise feed the lock back its own value.
    if (beam_index == 3) {
      this->last_azimuth_rad_ = radians;
      this->last_azimuth_ms_ = millis();
    }
    return true;
  }

  if (status != CTRL_WAIT && status != SERVICER_COMMAND_RETRY) {
    ESP_LOGW(TAG, "AEC azimuth read returned unexpected status 0x%02X", status);
    return false;
  }

  // Servicer busy. Normal during silence (no source to localize → no fresh
  // azimuth), hence DEBUG not WARN. The next poll picks it up.
  ESP_LOGD(TAG, "AEC azimuth read busy (status 0x%02X, no fresh data)", status);
  return false;
}

int RespeakerXVF3800::read_led_beam_direction() {
  float radians;
  // When locked, read beam 1 (the pinned fixed beam) straight from the chip;
  // otherwise read the auto-select beam (the adaptive default).
  const uint8_t beam_index = this->beam_locked_ ? 0 : 3;
  if (!this->read_azimuth_radians_(radians, beam_index)) {
    return -1;
  }

  float degrees = radians * 180.0f / M_PI;

  // Map degrees to LED index (0-11). Each LED covers 30 degrees.
  int led_index = (int)roundf(degrees / 30.0f);
  if (led_index < 0) {
    led_index += 12;
  }
  led_index = led_index % 12;

  ESP_LOGD(TAG, "AEC azimuth: %.1f degrees -> LED %d", degrees, led_index);

  return led_index;
}

void RespeakerXVF3800::lock_beam() {
  // Prefer the value the 10 Hz beam poll already has. It is at most one poll
  // old, it is the direction the wake word came from, and it costs no bus time
  // on the wake-word path. Fall back to one read if the poll is off or stale.
  float radians;
  if (this->last_azimuth_ms_ != 0 && (millis() - this->last_azimuth_ms_) <= AZIMUTH_MAX_AGE_MS) {
    radians = this->last_azimuth_rad_;
  } else if (!this->read_azimuth_radians_(radians)) {
    // Expected when the beam poll is disabled, or when the DSP has no source to
    // localize yet. Beam lock is an opt-in enhancement, so a miss is not an error.
    ESP_LOGD(TAG, "lock_beam: no fresh azimuth (beam poll disabled or DSP busy); not locking");
    return;
  }

  // AEC_FIXEDBEAMSAZIMUTH_VALUES is 2 floats (radians): fixed beam 1, fixed beam 2.
  // We point both at the same direction so whichever beam is gated picks up the source.
  uint8_t payload[2 * sizeof(float)];
  memcpy(&payload[0], &radians, sizeof(float));
  memcpy(&payload[sizeof(float)], &radians, sizeof(float));
  this->xmos_write_bytes(AEC_SERVICER_RESID, AEC_FIXEDBEAMS_AZIMUTH_CMD, payload, sizeof(payload));

  // AEC_FIXEDBEAMSONOFF is int32 (little-endian on XS3).
  uint8_t on[4] = {0x01, 0x00, 0x00, 0x00};
  this->xmos_write_bytes(AEC_SERVICER_RESID, AEC_FIXEDBEAMS_ONOFF_CMD, on, sizeof(on));

  this->beam_locked_ = true;

  ESP_LOGI(TAG, "Beam locked at %.3f rad (%.1f deg)", radians, radians * 180.0f / (float)M_PI);
}

void RespeakerXVF3800::unlock_beam() {
  uint8_t off[4] = {0x00, 0x00, 0x00, 0x00};
  this->xmos_write_bytes(AEC_SERVICER_RESID, AEC_FIXEDBEAMS_ONOFF_CMD, off, sizeof(off));
  this->beam_locked_ = false;
  ESP_LOGI(TAG, "Beam lock released");
}

float RespeakerXVF3800::get_azimuth_degrees() const {
  // While locked, the cache holds the locked direction: lock_beam() took it from
  // there, and the poll then reads the pinned slot, which is not cached.
  if (!this->beam_locked_ &&
      (this->last_azimuth_ms_ == 0 || (millis() - this->last_azimuth_ms_) > AZIMUTH_MAX_AGE_MS)) {
    return NAN;
  }
  const float degrees = fmodf(this->last_azimuth_rad_ * 180.0f / (float) M_PI, 360.0f);
  return degrees < 0.0f ? degrees + 360.0f : degrees;
}

void RespeakerXVF3800::xmos_write_bytes(uint8_t resid, uint8_t cmd, const uint8_t *value, uint8_t write_byte_num) {
  uint8_t payload[3 + 255];
  payload[0] = resid;
  payload[1] = cmd;
  payload[2] = write_byte_num;

  if (write_byte_num > 0 && value != nullptr) {
    memcpy(&payload[3], value, write_byte_num);
  }

  i2c::ErrorCode err = this->write(payload, 3 + write_byte_num);
  
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Error in xmos_write_bytes. resid=%d, cmd=%d, error=%d", resid, cmd, (int)err);
  }
}

void RespeakerXVF3800::set_led_ring(uint32_t *rgb_array) {
  // The YAML animation interval calls this 20 times a second whether the frame
  // changed or not. Skip the bus when it did not.
  if (this->led_frame_valid_ && memcmp(this->last_led_frame_, rgb_array, sizeof(this->last_led_frame_)) == 0) {
    return;
  }
  memcpy(this->last_led_frame_, rgb_array, sizeof(this->last_led_frame_));
  this->led_frame_valid_ = true;

  uint8_t payload[48];
  
  for (int i = 0; i < 12; i++) {
    uint32_t color = rgb_array[i];
    payload[i * 4 + 0] = (uint8_t)(color & 0xFF);
    payload[i * 4 + 1] = (uint8_t)((color >> 8) & 0xFF);
    payload[i * 4 + 2] = (uint8_t)((color >> 16) & 0xFF);
    payload[i * 4 + 3] = 0x00;
  }
  
  const uint8_t cmd = this->chip_version_at_least_(1, 0, 8) ? GPO_SERVICER_RESID_LED_RING_VALUE
                                                            : GPO_SERVICER_RESID_LED_RING_VALUE_PRE_1_0_8;
  this->xmos_write_bytes(GPO_SERVICER_RESID, cmd, payload, 48);
}

std::string RespeakerXVF3800::read_dfu_version() {
  uint8_t data[4] = {0};
  
  const uint8_t request[] = {0xF0, 0x58 | I2C_COMMAND_READ_BIT, 4};
  
  i2c::ErrorCode err = this->write(request, sizeof(request));
  if (err == i2c::ERROR_OK) {
    err = this->read(data, 4);
    if (err == i2c::ERROR_OK && data[0] == 0) {
      ESP_LOGI(TAG, "Version request successful: %u.%u.%u", data[1], data[2], data[3]);
      char version[12];  // "255.255.255" plus the null terminator
      std::snprintf(version, sizeof(version), "%u.%u.%u", static_cast<unsigned>(data[1]),
                    static_cast<unsigned>(data[2]), static_cast<unsigned>(data[3]));
      return version;
    }
  }
  return "Unknown";
}

void RespeakerXVF3800::set_asr_route(bool asr_route) {
  this->asr_route_ = asr_route;
  if (this->tuning_applied_)
    this->apply_audio_tuning_();
}

void RespeakerXVF3800::set_asr_out_gain(float gain) {
  this->asr_out_gain_ = gain;
  if (this->tuning_applied_)
    this->write_asr_out_gain_();
}

void RespeakerXVF3800::set_agc_max_gain(float gain) {
  this->agc_max_gain_ = gain;
  if (this->tuning_applied_)
    this->write_agc_max_gain_();
}

void RespeakerXVF3800::write_asr_out_gain_() {
  uint8_t gain[4];
  memcpy(gain, &this->asr_out_gain_, sizeof(gain));
  this->xmos_write_bytes(AEC_SERVICER_RESID, AEC_ASROUTGAIN_CMD, gain, sizeof(gain));
}

void RespeakerXVF3800::write_agc_max_gain_() {
  uint8_t gain[4];
  memcpy(gain, &this->agc_max_gain_, sizeof(gain));
  this->xmos_write_bytes(PP_SERVICER_RESID, PP_AGCMAXGAIN_CMD, gain, sizeof(gain));
}

void RespeakerXVF3800::apply_audio_tuning_() {
  // ASR route: both slots get the ASR output (7,3). Stock: comms (8,0) on STT.
  const uint8_t l_cat = this->asr_route_ ? 7 : 8;
  const uint8_t l_src = this->asr_route_ ? 3 : 0;
  const uint8_t l[2] = {l_cat, l_src};
  const uint8_t r[2] = {7, 3};

  // Category 7 is the ASR beam only while this is 1; otherwise (7,3) is mic 3's residual.
  const uint8_t asr_on[4] = {0x01, 0x00, 0x00, 0x00};  // int32 LE
  this->xmos_write_bytes(AEC_SERVICER_RESID, AEC_ASROUTONOFF_CMD, asr_on, sizeof(asr_on));

  this->xmos_write_bytes(AUDIO_MGR_SERVICER_RESID, AUDIO_MGR_OP_L_CMD, l, sizeof(l));
  this->xmos_write_bytes(AUDIO_MGR_SERVICER_RESID, AUDIO_MGR_OP_R_CMD, r, sizeof(r));

  this->write_asr_out_gain_();
  this->write_agc_max_gain_();
  this->tuning_applied_ = true;
  ESP_LOGI(TAG, "Audio tuning sent: route=%s, ASROUTGAIN=%.2f, AGCMAXGAIN=%.1f",
           this->asr_route_ ? "ASR" : "comms", this->asr_out_gain_, this->agc_max_gain_);
}

// =========================================================================
//   Child Component Implementations
// =========================================================================

// --- MuteSwitch Component ---
void MuteSwitch::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Mute Switch...");
}

void MuteSwitch::dump_config() {
  LOG_SWITCH("", "Respeaker Mute Switch", this);
}

void MuteSwitch::update() {
  if (this->parent_ == nullptr) {
    ESP_LOGW(TAG, "MuteSwitch parent not set");
    return;
  }

  bool mute_state = this->parent_->read_mute_status();
  
  if (this->state != mute_state) {
    this->publish_state(mute_state);
  }
}

void MuteSwitch::write_state(bool state) {
  ESP_LOGD(TAG, "MuteSwitch::write_state called with state: %s", state ? "ON" : "OFF");
  ESP_LOGD(TAG, "Parent pointer: %p", this->parent_);
  
  if (this->parent_ == nullptr) {
    ESP_LOGE(TAG, "MuteSwitch parent not set - cannot write mute status");
    return;
  }
  
  this->parent_->write_mute_status(state);
  this->publish_state(state);
}

// --- DFUVersionTextSensor Component ---
void DFUVersionTextSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up DFU Version Text Sensor...");
}

void DFUVersionTextSensor::dump_config() {
  LOG_TEXT_SENSOR("", "Respeaker DFU Version", this);
}

void DFUVersionTextSensor::update() {
  if (this->parent_ == nullptr) {
    ESP_LOGW(TAG, "DFUVersionTextSensor parent not set");
    return;
  }
  
  std::string version = this->parent_->read_dfu_version();
  if (this->get_raw_state() != version) {
    this->publish_state(version);
  }
}

// --- LEDBeamSensor Component ---
void LEDBeamSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up LED Beam Sensor...");
}

void LEDBeamSensor::dump_config() {
  LOG_SENSOR("", "Respeaker LED Beam Direction", this);
}

void LEDBeamSensor::update() {
  if (this->parent_ == nullptr) {
    ESP_LOGW(TAG, "LEDBeamSensor parent not set");
    return;
  }
  
  int led_index = this->parent_->read_led_beam_direction();
  if (led_index >= 0 && led_index <= 11) {
    if (!this->has_state() || this->get_raw_state() != led_index) {
      this->publish_state(led_index);
    }
  // } else {
  //   ESP_LOGW(TAG, "Invalid LED beam direction: %d", led_index);
  }
}

#ifdef USE_BINARY_SENSOR
// --- MuteButtonBinarySensor Component ---
void MuteButtonBinarySensor::dump_config() {
  LOG_BINARY_SENSOR("", "Respeaker Mute Button", this);
}

void MuteButtonBinarySensor::update() {
  if (this->parent_ == nullptr || this->parent_->is_failed()) {
    return;
  }

  bool pressed;
  if (this->parent_->read_mute_button(pressed)) {
    this->publish_state(pressed);
  }
}
#endif

}  // namespace respeaker_xvf3800
}  // namespace esphome
