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

  this->source_->add_data_callback([this](const microphone::AudioCapture &capture) { this->process_audio_(capture); });

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

void DecimatorMicrophone::process_audio_(const microphone::AudioCapture &capture) {
  const size_t frame_bytes = capture.stream_info.frames_to_bytes(1);
  const size_t frames = capture.data.size() / frame_bytes;
  const uint8_t first_frame = this->phase_;

  const size_t kept_frames =
      frames > this->phase_ ? (frames - this->phase_ + DECIMATION_FACTOR - 1) / DECIMATION_FACTOR : 0;
  this->output_buffer_.resize(kept_frames * frame_bytes);
  for (size_t i = 0; i < kept_frames; ++i) {
    std::memcpy(this->output_buffer_.data() + i * frame_bytes,
                capture.data.data() + (this->phase_ + i * DECIMATION_FACTOR) * frame_bytes, frame_bytes);
  }
  this->phase_ = (this->phase_ + DECIMATION_FACTOR - frames % DECIMATION_FACTOR) % DECIMATION_FACTOR;

  if (kept_frames > 0) {
    // The output starts at the first kept frame, not at the first frame of the chunk; 0 means no timestamp
    int64_t time_us = 0;
    if (capture.capture_time_us != 0) {
      time_us = capture.capture_time_us + capture.stream_info.frames_to_microseconds(first_frame);
    }
    this->publish_audio_(this->output_buffer_, this->audio_stream_info_, time_us);
  }
}

}  // namespace decimator
}  // namespace esphome
