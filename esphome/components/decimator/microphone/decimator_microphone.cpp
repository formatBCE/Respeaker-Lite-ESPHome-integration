#include "decimator_microphone.h"

#include "esphome/core/log.h"

#include <cstring>

namespace esphome {
namespace decimator {

static const char *const TAG = "decimator.microphone";

static constexpr uint8_t DECIMATION_FACTOR = 3;

void DecimatorMicrophone::setup() {
  const audio::AudioStreamInfo source_info = this->source_->get_audio_stream_info();
  this->audio_stream_info_ = audio::AudioStreamInfo(source_info.get_bits_per_sample(), source_info.get_channels(),
                                                    source_info.get_sample_rate() / DECIMATION_FACTOR);

  this->source_->add_data_callback([this](const std::vector<uint8_t> &data) { this->process_audio_(data); });

  this->disable_loop();
}

void DecimatorMicrophone::dump_config() {
  ESP_LOGCONFIG(TAG,
                "Decimator Microphone:\n"
                "  Sample Rate: %" PRIu32 " Hz (1 of %u frames)",
                this->audio_stream_info_.get_sample_rate(), DECIMATION_FACTOR);
}

void DecimatorMicrophone::start() {
  if (this->is_failed() || this->active_listeners_ == UINT8_MAX)
    return;
  ++this->active_listeners_;
  this->enable_loop();
}

void DecimatorMicrophone::stop() {
  if (this->active_listeners_ == 0)
    return;
  --this->active_listeners_;
  this->enable_loop();
}

void DecimatorMicrophone::loop() {
  if (this->active_listeners_ == 0) {
    if (this->state_ != microphone::STATE_STOPPED) {
      this->source_->stop();
      this->state_ = microphone::STATE_STOPPED;
    }
    this->disable_loop();
    return;
  }

  switch (this->state_) {
    case microphone::STATE_STOPPED:
      this->source_->start();
      this->state_ = microphone::STATE_STARTING;
      break;
    case microphone::STATE_STARTING:
      if (this->source_->is_running()) {
        this->state_ = microphone::STATE_RUNNING;
      }
      break;
    case microphone::STATE_RUNNING:
      // Follow the source if it restarts, e.g. after a driver error
      if (!this->source_->is_running()) {
        this->state_ = microphone::STATE_STARTING;
      }
      break;
    case microphone::STATE_STOPPING:
      break;
  }
}

void DecimatorMicrophone::process_audio_(const std::vector<uint8_t> &data) {
  const size_t frame_bytes = this->audio_stream_info_.frames_to_bytes(1);
  const size_t frames = data.size() / frame_bytes;

  const size_t kept_frames =
      frames > this->phase_ ? (frames - this->phase_ + DECIMATION_FACTOR - 1) / DECIMATION_FACTOR : 0;
  this->output_buffer_.resize(kept_frames * frame_bytes);
  for (size_t i = 0; i < kept_frames; ++i) {
    std::memcpy(this->output_buffer_.data() + i * frame_bytes,
                data.data() + (this->phase_ + i * DECIMATION_FACTOR) * frame_bytes, frame_bytes);
  }
  this->phase_ = (this->phase_ + DECIMATION_FACTOR - frames % DECIMATION_FACTOR) % DECIMATION_FACTOR;

  if (kept_frames > 0) {
    this->data_callbacks_.call(this->output_buffer_);
  }
}

}  // namespace decimator
}  // namespace esphome
