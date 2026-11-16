#include "aic3104.h"

#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace aic3104 {

static const char *const TAG = "aic3104";

#define ERROR_CHECK(err, msg) \
  if (!(err)) { \
    ESP_LOGE(TAG, msg); \
    this->mark_failed(); \
    return; \
  }

void AIC3104::setup() {
  // do nothing
}

void AIC3104::dump_config() {
  ESP_LOGCONFIG(TAG, "AIC3104:");
  LOG_I2C_DEVICE(this);

  if (this->is_failed()) {
    ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
  }
}

bool AIC3104::set_mute_off() {
  this->is_muted_ = false;
  return this->write_mute_();
}

bool AIC3104::set_mute_on() {
  this->is_muted_ = true;
  return this->write_mute_();
}

bool AIC3104::set_volume(float volume) {
  this->volume_ = clamp<float>(volume, 0.0, 1.0);
  ESP_LOGD(TAG, "AIC3104 set_volume called: %.2f", this->volume_);
  bool result = this->write_volume_();
  ESP_LOGD(TAG, "AIC3104 write_volume result: %s", result ? "SUCCESS" : "FAILED");
  return result;
}

bool AIC3104::is_muted() { return this->is_muted_; }

float AIC3104::volume() { return this->volume_; }

bool AIC3104::write_mute_() {
  // XVF3800/AIC3104 mute control - setting volume to maximum attenuation
  uint8_t mute_value = this->is_muted_ ? 0x80 : ((1.0f - this->volume_) * 0x80);
  
  if (!this->write_byte(AIC3104_PAGE_CTRL, 0x00) || 
      !this->write_byte(AIC3104_LEFT_DAC_VOLUME, mute_value) ||
      !this->write_byte(AIC3104_RIGHT_DAC_VOLUME, mute_value)) {
    ESP_LOGE(TAG, "Writing mute failed");
    return false;
  }
  
  ESP_LOGVV(TAG, "Mute %s (volume=0x%.2x)", this->is_muted_ ? "ON" : "OFF", mute_value);
  return true;
}

bool AIC3104::write_volume_() {
  ESP_LOGD(TAG, "write_volume_() called - volume: %.2f", this->volume_);
  
  if (!this->write_byte(AIC3104_PAGE_CTRL, 0x00)) {
    ESP_LOGE(TAG, "Failed to set page 0");
    return false;
  }
  
  // Map volume 0.0-1.0 to DAC range 0x80-0x00 (inverted)
  // 0x00 = 0dB (loudest), 0x7F = -63.5dB (quietest), 0x80 = mute
  uint8_t dac_val = (uint8_t)((1.0f - this->volume_) * 0x80);
  dac_val = clamp<uint8_t>(dac_val, 0x00, 0x80);
  
  ESP_LOGD(TAG, "Writing DAC volume: 0x%.2x (%.1fdB attenuation) to registers 0x2B/0x2C", 
           dac_val, -(float)dac_val);
  
  if (!this->write_byte(AIC3104_LEFT_DAC_VOLUME, dac_val) ||
      !this->write_byte(AIC3104_RIGHT_DAC_VOLUME, dac_val)) {
    ESP_LOGE(TAG, "Writing DAC volume failed");
    return false;
  }
  
  ESP_LOGD(TAG, "Volume %.1f%% → DAC: 0x%.2x (%.1fdB attenuation) - SUCCESS", 
           this->volume_ * 100.0f, dac_val, -(float)dac_val);
  
  return true;
}

}  // namespace aic3104
}  // namespace esphome
