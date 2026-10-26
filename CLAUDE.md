# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repository is

This repository holds an ESPHome firmware definition for the Seeed ReSpeaker Lite Voice Kit (XIAO ESP32-S3 plus an XMOS
XU316 audio front end). It is not an application with a build system. It is a YAML package plus one custom ESPHome
component. Home Assistant users consume it as a remote package.

There is no test suite and no linter configuration. The only CI workflow is `.github/workflows/release.yaml`, which builds
and publishes releases.

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

- `packages/` — the device definition, split into packages. Three layers, included in this order:
  - `core/` — the board-independent voice satellite: `base.yaml` (boot, network, API, phase globals), `voice-assistant.yaml`
    (wake word, pipeline, sounds, HA events), `media-player.yaml`, `timers-alarm.yaml`, and `controls.yaml` (the Voice PE
    button behavior). The core is meant to move to its own repository later, so it must not reference IDs of a board.
    The header of `core/base.yaml` lists the IDs that a device must supply and the `wake_word_start_hook` script that a
    device can extend; `controls.yaml` lists its own IDs and holds the button sounds. Koala uses all five core packages.
    Respeaker-XVF3800-ESPHome-integration uses all of them except `controls.yaml`: its `packages/base.yaml` includes them
    as a nested remote package at its `respeaker_lite_ref`. A core change reaches both repositories on their next bump.
  - `respeaker-lite/hardware.yaml` — the ReSpeaker Lite board: ESP32-S3, I2C, I2S, mics, speaker chain, AIC3204, XMOS
    (`respeaker_lite` component, mute, DFU), external components. Koala uses this package too.
  - `lite/device.yaml` — the XIAO device: flash size, user button pin, the single LED and every `control_leds*` script,
    the DFU LED automations, and `esphome.project`.
- `config/common/respeaker-satellite-base.yaml` — includes all packages. Existing user configs import this path, so keep it.
- `config/respeaker-satellite-dashboard-example.yaml` — the thin user config. It sets `respeaker_lite_ref` and imports
  the base package from `github://formatBCE/Respeaker-Lite-ESPHome-integration/...@${respeaker_lite_ref}`.
- `config/respeaker-satellite-factory.yaml` — factory configuration without credentials, the same as Koala's and
  XVF3800's: the base file, Improv over BLE (`improv_ble`, authorizer: `user_button`) and serial, BLE off while Wi-Fi is
  up, `name_add_mac_suffix`, and `dashboard_import` of the base file at `${respeaker_lite_ref}`. The `ota: http_request`
  and `update:` blocks stay commented out on purpose: the release workflow builds this file with ewt-gen, which adds
  them. Do not uncomment them in a commit.
- `esphome/components/respeaker_lite/` — the custom ESPHome external component (Python codegen plus C++).
- `esphome/components/decimator/` — the `decimator` microphone platform: 48 kHz XMOS output to 16 kHz, 1 frame of 3.
  Shared: the XVF3800 repository loads it from here at its `respeaker_lite_ref`, so a change reaches XVF on its next bump.
- `microwakeword/models/v2/` — the `kenobi` and `snooze` wake word models, served over raw GitHub URL at
  `${satellite_core_ref}`. `snooze` was trained by @dlarkinsPonderosa (XVF3800 #59/#60).
- `sounds/` — the twelve built-in sounds: Ogg Opus copies (48 kbps mono) of the Voice PE sounds, CC BY 4.0
  (`sounds/LICENSE.md` holds the attribution and the ffmpeg command). `core/voice-assistant.yaml` and `core/controls.yaml` get them at
  `${satellite_core_ref}`. Keep them Opus: both media player pipelines are `format: FLAC`, so no MP3 file means no MP3
  decoder in the build. A new MP3 or WAV sound adds that decoder back. Koala and XVF3800 get the sounds from this core.
- `respeaker_lite_i2s_dfu_firmware_48k_v1.1.0.bin` — the XMOS firmware that the device flashes to itself.
- `blueprints/automation/formatbce/` — Home Assistant automation blueprints, imported by raw GitHub URL.
- `readme/`, `casing/` — user documentation and 3D print files.

## Important: the source of truth is GitHub, not the working tree

`external_components` in `packages/respeaker-lite/hardware.yaml` pulls `respeaker_lite` and `decimator` from this
repository at `ref: ${respeaker_lite_ref}`. `i2s_audio` is the stock ESPHome component (the `formatBCE/esphome` fork is
gone): the 48 kHz `i2s_mics_raw` microphone feeds the `decimator` microphone `i2s_mics`, which keeps 1 frame of 3 with
no filter. This is exact because the XMOS firmware processes at 16 kHz, so the 48 kHz stream has no content above
8 kHz. Do not switch back to the stock `resampler` microphone: with esp-audio-libs 4.0.1 and 16 taps it cuts 4 kHz by
2 dB, 6 kHz by 8 dB and 7.5 kHz by 17 dB (two pre-filter biquads at 6.7 kHz, not configurable), which degrades the
wake word and STT (XVF3800 issue #69). The old fork did the same 1-of-3 decimation inside `i2s_audio`.
The XMOS firmware URL uses `${respeaker_lite_ref}`. The wake word model and sound URLs in
the core use `${satellite_core_ref}`, which follows `respeaker_lite_ref`. The default of `respeaker_lite_ref` is the current
release tag, not `main`.

To test package edits, include the local `config/common/respeaker-satellite-base.yaml` from a config outside the
repository. Relative includes resolve the same way locally and from the git cache.

To verify a refactor, run `esphome config` before and after, and diff the merged output. The output must not change.

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

The update starts only if the found version is at least `auto_update_min_version` (default `1.0.8`). Below that, the
component marks itself failed and does not flash, because the DFU of older XMOS firmware can erase its own factory
image (issue 94).

The `on_begin`, `on_end`, `on_error`, and `on_progress` automations only compile when at least one of them is present.
They set `USE_RESPEAKER_LITE_STATE_CALLBACK`.

### LED state machine

All LED output goes through the `control_leds` script. That script is a priority chain: a failed `respeaker_lite`
component, then ImprovBLE, then initialization,
then loss of the Wi-Fi or the API connection, then the button touch, then the timer or the alarm state, then the voice
assistant phase. Each branch calls a dedicated `control_leds_*` child script.

Never write to a light directly from an automation. Set the state that `control_leds` reads, then call
`script.execute: control_leds`. The voice assistant phase lives in the `voice_assistant_phase` global, and the phase
numbers come from the `substitutions:` block of `packages/core/base.yaml`.

`packages/core/controls.yaml` does not touch a light. It calls three hook scripts that the device supplies:
`control_leds_status_off`, `control_leds_factory_reset_coming_up`, and `control_leds_factory_reset_confirmed`.

### Timers, alarm, and snooze

`timer_ringing` (internal switch, `core/timers-alarm.yaml`) is the single ring path for HA timers and the alarm.
`on_turn_on` enables the `stop` model (and `snooze` if `snooze_enabled`), starts the repeating ring and the
`ring_timeout` script (15 min, `mode: restart`); `on_turn_off` stops that script and disables both models. Keep the
timeout in a script: a `delay` inside `on_turn_on` keeps running after turn-off (ESPHome runs re-triggered automation
delays in parallel) and cuts a later ring short.

"Snooze" is handled in the ringing branch of `on_wake_word_detected`, matched by the model's `wake_word` string.
`snooze_ring` stores an epoch deadline in the restored `snooze_until` global (0 = none) and turns the ring off. A 1 s
`interval` turns `timer_ringing` on at the deadline and drops a deadline that is more than 15 min old (the device was
off). Without a valid clock, "Snooze" is ignored and the ring continues. The `stop` model stays off during the snooze
window on purpose, so a false "Stop" cannot lose a wake-up. `Cancel snooze`, setting `alarm_time`, turning
`snooze_enabled` off, or turning `alarm_on` off clear the deadline. The `alarm_on` case is detected in that interval,
not in `alarm_on`'s triggers, because Koala replaces those triggers with `then:` blocks.

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

- `esphome.min_version` tracks the ESPHome release that the firmware needs, now `2026.10.0`. Keep `ESPHOME_VERSION` in `.github/workflows/release.yaml` at or above it.
- Releases use plain CalVer tags without a `v`, for example `2026.9.1`. To prepare a release, set the new version in
  four places: `esphome.project.version` in `packages/lite/device.yaml`, the `respeaker_lite_ref` default in
  `packages/respeaker-lite/hardware.yaml`, and `respeaker_lite_ref` in the example config and in the factory config.
  The default must equal the tag, because a pinned package then gets the component, the model, and the firmware from
  the same release.
- Push that commit to `main`. `.github/workflows/release.yaml` then works like XVF3800's: `prepare` reads the **first**
  `version:` line of `packages/lite/device.yaml`, checks that the tag does not exist and that the three
  `respeaker_lite_ref` values equal the version; `build` installs the pinned `ESPHOME_VERSION`, points the factory
  config's `respeaker_lite_ref` at the commit SHA (the tag does not exist yet), and runs the pinned `ewt-gen`, which
  compiles the firmware and whose `--import-url` pins adopted devices to the tag; `publish-pages` deploys the web
  installer and `manifest.json` to `https://formatbce.github.io/Respeaker-Lite-ESPHome-integration` (factory devices
  update from it); `release` creates the tag and the GitHub release with the binaries, only after Pages succeeded.
  Pages must use "GitHub Actions" as its source. A push without a new version does nothing. Do not tag by hand.
- Koala and XVF3800 releases need the Lite tag of their `respeaker_lite_ref`, so a failed Lite release blocks them. The
  tag is created last, so a manual run (`workflow_dispatch`) of the failed version can finish it.
- Blueprint files and model files are referenced by absolute raw GitHub URL. A rename breaks live installations.
- New scripts that can overlap use `mode: restart`. See commit 3136cf7.
- The `disable_buttons` switch guards every button automation. Add the same guard to a new button action.
- Package order matters. A later package overrides a scalar of an earlier one, and lists join in package order.
- A package extends an item of another package with `id: !extend <id>`. ESPHome resolves it after it merges all
  packages, so the order of the two packages does not matter.
