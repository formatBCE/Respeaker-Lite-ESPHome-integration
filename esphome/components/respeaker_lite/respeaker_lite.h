#pragma once

//#include <utility>
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace respeaker_lite {

class RespeakerLite : public i2c::I2CDevice, public Component {
 public:
  void setup() override;
  void loop() override;

  void mute_speaker();
  void unmute_speaker();

  void set_mute_state(binary_sensor::BinarySensor* mute_state) {
    this->mute_state_ = mute_state;
  }

  void set_firmware_version(text_sensor::TextSensor* firmware_version) {
    this->firmware_version_ = firmware_version;
  }

  void set_reset_pin(GPIOPin *reset_pin) { 
    this->reset_pin_ = reset_pin; 
  }

 protected:
  GPIOPin *reset_pin_;
  binary_sensor::BinarySensor *mute_state_{nullptr};
  text_sensor::TextSensor *firmware_version_{nullptr};

  bool get_firmware_version_();
  void get_mic_mute_state_();
};

}  // namespace respeaker_lite
}  // namespace esphome