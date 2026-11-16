## Installing the Koala firmware

### Flashing DFU software to Respeaker Lite
Your Respeaker Lite board comes pre-flashed with some software. However, most probably it's software, built to use the board as external microphone for computer over USB. There's also chance that the flashed firmware is intended for usage over I2S (which we need), but is using 16kHz sample rate (which we don't, as Koala is using 48kHz sample rate for better sound playback).

To flash Respeaker board with correct software, you can use [included sowtware](/respeaker_lite_i2s_dfu_firmware_48k_v1.1.0.bin) (or take one with version >= 1.1.0 and 48kHz sample rate from [official repo](https://github.com/respeaker/ReSpeaker_Lite/tree/master/xmos_firmwares)) with [instructions from Seeed site](https://wiki.seeedstudio.com/xiao_respeaker/#flash-the-i2s-firmware).

#### _Please re-insert the board after flash and make sure that displayed version is correct one._

### DFU software auto-update
Since version 2025.2.2, the Koala firmware includes corresponding DFU firmware for Respeaker Lite board. On first start after update, new firmware will be installed to Respeaker automatically. You will see Respeaker LED flashing yellow, while installing, and green on successful install.
So now there's no need to update DFU firmware. Woohoo!

### Flashing ESPHome configuration to ESP32-S3 board

#### Use [this page](https://formatbce.github.io/Respeaker-ESPHome-integration/koala/) to flash your device with factory firmware (featuring OTA updates)
It will install the firmware, similar to the one Voice PE has. It will be able to connect to any WiFi network via BLE Improv, and in case of reconnecting to other WiFi you will have to just reset the device to factory settings. Also this version will receive OTA from this repository. You will see Home Assistant system prompt to update your device once the update is available. Basically set-and-forget option, and the option to choose if you're making the device for a friend/family.

#### Manual building with ESPHome
1. Copy the [dashboard configuration](/config/koala-dashboard.yaml) into your ESPHome dashboard. You do not need other files. The configuration gets the firmware from GitHub.
2. Put `wifi_ssid` and `wifi_password` into your ESPHome secrets.
3. Build the firmware with ESPHome and flash it with your method of choice.

The `respeaker_lite_ref` substitution in the configuration selects the release. The device does not change until you change this value. To update the device, set `respeaker_lite_ref` to the tag of a newer release and install the configuration again. The [tags page](https://github.com/formatBCE/Respeaker-ESPHome-integration/tags) shows all releases. The Respeaker Lite and Respeaker XVF3800 devices use the same releases.

#### Move from the old repository
The Koala firmware was in the `Koala-Satellite` repository before. A configuration that uses that repository continues to build its old release, but it gets no new releases. To get new releases, change your dashboard configuration:
1. Replace the `koala_ref` substitution with `respeaker_lite_ref`. Set it to a release of this repository.
2. In the `packages:` block, set `url` to `https://github.com/formatBCE/Respeaker-ESPHome-integration` and `ref` to `${respeaker_lite_ref}`.

The [dashboard configuration](/config/koala-dashboard.yaml) shows the result. A device that you installed with the web installer moves to this repository with its next update.

#### Update from an older manual installation
Older instructions told you to copy `koala-base.yaml` into the `common` directory. That copy does not get updates. Do these steps to change to the new configuration:
1. Make a backup of your Koala configuration and of `common/koala-base.yaml`.
2. Replace your Koala configuration with the new [dashboard configuration](/config/koala-dashboard.yaml). Keep your own changes, for example the device name.
3. Install the configuration on the device.
4. Delete `common/koala-base.yaml` if no other configuration uses it.

The firmware is now a set of packages. Most of them are the voice satellite core and the ReSpeaker Lite hardware, and the [Respeaker Lite page](/readme/lite/README.md#change-the-configuration) tells you how to change them without a copy. The Koala hardware is in [packages/koala/device.yaml](/packages/koala/device.yaml).

#### Use another ESP32-S3 board
The firmware uses the pins of the ESP32-S3 DevKitC. For another board, set the pins in the `substitutions:` block of your dashboard configuration. Your values override the defaults.

| Substitution | DevKitC default | Function |
| --- | --- | --- |
| `user_button_pin` | `GPIO42` | Koala button |
| `led_ring_pin` | `GPIO13` | Data line of the LED ring |
| `dial_a_pin` | `GPIO47` | Encoder `CLK` |
| `dial_b_pin` | `GPIO21` | Encoder `DT` |
| `dial_button_pin` | `GPIO20` | Encoder `SW` |
| `esp_onboard_led_pin` | `GPIO48` | RGB LED on the DevKitC |
| `dial_button_mutes_speaker` | `"false"` | Set to `"true"` to toggle the speaker mute with a press of the dial |

Use a board with 16 MB flash. The ReSpeaker Lite pins do not change.

If your board has no RGB LED on `GPIO48`, remove that light:

```yaml
light:
  - id: !remove led_esp_onboard
```

On the DevKitC, `GPIO20` is a USB pin of the ESP32-S3. Noise on it gives false presses of the dial. For this reason, `dial_button_mutes_speaker` is `"false"` by default. Enable it only on a board where the dial press uses another pin.

A user reported these pins on a XIAO ESP32-S3. I did not test them:

- The LED ring uses the default pin, `GPIO13`. On the XIAO ESP32-S3 Plus, `GPIO13` is on a small pad between the through-hole pins. On the standard XIAO ESP32-S3, `GPIO13` is on the pads for the camera (Sense) board, so you must solder a thin wire to it.
- The encoder uses these pins on the XIAO ESP32-S3 Plus:

```yaml
substitutions:
  dial_a_pin: GPIO12
  dial_b_pin: GPIO41
  dial_button_pin: GPIO10
  dial_button_mutes_speaker: "true"
```

Set `user_button_pin` to a free pin of your board. The report does not include a pin for the Koala button.

### Connecting to Home Assistant
- I'm sure you already know how to connect ESPHome devices to Home Assistant. If it's your first time, please check the [official documentation](https://www.home-assistant.io/integrations/esphome/).
- In case of Factory firmware you will need to connect the device to the WiFi first. Check [Improv docs](https://www.home-assistant.io/integrations/improv_ble/) for details.
_**(When asked to authorize by pressing button on device, use small round button!)**_
- The voice satellite onboarding wizard will help you setup the pipeline and wake word.

### Using the alarm functionality.
Since 2025.4.1, the device has built-in alarm, that you can setup and use in your smart home. Check [this short tutorial](/readme/alarms.md) on how to set it up!

Since 2026.10.0, you can say "Snooze" to a ringing timer or alarm. The ring stops, and starts again after the **Snooze duration**. Read [Snooze](/readme/alarms.md#snooze) for the details.

### _Happy tinkering!_
