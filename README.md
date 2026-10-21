### _Check out [Koala Satellite](https://github.com/formatBCE/Koala-Satellite), the next step of evolution for this project!_

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/formatbce)

## What is it
This repository contains example code to integrate Seeed Respeaker Lite Voice Kit (with XIAO ESP32-S3 on board) with ESPHome.
I'm thankful for continuous help of ESPHome team and Seeed developers, that let me put this thing together. 
It's using [official code for Voice Preview Edition by ESPHome team](https://github.com/esphome/home-assistant-voice-pe), as well as firmware and guides from [Seeed Wiki](https://wiki.seeedstudio.com/xiao_respeaker/) and [their repository for Respeaker](https://github.com/respeaker/ReSpeaker_Lite/tree/master).

The built-in sounds come from the Voice PE project. The sounds are © 2024 by Clayton Charles Tapp, under CC BY 4.0. The `sounds/` directory holds copies in Ogg Opus format. Read [sounds/LICENSE.md](sounds/LICENSE.md) for the changes.

## *Disclaimer
### Use this on your own risk. Expect breaking changes.
Although Voice PE is released, parts of its code are still under active development. I will try to update YAML and XMOS board firmware as soon as i can, but it can't be instantenuous...
If you encounter a problem, before creating ticket here you may go to PE and Seeed repos, linked above, and see if something changed there. I'll gladly accept pull-requests. :)

## Why?
I completely abandoned proprietary assistants in the Summer of 2024 and fully committed to Assist all-hands.
With this board i finally can build device, that will satisfy me and (more important) my family as their trusted voice assistant.

## What to do with it?
__* Thanks to Mike aka @mikey60 and his fork to nabu_microphone, this project started to use the 48kHz sample rate for better music playback quality. Since 2026.10.2, the stock ESPHome `resampler` microphone does the conversion, and the firmware needs ESPHome 2026.10.0 or newer.*__
1. Get Respeaker Lite with ESP32 soldered to it (you may solder it yourself, pins on the back can remain dry, they're not used).
2. [Solder USR to D2 and MUTE to D3 pins](https://wiki.seeedstudio.com/respeaker_button/). _**ATTENTION! This step is mandatory, as without it the buttons on satellite won't work as intended.**_
3. [Flash 48kHz I2S firmware of version **not lower than 1.1.0** to the XMOS board](https://wiki.seeedstudio.com/xiao_respeaker/#flash-the-i2s-firmware) (pay attention to USB port, you need the main board port, not ESP32 one). Make sure you're using 48kHz version, as 16kHz version won't work with this repo. You can use included [firmware file](/respeaker_lite_i2s_dfu_firmware_48k_v1.1.0.bin) to be sure.
4. Flash the ESPHome firmware to the ESP32. Use the ESP32 USB port.
   - Copy [the example configuration](/config/respeaker-satellite-dashboard-example.yaml) into ESPHome, and change it for your device.
   - You do not need to download other files. The example gets the base configuration from this GitHub repository.
   - The example uses one release of this repository. The device does not change until you select a newer release. Read [Releases and updates](#releases-and-updates).
5. Add device to Home Assistant.

### Here are also some [useful blueprints to use with your satellite](readme/blueprints.md)

## Current abilities
The device has feature parity with Voice PE. Pretty much everything works same way, except volume control (solved in [Koala Satellite](https://github.com/formatBCE/Koala-Satellite)).
There are some additional things i've added for convenience:
- switches to turn off button sounds, mute/unmute sounds;
- sensors for next device timer properties (time is updated once per 5 sec, name is available if set);
- TTS URI event `esphome.tts_uri`, sent to Home Assistant every time there's voice response (check out example automation to redirect response to other media player [here](readme/tts_uri.md));
- STT text event `esphome.stt_text`, sent every time there's text of response available (you may use it to display readable response on your dashboard, for example);
- daily alarm functionality, read more on it [here](readme/alarms.md);
- "Snooze" for a ringing timer or alarm, read more on it [here](readme/alarms.md#snooze). The "Snooze" wake word model was trained by [dlarkinsPonderosa](https://github.com/dlarkinsPonderosa).

## Releases and updates
Each release has a tag, for example `2026.9.1`. The [tags page](https://github.com/formatBCE/Respeaker-Lite-ESPHome-integration/tags) shows all releases.

The `respeaker_lite_ref` substitution in your configuration selects the release. One value controls the base configuration, the `respeaker_lite` component, the wake word model, and the XMOS firmware.

To update the device:
1. Set `respeaker_lite_ref` to the tag of the new release.
2. Install the configuration on the device again.

Use `main` only to test new code. A change in `main` can break the build or the device. To get the newest code before a release, set `respeaker_lite_ref` to `main`.

## Change the configuration
The firmware is a set of packages in the [packages](/packages) directory:

| Package | Content |
| --- | --- |
| `core/base.yaml` | Boot, network, API, and diagnostic entities |
| `core/voice-assistant.yaml` | Wake word, voice pipeline, sounds, and Home Assistant events |
| `core/media-player.yaml` | Media player, Sendspin, and audio ducking |
| `core/timers-alarm.yaml` | Timers, daily alarm, snooze, and time zone |
| `core/controls.yaml` | User button actions, button sounds, and the Button press event |
| `respeaker-lite/hardware.yaml` | ReSpeaker Lite board: audio, XMOS, and firmware update |
| `lite/device.yaml` | XIAO ESP32-S3: user button pin and the RGB LED |

[Koala Satellite](https://github.com/formatBCE/Koala-Satellite) and the [Respeaker XVF3800 integration](https://github.com/formatBCE/Respeaker-XVF3800-ESPHome-integration) also use the `core/` packages. A change to a core package changes these devices too.

The example configuration includes all packages through `config/common/respeaker-satellite-base.yaml`. You can change the result in your own configuration. You do not need to copy the files.

**Change a value.** Set a substitution with the same name. Your value overrides the value in the package. For example, use your own wake sound:

```yaml
substitutions:
  wake_word_triggered_sound_file: https://example.com/my-wake-sound.flac
```

**Add steps to a script.** Use `!extend`. The new steps run after the existing steps:

```yaml
script:
  - id: !extend control_leds
    then:
      - logger.log: "LED state changed"
```

**Remove an entity.** Use `!remove`:

```yaml
button:
  - id: !remove restart_button
```

## DFU software auto-update
Since version 2025.2.2, the firmware includes the XMOS firmware for the ReSpeaker Lite board.

**CAUTION:** Do not disconnect the power while the LED flashes white. An interrupted XMOS update can make the board stop working.

At the first start after an update, the ESP32 installs the XMOS firmware automatically. The LED flashes white during the installation. The LED flashes green when the installation is complete.

The automatic installation starts only if the XMOS firmware on the board is version 1.0.8 or newer. On older versions, the XMOS update can erase the factory firmware. The board then does not start, and its recovery needs a SOIC-8 clip programmer. Read [issue #94](https://github.com/formatBCE/Respeaker-Lite-ESPHome-integration/issues/94).

If the XMOS firmware is older than 1.0.8, the ESP32 does not install the update. The LED shows the error state, and the log shows the version that the ESP32 found. To update the board, do step 3 of [What to do with it?](#what-to-do-with-it). The USB procedure uses the same XMOS update function, so the same risk can apply.

### Casing
I made some casing to improve family approval factor. [Check it out.](casing/Casing.md)


[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/formatbce)
