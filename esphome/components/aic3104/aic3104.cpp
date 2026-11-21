#include "aic3104.h"

#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace aic3104 {

static const char *const TAG = "aic3104";

// The XVF3800 configures the codec about 5 s after it boots, and it resets the
// codec again when it reboots on its own (after a DSP firmware update, for
// example). So the output gain waits for the first delay and is checked again
// every minute.
static const uint32_t AIC3104_GAIN_SETTLE_MS = 5000;
static const uint32_t AIC3104_GAIN_CHECK_INTERVAL_MS = 60000;

#define ERROR_CHECK(err, msg) \
  if (!(err)) { \
    ESP_LOGE(TAG, msg); \
    this->mark_failed(); \
    return; \
  }

void AIC3104::setup() {
  // do nothing
}

void AIC3104::loop() {
  if (!this->output_gain_enabled_) {
    // Nothing to do unless the optional output gain is configured. The number
    // may call set_output_gain() before or after this setup(), so the loop
    // stops itself here instead of in setup().
    this->disable_loop();
    return;
  }
  const uint32_t now = millis();
  if ((int32_t) (now - this->next_gain_check_) < 0) {
    return;
  }
  this->next_gain_check_ = now + AIC3104_GAIN_CHECK_INTERVAL_MS;
  this->apply_output_gain_();
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

void AIC3104::set_output_gain(uint8_t gain_db) {
  this->output_gain_ = std::min<uint8_t>(gain_db, 9);
  this->output_gain_enabled_ = true;
  // Apply on the next loop, but not before the XVF3800 has set up the codec.
  const uint32_t now = millis();
  this->next_gain_check_ = now < AIC3104_GAIN_SETTLE_MS ? AIC3104_GAIN_SETTLE_MS : now;
  this->enable_loop();
}

void AIC3104::apply_output_gain_() {
  static const uint8_t level_regs[] = {AIC3104_HPLOUT_LEVEL, AIC3104_HPLCOM_LEVEL, AIC3104_HPROUT_LEVEL,
                                       AIC3104_HPRCOM_LEVEL, AIC3104_LEFT_LOP_LEVEL, AIC3104_RIGHT_LOP_LEVEL};

  if (!this->write_byte(AIC3104_PAGE_CTRL, 0x00)) {
    ESP_LOGW(TAG, "Failed to set page 0");
    return;
  }
  // D7-D4 of each output level register hold the gain in dB. D3-D0 hold the
  // mute, drive and power bits and a read-only status bit, so keep them.
  // The write is skipped when the value is already right, so a normal
  // check only reads.
  for (uint8_t reg : level_regs) {
    uint8_t old_value;
    if (!this->read_byte(reg, &old_value)) {
      ESP_LOGW(TAG, "Reading output level 0x%02X failed", reg);
      continue;
    }
    uint8_t new_value = (old_value & 0x0F) | (this->output_gain_ << 4);
    if (new_value == old_value) {
      continue;
    }
    if (!this->write_byte(reg, new_value)) {
      ESP_LOGW(TAG, "Writing output level 0x%02X failed", reg);
      continue;
    }
    ESP_LOGI(TAG, "Output level 0x%02X: 0x%02X -> 0x%02X (+%u dB)", reg, old_value, new_value, this->output_gain_);
  }
}

void AIC3104OutputGain::setup() {
  float value = 0;  // 0 dB is the default, the same as without this entity
  this->pref_ = this->make_entity_preference<float>();
  if (!this->pref_.load(&value)) {
    value = 0;
  }
  this->publish_state(value);
  this->parent_->set_output_gain((uint8_t) value);
}

void AIC3104OutputGain::control(float value) {
  this->publish_state(value);
  this->pref_.save(&value);
  this->parent_->set_output_gain((uint8_t) value);
}

void AIC3104OutputGain::dump_config() { LOG_NUMBER("", "AIC3104 Output Gain", this); }

}  // namespace aic3104
}  // namespace esphome
