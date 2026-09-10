# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repository is

This repository holds an ESPHome firmware definition for the Seeed ReSpeaker Lite Voice Kit (XIAO ESP32-S3 plus an XMOS
XU316 audio front end). It is not an application with a build system. It is a YAML package plus one custom ESPHome
component. Home Assistant users consume it as a remote package.

There is no test suite, no linter configuration, and no CI workflow. `.github/` holds only `FUNDING.yml`.

## Commands

Compile and upload need the ESPHome CLI and a device config that includes the package.

```bash
esphome config   config/respeaker-satellite-dashboard-example.yaml   # resolve packages, validate, print merged YAML
esphome compile  config/respeaker-satellite-dashboard-example.yaml   # build only
esphome run      config/respeaker-satellite-dashboard-example.yaml   # build, flash, then attach logs
esphome logs     config/respeaker-satellite-dashboard-example.yaml   # attach to a running device
```

The example config needs `wifi_ssid`, `wifi_password`, and `ota_password` in a `secrets.yaml` file beside it.

`esphome config` is the fastest check for a YAML change. It downloads every remote model file and the XMOS firmware
binary, so it also proves that all URLs and the MD5 checksum are correct.

## Layout

- `config/common/respeaker-satellite-base.yaml` — the whole device definition, about 1650 lines. Almost every change
  belongs here.
- `config/respeaker-satellite-dashboard-example.yaml` — the thin user config. It imports the base package from
  `github://formatBCE/Respeaker-Lite-ESPHome-integration/...@main`.
- `esphome/components/respeaker_lite/` — the custom ESPHome external component (Python codegen plus C++).
- `microwakeword/models/v2/` — the `kenobi` wake word model, served over raw GitHub URL.
- `respeaker_lite_i2s_dfu_firmware_48k_v1.1.0.bin` — the XMOS firmware that the device flashes to itself.
- `blueprints/automation/formatbce/` — Home Assistant automation blueprints, imported by raw GitHub URL.
- `readme/`, `casing/` — user documentation and 3D print files.

## Important: the source of truth is GitHub, not the working tree

`external_components` in the base YAML pulls `respeaker_lite` from this repository at `ref: main`, and it pulls a patched
`i2s_audio` from the `respeaker_microphone` branch of `formatBCE/esphome`. The base YAML also references the wake word
model and the XMOS firmware by raw GitHub URL.

So a local edit to `esphome/components/respeaker_lite/` has no effect on a build. To test a component change, point the
source at a local path first:

```yaml
external_components:
  - source: { type: local, path: esphome/components }
    components: [respeaker_lite]
```

Do not commit that change. The same applies to the model and firmware URLs.

## Architecture

### Two processors, one I2C bus

The ESP32-S3 runs ESPHome. The XMOS board does the microphone array processing, the acoustic echo cancellation, and the
hardware mute. The `respeaker_lite` component talks to the XMOS board over I2C at address `0x42` on bus `internal_i2c`
(SDA GPIO5, SCL GPIO6). Audio does not use that bus. Audio uses I2S, where the ESP32 is the **secondary** and the XMOS
board drives the clocks, at 48 kHz and 32 bit.

The component exposes three things over I2C:
- The XMOS firmware version, as a text sensor.
- The hardware mute state, as an internal binary sensor.
- A DFU client that flashes the XMOS board.

### Automatic XMOS firmware update

`RespeakerLite::setup()` resets the XMOS board, waits 3 seconds, reads the version, and starts a DFU transfer if the
version does not match the compiled-in binary. `can_proceed()` blocks the rest of ESPHome setup until the version matches.
The transfer runs in `loop()` as a state machine over `RespeakerLiteUpdaterStatus`.

The firmware binary is embedded into the ESP32 image at compile time. `__init__.py` downloads the URL, checks the MD5,
and emits the bytes as a PROGMEM array. To change the firmware you must update the `url`, the `version`, and the `md5`
together in the `respeaker_lite:` block. A wrong MD5 fails validation, and a wrong `version` string causes a reflash on
every boot.

The `on_begin`, `on_end`, `on_error`, and `on_progress` automations only compile when at least one of them is present.
They set `USE_RESPEAKER_LITE_STATE_CALLBACK`.

### LED state machine

All LED output goes through the `control_leds` script. That script is a priority chain: ImprovBLE, then initialization,
then loss of the Wi-Fi or the API connection, then the button touch, then the timer or the alarm state, then the voice
assistant phase. Each branch calls a dedicated `control_leds_*` child script.

Never write to a light directly from an automation. Set the state that `control_leds` reads, then call
`script.execute: control_leds`. The voice assistant phase lives in the `voice_assistant_phase` global, and the phase
numbers come from the `substitutions:` block at the top of the base YAML.

### Audio pipeline

```
announcement_resampling_speaker ─┐
                                 ├─> mixing_speaker ─> i2s_audio_speaker ─> aic3204_dac
media_resampling_speaker ────────┘
```

Both resamplers force 48 kHz, because the mixer needs one common sample rate. `micro_wake_word` reads channel 1 of the
stereo `i2s_mics` microphone with `gain_factor: 4`. The `stop` model is internal and the `activate_stop_word_once` script
enables it only while the assistant speaks.

### Home Assistant interface

The device sends four custom events to Home Assistant: `esphome.tts_uri`, `esphome.stt_text`, plus the wake word and the
alarm events. Corresponding scripts named `send_*_event` fire them. It exposes exactly three API actions: `start_va`,
`stop_va`, and `set_time_zone`. The alarm time is set through the `alarm_time` datetime entity, not through an action.
`readme/alarms.md` still tells the user to call an `esphome.{device}_set_alarm_time` action, and that action no longer
exists.

Time zone and alarm settings persist in `restore_value: yes` globals. The `on_boot` handler at priority -100 applies them
again with `setenv("TZ", ...)`.

## Conventions

- Version bumps change `esphome.project.version` and `esphome.min_version` in the base YAML together. Both track the
  ESPHome release, for example `2026.6.0`.
- Blueprint files and model files are referenced by absolute raw GitHub URL. A rename breaks live installations.
- New scripts that can overlap use `mode: restart`. See commit 3136cf7.
- The `disable_buttons` switch guards every button automation. Add the same guard to a new button action.
