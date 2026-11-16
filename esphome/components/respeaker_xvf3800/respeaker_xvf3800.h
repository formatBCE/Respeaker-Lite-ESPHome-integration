#pragma once

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"
#include <cstring>

namespace esphome {
namespace respeaker_xvf3800 {

// Forward-declare the main component class
class RespeakerXVF3800;

static const uint8_t REGISTER_CHANNEL_1_STAGE = 0x40;

// Configuration servicer resource IDs
static const uint8_t DFU_CONTROLLER_SERVICER_RESID = 240;
static const uint8_t CONFIGURATION_SERVICER_RESID = 241;
static const uint8_t CONFIGURATION_COMMAND_READ_BIT = 0x80;
static const uint8_t DFU_COMMAND_READ_BIT = 0x80;

static const uint16_t DFU_TIMEOUT_MS = 4000;
static const uint16_t MAX_XFER = 128;  // maximum number of bytes we can transfer per block

// Original XVF3800 constants
const uint8_t GPO_SERVICER_RESID = 20;
const uint8_t GPO_SERVICER_RESID_GPO_READ_VALUES = 0;
const uint8_t GPO_SERVICER_RESID_GPO_WRITE_VALUE = 1;
// LED_RING_COLOR is cmd 19 from firmware 1.0.8 on. Before 1.0.8 it was cmd 18,
// which 1.0.8 gave to DOA_VALUE. Seeed I2S firmware changelog, v1.0.8.
const uint8_t GPO_SERVICER_RESID_LED_RING_VALUE = 19;
const uint8_t GPO_SERVICER_RESID_LED_RING_VALUE_PRE_1_0_8 = 18;
const uint8_t GPO_GPO_READ_NUM_BYTES = 5;

// AEC Azimuth constants for LED beam sensor
const uint8_t AEC_SERVICER_RESID = 33;
const uint8_t AEC_AZIMUTH_VALUES_CMD = 75;

// AEC fixed-beam (beam-lock) commands. Verified against Respeaker xvf_host.py.
// AEC_FIXEDBEAMSONOFF       : (33, 37, 1, rw, int32)   — 0 = off, 1 = on
// AEC_FIXEDBEAMSAZIMUTH_VAL : (33, 81, 2, rw, radians) — two floats: beam 1, beam 2
const uint8_t AEC_FIXEDBEAMS_ONOFF_CMD = 37;
const uint8_t AEC_FIXEDBEAMS_AZIMUTH_CMD = 81;

// Audio tuning commands (resid, cmd) from Respeaker xvf_host.py.
// AUDIO_MGR_OP_L/R take (category, source): 6 = processed, 7 = AEC residual or
// ASR, 8 = user chosen (a copy of 6,3). See the XMOS output-selection table:
// https://www.xmos.com/documentation/XM-014888-PC/html/modules/fwk_xvf/doc/user_guide/03_using_the_host_application.html#output-selection
const uint8_t AUDIO_MGR_SERVICER_RESID = 35;
const uint8_t AUDIO_MGR_OP_L_CMD = 15;
const uint8_t AUDIO_MGR_OP_R_CMD = 19;
const uint8_t AEC_ASROUTONOFF_CMD = 35;  // 1 = category 7 carries ASR beams, 0 = AEC residuals
const uint8_t AEC_ASROUTGAIN_CMD = 36;
const uint8_t PP_SERVICER_RESID = 17;
const uint8_t PP_AGCMAXGAIN_CMD = 11;  // comms path only; does not affect the ASR output

// IO config servicer (GPI inputs). Verified against Respeaker xvf_host.py.
// GPI_VALUE_ALL : (36, 6, 1, ro, uint32) — bitmap, bit 0 = onboard Mute button.
// xvf_host.py calls it active-referenced, but on firmware 1.0.9 bit 0 reads 1
// while released and 0 while pressed (measured on the device).
const uint8_t IO_CONFIG_SERVICER_RESID = 36;
const uint8_t IO_CONFIG_GPI_VALUE_ALL_CMD = 6;
const uint32_t GPI_MUTE_BUTTON_BIT = 1 << 0;

const uint8_t RESID_LED = 0x0C;
const uint8_t RESID_DFU_VERSION = 0xFE;
const uint8_t I2C_COMMAND_READ_BIT = 0x80;

enum TransportProtocolReturnCode : uint8_t {
  CTRL_DONE = 0,
  CTRL_WAIT = 1,
  CTRL_INVALID = 3,
  // Servicer-level retry signal from XMOS sln_voice (CONTROL_SERVICER_COMMAND_RETRY).
  // Returned when the servicer has no fresh data yet — common during silence on the
  // AEC azimuth read. Functionally equivalent to CTRL_WAIT for the host.
  SERVICER_COMMAND_RETRY = 0x40,
};

enum RespeakerXVF3800UpdaterStatus : uint8_t {
  UPDATE_OK,
  UPDATE_COMMUNICATION_ERROR,
  UPDATE_READ_VERSION_ERROR,
  UPDATE_TIMEOUT,
  UPDATE_FAILED,
  UPDATE_BAD_STATE,
  UPDATE_IN_PROGRESS,
  UPDATE_REBOOT_PENDING,
  UPDATE_VERIFY_NEW_VERSION,
};

// Configuration enums from the XMOS firmware's src/configuration/configuration_servicer.h
enum ConfCommands : uint8_t {
  CONFIGURATION_SERVICER_RESID_VNR_VALUE = 0x00,
};

// DFU enums from https://github.com/xmos/sln_voice/blob/develop/examples/ffva/src/dfu_int/dfu_state_machine.h
enum DfuIntAltSetting : uint8_t {
  DFU_INT_ALTERNATE_FACTORY,
  DFU_INT_ALTERNATE_UPGRADE,
};

enum DfuIntState : uint8_t {
  DFU_INT_APP_IDLE,    // unused
  DFU_INT_APP_DETACH,  // unused
  DFU_INT_DFU_IDLE,
  DFU_INT_DFU_DNLOAD_SYNC,
  DFU_INT_DFU_DNBUSY,
  DFU_INT_DFU_DNLOAD_IDLE,
  DFU_INT_DFU_MANIFEST_SYNC,
  DFU_INT_DFU_MANIFEST,
  DFU_INT_DFU_MANIFEST_WAIT_RESET,
  DFU_INT_DFU_UPLOAD_IDLE,
  DFU_INT_DFU_ERROR,
};

enum DfuIntStatus : uint8_t {
  DFU_INT_DFU_STATUS_OK,
  DFU_INT_DFU_STATUS_ERR_TARGET,
  DFU_INT_DFU_STATUS_ERR_FILE,
  DFU_INT_DFU_STATUS_ERR_WRITE,
  DFU_INT_DFU_STATUS_ERR_ERASE,
  DFU_INT_DFU_STATUS_ERR_CHECK_ERASED,
  DFU_INT_DFU_STATUS_ERR_PROG,
  DFU_INT_DFU_STATUS_ERR_VERIFY,
  DFU_INT_DFU_STATUS_ERR_ADDRESS,
  DFU_INT_DFU_STATUS_ERR_NOTDONE,
  DFU_INT_DFU_STATUS_ERR_FIRMWARE,
  DFU_INT_DFU_STATUS_ERR_VENDOR,
  DFU_INT_DFU_STATUS_ERR_USBR,
  DFU_INT_DFU_STATUS_ERR_POR,
  DFU_INT_DFU_STATUS_ERR_UNKNOWN,
  DFU_INT_DFU_STATUS_ERR_STALLEDPKT,
};

enum DfuCommands : uint8_t {
  DFU_CONTROLLER_SERVICER_RESID_DFU_DETACH = 0,
  DFU_CONTROLLER_SERVICER_RESID_DFU_DNLOAD = 1,
  DFU_CONTROLLER_SERVICER_RESID_DFU_UPLOAD = 2,
  DFU_CONTROLLER_SERVICER_RESID_DFU_GETSTATUS = 3,
  DFU_CONTROLLER_SERVICER_RESID_DFU_CLRSTATUS = 4,
  DFU_CONTROLLER_SERVICER_RESID_DFU_GETSTATE = 5,
  DFU_CONTROLLER_SERVICER_RESID_DFU_ABORT = 6,
  DFU_CONTROLLER_SERVICER_RESID_DFU_SETALTERNATE = 64,
  DFU_CONTROLLER_SERVICER_RESID_DFU_TRANSFERBLOCK = 65,
  DFU_CONTROLLER_SERVICER_RESID_DFU_GETVERSION = 88,
  DFU_CONTROLLER_SERVICER_RESID_DFU_REBOOT = 89,
};

enum DFUAutomationState {
  DFU_COMPLETE = 0,
  DFU_START,
  DFU_IN_PROGRESS,
  DFU_ERROR,
};

// --- Component Classes ---

// MuteSwitch class that handles the mute functionality
class MuteSwitch : public switch_::Switch, public PollingComponent {
 public:
  void set_parent(RespeakerXVF3800 *parent) {
    parent_ = parent;
    ESP_LOGD("respeaker_xvf3800", "MuteSwitch parent set to %p", parent);
  }
  void setup() override;
  void write_state(bool state) override;
  void update() override;
  void dump_config() override;

 protected:
  RespeakerXVF3800 *parent_{nullptr};
};

// DFUVersionTextSensor class to read the firmware version
class DFUVersionTextSensor : public text_sensor::TextSensor, public PollingComponent {
 public:
  void set_parent(RespeakerXVF3800 *parent) { parent_ = parent; }
  void setup() override;
  void update() override;
  void dump_config() override;

 protected:
  RespeakerXVF3800 *parent_{nullptr};
};

// LEDBeamSensor class to read the active LED beam direction
class LEDBeamSensor : public sensor::Sensor, public PollingComponent {
 public:
  void set_parent(RespeakerXVF3800 *parent) { parent_ = parent; }
  void setup() override;
  void update() override;
  void dump_config() override;

 protected:
  RespeakerXVF3800 *parent_{nullptr};
};

#ifdef USE_BINARY_SENSOR
// MuteButtonBinarySensor polls the onboard Mute button. The DSP still mutes on a
// press by itself; this sensor only reports the button.
class MuteButtonBinarySensor : public binary_sensor::BinarySensor, public PollingComponent {
 public:
  void set_parent(RespeakerXVF3800 *parent) { parent_ = parent; }
  void update() override;
  void dump_config() override;

 protected:
  RespeakerXVF3800 *parent_{nullptr};
};
#endif

// --- Main Hub Class ---

class RespeakerXVF3800 : public i2c::I2CDevice, public Component {
 public:
  void setup() override;
  bool can_proceed() override {
    return this->is_failed() || (this->version_read_() && (this->versions_match_() || !this->firmware_bin_is_valid_()));
  }
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE - 1; }
  void loop() override;

#ifdef USE_RESPEAKER_XVF3800_STATE_CALLBACK
  void add_on_state_callback(std::function<void(DFUAutomationState, float, RespeakerXVF3800UpdaterStatus)> &&callback) {
    this->state_callback_.add(std::move(callback));
  }
#endif

  void set_reset_pin(GPIOPin *reset_pin) { reset_pin_ = reset_pin; }

  void set_firmware_bin(const uint8_t *data, const uint32_t len) {
    this->firmware_bin_ = data;
    this->firmware_bin_length_ = len;
  }

  #ifdef USE_BINARY_SENSOR
  void set_mute_state(binary_sensor::BinarySensor *mute_state) { this->mute_state_ = mute_state; }
  #endif

  void set_firmware_version(text_sensor::TextSensor* firmware_version) {
    this->firmware_version_ = firmware_version;
  }
  
  void set_firmware_version(uint8_t major, uint8_t minor, uint8_t patch) {
    this->firmware_bin_version_major_ = major;
    this->firmware_bin_version_minor_ = minor;
    this->firmware_bin_version_patch_ = patch;
  }

  void start_dfu_update();
  uint8_t read_vnr();

  // Public methods for child components
  bool read_gpo_values(uint8_t *buffer, uint8_t *status);
  bool read_gpio_status(uint32_t *gpio_status);
  bool read_mute_status();
  void write_mute_status(bool value);
  // Reads the Mute button. Returns false if the DSP gave no valid answer.
  bool read_mute_button(bool &pressed);
  
  // Individual LED ring control (12 LEDs)
  void set_led_ring(uint32_t *rgb_array);
  
  std::string read_dfu_version();
  
  // Read LED beam direction (0-11)
  int read_led_beam_direction();

  // Beam lock: pin the AEC beam to the current azimuth for the duration of an utterance,
  // then release it. Intended to be called from voice_assistant lambdas.
  void lock_beam();
  void unlock_beam();

  // Voice direction in degrees [0, 360): the locked direction while the beam is
  // locked, else the last auto-select azimuth of the beam poll. No bus access.
  // NAN when the poll has no value younger than AZIMUTH_MAX_AGE_MS.
  float get_azimuth_degrees() const;

  // Setters for child components
  void set_mute_switch(MuteSwitch *mute_switch) { mute_switch_ = mute_switch; }
  void set_dfu_version_sensor(DFUVersionTextSensor *dfu_version_sensor) { dfu_version_sensor_ = dfu_version_sensor; }
  void set_led_beam_sensor(LEDBeamSensor *led_beam_sensor) { led_beam_sensor_ = led_beam_sensor; }

  void set_asr_route(bool asr_route);
  void set_asr_out_gain(float gain);
  void set_agc_max_gain(float gain);

 protected:
#ifdef USE_RESPEAKER_XVF3800_STATE_CALLBACK
  CallbackManager<void(DFUAutomationState, float, RespeakerXVF3800UpdaterStatus)> state_callback_{};
#endif
  RespeakerXVF3800UpdaterStatus dfu_update_send_block_();
  uint32_t load_buf_(uint8_t *buf, const uint8_t max_len, const uint32_t offset);
  bool firmware_bin_is_valid_() { return this->firmware_bin_ != nullptr && this->firmware_bin_length_; }
  bool version_read_();
  bool versions_match_();
  bool chip_version_at_least_(uint8_t major, uint8_t minor, uint8_t patch);

  bool dfu_get_status_();
  bool dfu_get_version_();
  bool dfu_reboot_();
  bool dfu_set_alternate_();
  bool dfu_check_if_ready_();

  GPIOPin *reset_pin_{nullptr};
  #ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *mute_state_{nullptr};
  #endif
  text_sensor::TextSensor *firmware_version_{nullptr};

  bool get_firmware_version_();

  uint8_t dfu_state_{0};
  uint8_t dfu_status_{0};
  uint32_t dfu_status_next_req_delay_{0};

  uint8_t const *firmware_bin_{nullptr};
  uint32_t firmware_bin_length_{0};
  uint8_t firmware_bin_version_major_{0};
  uint8_t firmware_bin_version_minor_{0};
  uint8_t firmware_bin_version_patch_{0};

  uint8_t firmware_version_major_{0};
  uint8_t firmware_version_minor_{0};
  uint8_t firmware_version_patch_{0};

  uint32_t bytes_written_{0};
  uint32_t last_progress_{0};
  uint32_t last_ready_{0};
  uint32_t status_last_read_ms_{0};
  uint32_t update_start_time_{0};
  RespeakerXVF3800UpdaterStatus dfu_update_status_{UPDATE_OK};

  // Child components
  MuteSwitch *mute_switch_{nullptr};
  DFUVersionTextSensor *dfu_version_sensor_{nullptr};
  LEDBeamSensor *led_beam_sensor_{nullptr};

  // Beam-lock state. While true, read_led_beam_direction() reads beam-1 (the
  // pinned fixed beam) from the chip instead of the auto-select beam, so the
  // LED ring stays pointed at the captured wake-word direction.
  bool beam_locked_{false};

  // Last azimuth the DSP actually returned, and when. The 10 Hz beam poll keeps
  // this fresh, so lock_beam() can pin the beam without its own blocking read.
  float last_azimuth_rad_{0.0f};
  uint32_t last_azimuth_ms_{0};
  static constexpr uint32_t AZIMUTH_MAX_AGE_MS = 500;

  // Last frame written by set_led_ring(), so an unchanged frame skips the bus.
  uint32_t last_led_frame_[12]{};
  bool led_frame_valid_{false};

  // The chip resets these on power-up; they are (re)applied after boot.
  bool asr_route_{false};
  float asr_out_gain_{1.0f};
  float agc_max_gain_{32.0f};
  bool tuning_applied_{false};
  void apply_audio_tuning_();
  void write_asr_out_gain_();
  void write_agc_max_gain_();

  // Helper method for XMOS communication
  void xmos_write_bytes(uint8_t resid, uint8_t cmd, const uint8_t *value, uint8_t write_byte_num);

  // Reads one of the four AEC azimuth slots (radians) returned by cmd 75:
  //   0 = beam 1 (fixed beam 1 when fixed mode is on)
  //   1 = beam 2 (fixed beam 2 when fixed mode is on)
  //   2 = free-running beam
  //   3 = auto-select beam (default — what the adaptive LED follows)
  // Returns true on success. Shared by read_led_beam_direction() and lock_beam().
  // Single attempt, never blocks: a CTRL_WAIT answer returns false and the next
  // poll retries. Do not add a retry loop here — it runs in the main loop.
  bool read_azimuth_radians_(float &out_radians, uint8_t beam_index = 3);
};

}  // namespace respeaker_xvf3800
}  // namespace esphome
