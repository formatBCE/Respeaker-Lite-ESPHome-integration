#pragma once

#include <utility>
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"

namespace esphome {
namespace respeakerlite {

class RespeakerLite : public i2c::I2CDevice, public Component {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override;

  void set_mute_state(binary_sensor::BinarySensor* mute_state) {
    this->mute_state_ = mute_state;
  }

 protected:
  binary_sensor::BinarySensor* mute_state_{nullptr};
};

}  // namespace respeakerlite
}  // namespace esphome