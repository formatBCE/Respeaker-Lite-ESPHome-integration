#pragma once

#include "esphome/components/audio_dac/audio_dac.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace aic3104 {

// TLV320AIC3104 Register Addresses
// Page 0
#define AIC3104_PAGE_CTRL               0x00 // Register 0 - Page Control
#define AIC3104_SW_RST                  0x01 // Register 1 - Software Reset
#define AIC3104_CODEC_SR_SEL            0x02 // Register 2: Codec Sample Rate Select Register
#define AIC3104_PLL_Q_P                 0x03 // Register 3: PLL Programming Register A (Q and P value)
#define AIC3104_PLL_J                   0x04 // Register 4: PLL Programming Register B (J value)
#define AIC3104_PLL_D_MSB               0x05 // Register 5: PLL Programming Register C (D value MSB)
#define AIC3104_PLL_D_LSB               0x06 // Register 6: PLL Programming Register D (D value LSB)
#define AIC3104_CODEC_DATA_PATH         0x07 // Register 7: Codec Data-Path Setup Register
#define AIC3104_ASD_IF_CTRL_A           0x08 // Register 8: Audio Serial Data Interface Control Register A
#define AIC3104_ASD_IF_CTRL_B           0x09 // Register 9: Audio Serial Data Interface Control Register B
#define AIC3104_ASD_IF_CTRL_C           0x0A // Register 10: Audio Serial Data Interface Control Register C
#define AIC3104_OVRF_STATUS_PLL_R       0x0B // Register 11: Audio Codec Overflow Flag Register and PLL R value
#define AIC3104_DIGITAL_FILTER          0x0C // Register 12: Audio Codec Digital Filter Control Register
#define AIC3104_HEADSET_BUT_DET_A       0x0D // Register 13: Headset, Button Press Detection Register A
#define AIC3104_HEADSET_BUT_DET_B       0x0E // Register 14: Headset, Button Press Detection Register B
#define AIC3104_LEFT_ADC_PGA            0x0F // Register 15: Left-ADC PGA Gain Control Register
#define AIC3104_RIGHT_ADC_PGA           0x10 // Register 16: Right-ADC PGA Gain Control Register
#define AIC3104_MIC2LR_LADC             0x11 // Register 17: MIC2L/R to Left-ADC Control Register
#define AIC3104_MIC2_LINE2_RADC         0X12 // Register 18: MIC2/LINE2 to Right-ADC Control Register
#define AIC3104_MIC1LP_LINE1LP_LADC     0x13 // Register 19: MIC1LP/LINE1LP to Left-ADC Control Register
#define AIC3104_MIC1RP_LINE1RP_LADC     0x15 // Register 21: MIC1RP/LINE1RP to Left-ADC Control Register
#define AIC3104_MIC1RP_LINE1RP_RADC     0x16 // Register 22: MIC1RP/LINE1RP to Right-ADC Control Register
#define AIC3104_MIC1LP_LINE1LP_RADC     0x18 // Register 24: MIC1LP/LINE1LP to Right-ADC Control Register
#define AIC3104_MICBIAS_CTRL            0x19 // Register 25: MICBIAS Control Register
#define AIC3104_LAGC_CTRL_A             0x1A // Register 26: Left-AGC Control Register A
#define AIC3104_LAGC_CTRL_B             0x1B // Register 27: Left-AGC Control Register B
#define AIC3104_LAGC_CTRL_C             0x1C // Register 28: Left-AGC Control Register C
#define AIC3104_RAGC_CTRL_A             0x1D // Register 29: Right-AGC Control Register A
#define AIC3104_RAGC_CTRL_B             0x1E // Register 30: Right-AGC Control Register B
#define AIC3104_RAGC_CTRL_C             0x1F // Register 31: Right-AGC Control Register C
#define AIC3104_LAGC_GAIN               0x20 // Register 32: Left-AGC Gain Register
#define AIC3104_RAGC_GAIN               0x21 // Register 33: Right-AGC Gain Register
#define AIC3104_LAGC_NOISE_GATE_DB      0x22 // Register 34: Left-AGC Noise Gate Debounce Register
#define AIC3104_RAGC_NOISE_GATE_DB      0x23 // Register 35: Right-AGC Noise Gate Debounce Register
#define AIC3104_ADC_FLAG                0x24 // Register 36: ADC Flag Register
#define AIC3104_DAC_POWER_OUTPUT        0x25 // Register 37: DAC Power and Output Driver Control Register
#define AIC3104_HP_OUTPUT_CTRL          0x26 // Register 38: High-Power Output Driver Control Register
#define AIC3104_HP_OUTPUT_STAGE         0x28 // Register 40: High-Power Output Stage Control Register
#define AIC3104_DAC_OUTPUT_SWITCH       0x29 // Register 41: DAC Output Switching Control Register
#define AIC3104_OUTPUT_POP_REDUCE       0x2A // Register 42: Output Driver Pop Reduction Register
#define AIC3104_LEFT_DAC_VOLUME         0x2B // Register 43: Left-DAC Digital Volume Control Register
#define AIC3104_RIGHT_DAC_VOLUME        0x2C // Register 44: Right-DAC Digital Volume Control Register
#define AIC3104_PGA_L_HPLOUT_VOLUME     0x2E // Register 46: PGA_L to HPLOUT Volume Control Register
#define AIC3104_DAC_L1_HPLOUT_VOLUME    0x2F // Register 47: DAC_L1 to HPLOUT Volume Control Register
#define AIC3104_PGA_R_HPLOUT_VOLUME     0x31 // Register 49: PGA_R to HPLOUT Volume Control Register
#define AIC3104_DAC_R1_HPLOUT_VOLUME    0x32 // Register 50: DAC_R1 to HPLOUT Volume Control Register
#define AIC3104_HPLOUT_LEVEL            0x33 // Register 51: HPLOUT Output Level Control Register
#define AIC3104_PGA_L_HPLCOM_VOLUME     0x35 // Register 53: PGA_L to HPLCOM Volume Control Register
#define AIC3104_DAC_L1_HPLCOM_VOLUME    0x36 // Register 54: DAC_L1 to HPLCOM Volume Control Register
#define AIC3104_PGA_R_HPLCOM_VOLUME     0x38 // Register 56: PGA_R to HPLCOM Volume Control Register
#define AIC3104_DAC_R1_HPLCOM_VOLUME    0x39 // Register 57: DAC_R1 to HPLCOM Volume Control Register
#define AIC3104_HPLCOM_LEVEL            0x3A // Register 58: HPLCOM Output Level Control Register
#define AIC3104_PGA_L_HPROUT_VOLUME     0x3C // Register 60: PGA_L to HPROUT Volume Control Register
#define AIC3104_DAC_L1_HPROUT_VOLUME    0x3D // Register 61: DAC_L1 to HPROUT Volume Control Register
#define AIC3104_PGA_R_HPROUT_VOLUME     0x3F // Register 63: PGA_R to HPROUT Volume Control Register
#define AIC3104_DAC_R1_HPROUT_VOLUME    0x40 // Register 64: DAC_R1 to HPROUT Volume Control Register
#define AIC3104_HPROUT_LEVEL            0x41 // Register 65: HPROUT Output Level Control Register
#define AIC3104_PGA_L_HPRCOM_VOLUME     0x43 // Register 67: PGA_L to HPRCOM Volume Control Register
#define AIC3104_DAC_L1_HPRCOM_VOLUME    0x44 // Register 68: DAC_L1 to HPRCOM Volume Control Register
#define AIC3104_PGA_R_HPRCOM_VOLUME     0x46 // Register 70: PGA_R to HPRCOM Volume Control Register
#define AIC3104_DAC_R1_HPRCOM_VOLUME    0x47 // Register 71: DAC_R1 to HPRCOM Volume Control Register
#define AIC3104_HPRCOM_LEVEL            0x48 // Register 72: HPRCOM Output Level Control Register
#define AIC3104_PGA_L_LEFT_LOP_VOLUME   0x51 // Register 81: PGA_L to LEFT_LOP/M Volume Control Register
#define AIC3104_DAC_L1_LEFT_LOP_VOLUME  0x52 // Register 82: DAC_L1 to LEFT_LOP/M Volume Control Register
#define AIC3104_PGA_R_LEFT_LOP_VOLUME   0x54 // Register 84: PGA_R to LEFT_LOP/M Volume Control Register
#define AIC3104_DAC_R1_LEFT_LOP_VOLUME  0x55 // Register 85: DAC_R1 to LEFT_LOP/M Volume Control Register
#define AIC3104_LEFT_LOP_LEVEL          0x56 // Register 86: LEFT_LOP/M Output Level Control Register
#define AIC3104_PGA_L_RIGHT_LOP_VOLUME  0x58 // Register 88: PGA_L to RIGHT_LOP/M Volume Control Register
#define AIC3104_DAC_L1_RIGHT_LOP_VOLUME 0x59 // Register 89: DAC_L1 to RIGHT_LOP/M Volume Control Register
#define AIC3104_PGA_R_RIGHT_LOP_VOLUME  0x5B // Register 91: PGA_R to RIGHT_LOP/M Volume Control Register
#define AIC3104_DAC_R1_RIGHT_LOP_VOLUME 0x5C // Register 92: DAC_R1 to RIGHT_LOP/M Volume Control Register
#define AIC3104_RIGHT_LOP_LEVEL         0x5D // Register 93: RIGHT_LOP/M Output Level Control Register
#define AIC3104_MODULE_POWER_STATUS     0x5E // Register 94: Module Power Status Register
#define AIC3104_OUTPUT_SCD_STATUS       0x5F // Register 95: Output Driver Short-Circuit Detection Status Register
#define AIC3104_STICKY_INT_FLAGS        0x60 // Register 96: Sticky Interrupt Flags Register
#define AIC3104_REALTIME_INT_FLAGS      0x61 // Register 97: Real-Time Interrupt Flags Register
#define AIC3104_CLOCK_REG               0x65 // Register 101: Clock Register
#define AIC3104_CLOCK_GEN_CTRL          0x66 // Register 102: Clock Generation Control Register
#define AIC3104_LAGC_NEW_ATTACK         0x67 // Register 103: Left-AGC New Programmable Attack Time Register
#define AIC3104_LAGC_NEW_DECAY          0x68 // Register 104: Left-AGC New Programmable Decay Time Register
#define AIC3104_RAGC_NEW_ATTACK         0x69 // Register 105: Right-AGC New Programmable Attack Time Register
#define AIC3104_RAGC_NEW_DECAY          0x6A // Register 106: Right-AGC New Programmable Decay Time Register
#define AIC3104_NEW_ADC_PATH            0x6B // Register 107: New Programmable ADC Digital Path and I2C Bus Condition Register
#define AIC3104_PASSIVE_BYPASS          0x6C // Register 108: Passive Analog Signal Bypass Selection During Power Down Register
#define AIC3104_DAC_QUIESCENT_CUR       0x6D // Register 109: DAC Quiescent Current Adjustment Register

class AIC3104 : public audio_dac::AudioDac, public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;

  bool set_mute_off() override;
  bool set_mute_on() override;
  bool set_volume(float volume) override;

  bool is_muted() override;
  float volume() override;

 protected:
  bool write_mute_();
  bool write_volume_();

  float volume_{0};
};

}  // namespace aic3104
}  // namespace esphome
