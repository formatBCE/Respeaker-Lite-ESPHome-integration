#pragma once
#include "respeaker_lite.h"

#include "esphome/core/automation.h"

namespace esphome {
namespace respeaker_lite {

template<typename... Ts> class RespeakerLiteFlashAction : public Action<Ts...> {
 public:
  RespeakerLiteFlashAction(RespeakerLite *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->start_dfu_update(); }

 protected:
  RespeakerLite *parent_;
};
#ifdef USE_RESPEAKER_LITE_STATE_CALLBACK
class DFUStartTrigger : public Trigger<> {
 public:
  explicit DFUStartTrigger(RespeakerLite *parent) {
    parent->add_on_state_callback(
        [this, parent](DFUAutomationState state, float progress, RespeakerLiteUpdaterStatus error) {
          if (state == DFU_START && !parent->is_failed()) {
            trigger();
          }
        });
  }
};

class DFUProgressTrigger : public Trigger<float> {
 public:
  explicit DFUProgressTrigger(RespeakerLite *parent) {
    parent->add_on_state_callback(
        [this, parent](DFUAutomationState state, float progress, RespeakerLiteUpdaterStatus error) {
          if (state == DFU_IN_PROGRESS && !parent->is_failed()) {
            trigger(progress);
          }
        });
  }
};

class DFUEndTrigger : public Trigger<> {
 public:
  explicit DFUEndTrigger(RespeakerLite *parent) {
    parent->add_on_state_callback(
        [this, parent](DFUAutomationState state, float progress, RespeakerLiteUpdaterStatus error) {
          if (state == DFU_COMPLETE && !parent->is_failed()) {
            trigger();
          }
        });
  }
};

class DFUErrorTrigger : public Trigger<uint8_t> {
 public:
  explicit DFUErrorTrigger(RespeakerLite *parent) {
    parent->add_on_state_callback(
        [this, parent](DFUAutomationState state, float progress, RespeakerLiteUpdaterStatus error) {
          if (state == DFU_ERROR && !parent->is_failed()) {
            trigger(error);
          }
        });
  }
};
#endif
}  // namespace respeaker_lite
}  // namespace esphome