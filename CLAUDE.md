# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repository is

This repository holds the ESPHome firmware of three Home Assistant voice satellites of the ReSpeaker family:

| Device | Hardware | Device packages |
| --- | --- | --- |
| Respeaker Lite | Seeed ReSpeaker Lite Voice Kit: XIAO ESP32-S3 plus an XMOS XU316 audio front end | `respeaker-lite/`, `lite/` |
| Respeaker XVF3800 | Seeed ReSpeaker XVF3800 USB 4-Mic Array: XIAO ESP32-S3, XMOS XVF3800 DSP, TLV320AIC3104 codec | `xvf3800/` |
| Koala Satellite | An ESP32-S3 DevKitC and the ReSpeaker Lite board, with an LED ring and a dial | `respeaker-lite/`, `koala/` |

All three use one board-independent voice satellite core (`packages/core/`) and one release. It is not an application
with a build system. It is YAML packages plus custom ESPHome components. Home Assistant users consume it as a remote
package.

The XVF3800 and Koala firmware came from the repositories `Respeaker-XVF3800-ESPHome-integration` and `Koala-Satellite`.
Those repositories are frozen. Their old tags still serve old user configurations.

There is no test suite and no linter configuration. The only CI workflow is `.github/workflows/release.yaml`, which builds
and publishes releases.

## Commands

Compile and upload need the ESPHome CLI and a device config that includes the packages. The factory configs need no
secrets:

```bash
esphome config   config/respeaker-satellite-factory.yaml       # Lite: resolve packages, validate, print merged YAML
esphome config   config/respeaker-xvf-satellite-factory.yaml   # XVF3800
esphome config   config/koala-satellite.yaml                   # Koala
esphome compile  <config>                                      # build only
esphome run      <config>                                      # build, flash, then attach logs
esphome logs     <config>                                      # attach to a running device
```

The user examples (`config/respeaker-satellite-dashboard-example.yaml`, `config/respeaker-xvf-satellite-example.yaml`,
`config/koala-dashboard.yaml`) need secrets in a `secrets.yaml` file beside them. `config/secrets.yaml.example` lists them.

`esphome config` is the fastest check for a YAML change. It downloads every remote model file and the XMOS firmware
binaries, so it also proves that all URLs and the MD5 checksums are correct.

## Layout

- `packages/` — the device definitions, split into packages:
  - `core/` — the board-independent voice satellite: `base.yaml` (boot, network, API, phase globals, the
    `respeaker_lite_ref` default), `voice-assistant.yaml` (wake word, pipeline, sounds, HA events), `media-player.yaml`,
    `timers-alarm.yaml`, and `controls.yaml` (the Voice PE button behavior). It must not reference IDs of a board.
    The header of `core/base.yaml` lists the IDs that a device must supply and the `wake_word_start_hook` script that a
    device can extend; `controls.yaml` lists its own IDs and holds the button sounds. Lite and Koala use all five core
    packages. XVF3800 uses all of them except `controls.yaml`, because the XMOS chip owns its buttons.
  - `respeaker-lite/hardware.yaml` — the ReSpeaker Lite board: ESP32-S3, I2C, I2S, mics, speaker chain, AIC3204, XMOS
    (`respeaker_lite` component, mute, DFU), external components. Lite and Koala use it.
  - `lite/device.yaml` — the XIAO device: flash size, user button pin, the single LED and every `control_leds*` script,
    the DFU LED automations, and `esphome.project`. The release workflow reads the version from this file.
  - `xvf3800/` — `base.yaml` (identity, esp32, psram, i2c, Wi-Fi tuning, `set_led_color`, external components),
    `hardware.yaml` (I2S, speaker, DSP, codec, DFU), `voice-assistant.yaml` (beam lock, `voice_direction`, volume range),
    `leds.yaml` (all LED effect scripts), `timers-alarm.yaml` (the timer-tick LED animation only).
  - `koala/device.yaml` — the Koala hardware: pins as substitutions, the LED ring and all its effects, the dial, the
    onboard LED, and `esphome.project`.
- `config/common/` — one file per device that includes the packages in order: `respeaker-satellite-base.yaml`,
  `respeaker-xvf-satellite-base.yaml`, `koala-base.yaml`. User configs and adopted devices import these paths, so keep
  them. The core comes first, so a value in a device package wins.
- `config/` — per device a thin user example and a factory configuration without credentials (Improv over BLE and
  serial, BLE off while Wi-Fi is up, `dashboard_import` of the common file at `${respeaker_lite_ref}`). The Improv
  authorizer is `user_button` on Lite and Koala and `mute_button` on XVF3800. The `ota: http_request` and `update:`
  blocks stay commented out on purpose: the release workflow builds these files with ewt-gen, which adds them. Do not
  uncomment them in a commit.
- `esphome/components/` — `respeaker_lite` (XMOS control of the Lite board), `respeaker_xvf3800` (XMOS DSP hub),
  `aic3104` (codec), and `decimator` (the microphone platform: 48 kHz XMOS output to 16 kHz, 1 frame of 3; all devices).
- `microwakeword/models/v2/` — the `kenobi` and `snooze` wake word models, served over raw GitHub URL at
  `${satellite_core_ref}`. `snooze` was trained by @dlarkinsPonderosa (XVF3800 repository #59/#60).
- `sounds/` — the twelve built-in sounds: Ogg Opus copies (48 kbps mono) of the Voice PE sounds, CC BY 4.0
  (`sounds/LICENSE.md` holds the attribution and the ffmpeg command). `core/voice-assistant.yaml` and `core/controls.yaml`
  get them at `${satellite_core_ref}`. Keep them Opus: both media player pipelines are `format: FLAC`, so no MP3 file
  means no MP3 decoder in the build. A new MP3 or WAV sound adds that decoder back.
- `respeaker_lite_i2s_dfu_firmware_48k_v1.1.0.bin`, `respeaker_xvf3800_i2s_master_v1.0.9_48k.bin` — the XMOS firmware
  that each device flashes to itself.
- `blueprints/automation/formatbce/` — Home Assistant automation blueprints, imported by raw GitHub URL.
- `readme/` — user documentation of the shared features. `readme/<device>/` and `casing/<device>/` — the documentation
  and the 3D print files of one device (`lite`, `xvf3800`, `koala`).
- `site/index.html` — the device list page at the root of the GitHub Pages site.

## Important: the source of truth is GitHub, not the working tree

`external_components` in `packages/respeaker-lite/hardware.yaml` and in `packages/xvf3800/base.yaml` pull the components
from this repository at `ref: ${respeaker_lite_ref}`. `i2s_audio` is the stock ESPHome component (the `formatBCE/esphome`
fork is gone): the 48 kHz `i2s_mics_raw` microphone feeds the `decimator` microphone `i2s_mics`, which keeps 1 frame of
3 with no filter. This is exact because the XMOS firmware processes at 16 kHz, so the 48 kHz stream has no content above
8 kHz. Do not switch back to the stock `resampler` microphone: with esp-audio-libs 4.0.1 and 16 taps it cuts 4 kHz by
2 dB, 6 kHz by 8 dB and 7.5 kHz by 17 dB (two pre-filter biquads at 6.7 kHz, not configurable), which degrades the
wake word and STT (XVF3800 repository issue #69).

The XMOS firmware URLs use `${respeaker_lite_ref}`. The wake word model and sound URLs in the core use
`${satellite_core_ref}`, an alias of `respeaker_lite_ref`. The default of `respeaker_lite_ref` (in
`packages/core/base.yaml`) is the current release tag, not `main`.

To test package edits, include the local `config/common/<device>-base.yaml` file from a config outside the repository.
The common files use relative includes only, so the core and the device packages come from the working tree.

To verify a refactor, run `esphome config` before and after, and diff the merged output. The output must not change.

A local edit to `esphome/components/` has no effect on a build. To test a component change, point the source at a local
path first:

```yaml
external_components:
  - source: { type: local, path: esphome/components }
    components: [respeaker_lite, decimator]   # XVF3800: [respeaker_xvf3800, aic3104, decimator]
```

Do not commit that change. The same applies to the model and firmware URLs.

## Architecture

### Two processors, one I2C bus (Lite and Koala)

The ESP32-S3 runs ESPHome. The XMOS board does the microphone array processing, the acoustic echo cancellation, and the
hardware mute. The `respeaker_lite` component talks to the XMOS board over I2C at address `0x42` on bus `internal_i2c`
(SDA GPIO5, SCL GPIO6). Audio does not use that bus. Audio uses I2S, where the ESP32 is the **secondary** and the XMOS
board drives the clocks, at 48 kHz and 32 bit.

The component exposes three things over I2C:
- The XMOS firmware version, as a text sensor.
- The hardware mute state, as an internal binary sensor.
- A DFU client that flashes the XMOS board.

### Automatic XMOS firmware update (Lite and Koala)

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

All LED output goes through the `control_leds` script of the device. That script is a priority chain: a failed XMOS
component, then ImprovBLE, then initialization, then loss of the Wi-Fi or the API connection, then the button touch,
then the timer or the alarm state, then the voice assistant phase. Each branch calls a dedicated `control_leds_*` child
script.

Never write to a light directly from an automation. Set the state that `control_leds` reads, then call
`script.execute: control_leds`. The voice assistant phase lives in the `voice_assistant_phase` global, and the phase
numbers come from the `substitutions:` block of `packages/core/base.yaml`.

`packages/core/controls.yaml` does not touch a light. It calls three hook scripts that Lite and Koala supply:
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
stereo `i2s_mics` microphone with `gain_factor: 4` (XVF3800: gain 1). The `stop` model is internal and the
`activate_stop_word_once` script enables it only while the assistant speaks.

### Home Assistant interface

The device sends four custom events to Home Assistant: `esphome.tts_uri`, `esphome.stt_text`, plus the wake word and the
alarm events. Corresponding scripts named `send_*_event` fire them. It exposes exactly three API actions: `start_va`,
`stop_va`, and `set_time_zone` (XVF3800 adds `set_led_color`). The alarm time is set through the `alarm_time` datetime
entity, not through an action. `readme/alarms.md` still tells the user to call an `esphome.{device}_set_alarm_time`
action, and that action no longer exists.

Time zone and alarm settings persist in `restore_value: yes` globals. The `on_boot` handler at priority -100 applies them
again with `setenv("TZ", ...)`.

### XVF3800: hub and children

`RespeakerXVF3800` (`respeaker_xvf3800.h/.cpp`) is an `i2c::I2CDevice` hub at address `0x2C`. It owns three optional
`PollingComponent` children — `MuteSwitch`, `DFUVersionTextSensor`, `LEDBeamSensor` — each of which holds a back-pointer
set in `to_code()` and does nothing but call a hub method in `update()`. Add new entities the same way: declare the class
in `__init__.py`, add an optional schema key, create it and wire both directions in `to_code()`. The `aic3104` codec is
at address `0x18`.

All DSP access is `(resid, cmd, payload)` over I2C, mirroring Respeaker's `xvf_host.py`. Reads set the high bit (`0x80`)
on the command byte, then read `1 + n` bytes where byte 0 is a `TransportProtocolReturnCode`. Only `CTRL_DONE` means the
payload is valid; `CTRL_WAIT` and `SERVICER_COMMAND_RETRY` (`0x40`) are normal transient answers and must be retried, not
logged as errors. Take new resource IDs and command numbers from `xvf_host.py` and record the source in a comment.

### XVF3800: beam direction and beam lock

The DSP reports four azimuth slots (radians) from `AEC_SERVICER_RESID`/cmd 75: beam 1, beam 2, free-running,
auto-select. `read_led_beam_direction()` normally reads slot 3 and maps it to one of 12 LED positions. `lock_beam()`
captures the current azimuth, writes it to the fixed-beam registers, enables fixed-beam mode, and makes later reads
follow slot 0. `packages/xvf3800/voice-assistant.yaml` calls `lock_beam()` from the core's `wake_word_start_hook` (if
the `Beam lock` switch is on) and `unlock_beam()` on STT VAD end and when the pipeline ends. `get_azimuth_degrees()`
returns the cached azimuth with no bus access; the hook publishes it once per wake word to the disabled-by-default
`voice_direction` sensor. Never stream it from the 10 Hz poll.

### XVF3800: DFU firmware update

The same embed-at-build pattern as Lite: `can_proceed()` blocks startup until the on-chip version is read and matches,
or the component fails; on a mismatch `setup()` starts the update itself (`start_dfu_update()`, also the
`respeaker_xvf3800.flash` action), a non-blocking state machine in `loop()` that sends 128-byte blocks.
`packages/xvf3800/hardware.yaml` reports the result in the `dsp_status` text sensor (`OK`, `Update failed`,
`No response`). The triggers compile only when `USE_RESPEAKER_XVF3800_STATE_CALLBACK` is defined. To change the
firmware, update the binary, the `version`, and the `md5` together.

### XVF3800: LED ring and mute button

The ring is 12 RGB LEDs written as one 12-entry `uint32_t` array through `set_led_ring()`. All animation is YAML in
`packages/xvf3800/leds.yaml`: a 50 ms `interval` dispatches on the `current_led_effect` global to one of the
`update_*_effect` scripts. `set_led_ring()` uses cmd 19 from XMOS firmware 1.0.8 on, cmd 18 before.

The `LED Ring` light (`platform: rgb`) does not write the LEDs; it drives the animation engine. Its three template
outputs are stubs on purpose. The light's `on_state` is the **single writer** of the `user_led_ring_*`, `user_idle_effect`,
and `user_idle_speed` globals; the preset select and the `set_led_color` action call `id(led_ring_light).make_call()`. The
light owns the idle branch of `control_leds` only. A light change sets `led_force_update` to demand a repaint.

The DSP keeps its built-in mute action. `MuteButtonBinarySensor` (`mute_button`, 50 ms poll) only reads the button
(`GPI_VALUE_ALL`, resid 36, cmd 6, bit 0, low while pressed on firmware 1.0.9). `packages/xvf3800/hardware.yaml` uses it
for the Voice PE factory reset hold. Do not set `MUTE_FUNCTION_ENABLE` to 0: the hardware mute must keep working when
the ESP misbehaves.

### Koala device

- Board pins are substitutions at the top of `packages/koala/device.yaml`, with DevKitC defaults (`user_button_pin`,
  `led_ring_pin`, `dial_a_pin`, `dial_b_pin`, `dial_button_pin`, `esp_onboard_led_pin`). Do not hardcode a board pin.
  `dial_button_mutes_speaker` (default `"false"`) enables the dial press action, because `GPIO20` on the DevKitC is a
  noisy USB pin.
- `alarm_on` in the device package uses the `then:` trigger form on purpose. A map replaces the core trigger list. A list
  would add to it.
- The device package extends `alarm_on` and `external_media_player` with `id: !extend`, and it supplies the three
  `controls.yaml` hooks.
- The Koala firmware is a fork of the Voice PE firmware. Keep the behavior aligned with Voice PE where you can.

## Conventions

- `esphome.min_version` tracks the ESPHome release that the firmware needs, now `2026.10.0`. Keep `ESPHOME_VERSION` in
  `.github/workflows/release.yaml` at or above it.
- Releases use plain CalVer tags without a `v`, for example `2026.9.1`. One tag serves all devices. To prepare a
  release, set the new version in ten places:
  - the `respeaker_lite_ref` default in `packages/core/base.yaml`;
  - `esphome.project.version` in `packages/lite/device.yaml`, `packages/xvf3800/base.yaml`, `packages/koala/device.yaml`;
  - `respeaker_lite_ref` in the three user examples and the three factory configs in `config/`.

  The default must equal the tag, because a pinned package then gets the components, the models, and the firmware from
  the same release.
- Push that commit to `main`. `.github/workflows/release.yaml` then runs: `prepare` reads the **first** `version:` line of
  `packages/lite/device.yaml`, checks that the tag does not exist and that all ten places equal the version; `build` is a
  matrix over `lite`, `xvf3800`, `koala`, installs the pinned `ESPHOME_VERSION`, points the factory config's
  `respeaker_lite_ref` at the commit SHA (the tag does not exist yet), runs the pinned `ewt-gen` with
  `--publish-url https://formatbce.github.io/Respeaker-ESPHome-integration/<device>`, and checks that the generated
  `update:` source is that device's manifest; `publish-pages` deploys the three sites in their subpaths plus
  `site/index.html`; `release` creates the tag and the GitHub release with the binaries (prefixed with the device), only
  after Pages succeeded. Pages must use "GitHub Actions" as its source. A push without a new version does nothing. Do not
  tag by hand. The tag is created last, so a manual run (`workflow_dispatch`) of a failed version can finish it.
- Factory devices update from `https://formatbce.github.io/Respeaker-ESPHome-integration/<device>/manifest.json`. Never
  change a device's Pages path. Devices flashed before the move read the old manifest URLs; frozen bridge copies in the
  `formatbce.github.io` user site repository serve them.
- This repository was named `Respeaker-Lite-ESPHome-integration`. GitHub redirects git and raw URLs of that name, and
  old XVF3800 and Koala tags load the core through that redirect. Never create a repository with the old name.
- Blueprint files and model files are referenced by absolute raw GitHub URL. A rename breaks live installations.
- New scripts that can overlap use `mode: restart`. See commit 3136cf7.
- The `disable_buttons` switch guards every button automation. Add the same guard to a new button action.
- Package order matters. A later package overrides a scalar of an earlier one, and lists join in package order.
- A package extends an item of another package with `id: !extend <id>`. ESPHome resolves it after it merges all
  packages, so the order of the two packages does not matter.
