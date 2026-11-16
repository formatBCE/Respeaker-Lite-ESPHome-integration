#pragma once
#include "respeaker_xvf3800.h"

#include "esphome/core/automation.h"

namespace esphome {
namespace respeaker_xvf3800 {

template<typename... Ts> class RespeakerXVF3800FlashAction : public Action<Ts...> {
 public:
  RespeakerXVF3800FlashAction(RespeakerXVF3800 *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->start_dfu_update(); }

 protected:
  RespeakerXVF3800 *parent_;
};
#ifdef USE_RESPEAKER_XVF3800_STATE_CALLBACK
class DFUStartTrigger : public Trigger<> {
 public:
  explicit DFUStartTrigger(RespeakerXVF3800 *parent) {
    parent->add_on_state_callback(
        [this, parent](DFUAutomationState state, float progress, RespeakerXVF3800UpdaterStatus error) {
          if (state == DFU_START && !parent->is_failed()) {
            trigger();
          }
        });
  }
};

class DFUProgressTrigger : public Trigger<float> {
 public:
  explicit DFUProgressTrigger(RespeakerXVF3800 *parent) {
    parent->add_on_state_callback(
        [this, parent](DFUAutomationState state, float progress, RespeakerXVF3800UpdaterStatus error) {
          if (state == DFU_IN_PROGRESS && !parent->is_failed()) {
            trigger(progress);
          }
        });
  }
};

class DFUEndTrigger : public Trigger<> {
 public:
  explicit DFUEndTrigger(RespeakerXVF3800 *parent) {
    parent->add_on_state_callback(
        [this, parent](DFUAutomationState state, float progress, RespeakerXVF3800UpdaterStatus error) {
          if (state == DFU_COMPLETE && !parent->is_failed()) {
            trigger();
          }
        });
  }
};

class DFUErrorTrigger : public Trigger<uint8_t> {
 public:
  explicit DFUErrorTrigger(RespeakerXVF3800 *parent) {
    parent->add_on_state_callback(
        [this, parent](DFUAutomationState state, float progress, RespeakerXVF3800UpdaterStatus error) {
          if (state == DFU_ERROR && !parent->is_failed()) {
            trigger(error);
          }
        });
  }
};
#endif
}  // namespace respeaker_xvf3800
}  // namespace esphome
