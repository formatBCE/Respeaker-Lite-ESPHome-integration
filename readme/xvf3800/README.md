# reSpeaker XVF3800 — ESPHome voice satellite

ESPHome components and configuration packages that make a
[reSpeaker XVF3800 USB 4-Mic Array](https://github.com/respeaker/reSpeaker_XVF3800_USB_4MIC_ARRAY)
into a Home Assistant voice satellite.

The board has three parts. This project drives all of them:

| Part | Function | Component |
| --- | --- | --- |
| ESP32-S3 | Host. Runs ESPHome. | ESPHome core |
| XMOS XVF3800 | Audio DSP. Beam forming, mute, LED ring. | `respeaker_xvf3800` |
| TLV320AIC3104 | Audio codec. | `aic3104` |

> **Warning:** This project is under development. Use it at your own risk.
> Test reports are welcome.

---

## What you get

The device gives these entities to Home Assistant:

| Entity | Type | Function |
| --- | --- | --- |
| Microphone Mute | switch | Mutes the microphone array in the DSP |
| Mute-unmute sound | switch | Plays a sound when you mute the microphone |
| Wake sound | switch | Plays a sound when the wake word starts the pipeline |
| Beam lock | switch | Locks the microphone beam during one utterance |
| Alarm on | switch | Turns the alarm clock on |
| Alarm time | datetime | Sets the alarm time |
| Alarm action | select | Selects what the alarm does |
| Snooze enabled | switch | Lets you say "Snooze" to a ringing timer or alarm |
| Snooze duration | number | Sets the time until the ring starts again (1 to 60 minutes) |
| Snooze pending | binary sensor | Shows that a snoozed ring waits to start again |
| Cancel snooze | button | Cancels a snoozed ring |
| Wake word sensitivity | select | Sets the wake word threshold |
| DSP route | select | Chooses what each DSP output channel feeds |
| ASR output gain | number | Gain on the ASR output (wake word; STT on the ASR route) |
| AGC max gain (comms) | number | Maximum AGC gain of the comms output (STT on the default route) |
| LED Ring Color Preset | select | Selects the LED ring colour |
| LED Ring | light | Sets the LED ring colour, the brightness and the idle effect |
| Firmware Version | text sensor | Shows the XMOS DSP firmware version |
| DSP Status | text sensor | Shows the result of the DSP firmware check at boot |
| Current device time | text sensor | Shows the device clock |
| Voice Direction | sensor | Shows the direction of the last wake word in degrees (disabled by default) |
| Next timer, Next timer name | sensor, text sensor | Show the first active timer |
| Media Player | media player | Plays announcements and media |
| Restart, Factory Reset | button | Restart and factory reset |

The **LED Ring** light controls the 12-LED ring. Read
[LED effects](#led-effects) for more information.

---

## Requirements

- ESPHome **2026.10.0** or later.
- Home Assistant with the ESPHome integration.
- A reSpeaker XVF3800 USB 4-Mic Array board.
- A USB-C cable and a computer that can flash the board.

The microphones run at 48 kHz, the same clock as the speaker. The
`decimator` microphone platform converts them to 16 kHz for the voice
assistant and the wake word. It keeps 1 frame of 3 and does not filter the
audio. The DSP processes the audio at 16 kHz, so the 48 kHz stream has no
content above 8 kHz. This needs ESPHome 2026.10.0 or later.

---

## Install

### 1. Make the secrets file

Copy `config/secrets.yaml.example` to `config/secrets.yaml`. Set your own values.

```yaml
wifi_ssid: "YOUR_WIFI_NAME"
wifi_password: "YOUR_WIFI_PASSWORD"
ota_password: "YOUR_OTA_PASSWORD"
api_key: "YOUR_BASE64_API_ENCRYPTION_KEY"
```

Make a new API key at the [ESPHome API page](https://esphome.io/components/api.html).
Keep `secrets.yaml` out of version control.

### 2. Make the device configuration

Create a file for your device. The packages give all the other configuration.

```yaml
substitutions:
  name: respeaker-xvf3800-assistant
  friendly_name: reSpeaker XVF3800 Assistant
  respeaker_ref: 2026.11.0

packages:
  respeaker:
    url: https://github.com/formatBCE/Respeaker-XVF3800-ESPHome-integration
    ref: ${respeaker_ref}
    refresh: 1d
    files:
      - packages/base.yaml
      - packages/hardware.yaml
      - packages/voice-assistant.yaml
      - packages/leds.yaml
      - packages/timers-alarm.yaml

# A remote package cannot read secrets, so the credentials go here.
# ESPHome joins these blocks with the blocks in the packages.
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

api:
  encryption:
    key: !secret api_key

ota:
  - platform: esphome
    id: ota_esphome
    password: !secret ota_password
```

`config/respeaker-xvf-satellite-example.yaml` holds this same configuration.

### 3. Build and flash

Connect the board with USB. Then run:

```bash
esphome run config/respeaker-xvf-satellite-example.yaml
```

The first build takes several minutes, because ESPHome compiles the ESP-IDF
framework. Later builds are much faster.

### 4. Add the device to Home Assistant

Home Assistant finds the device automatically. Give it the API key from your
`secrets.yaml` file.

### Install with the web installer

Use the web installer if you do not want to build the firmware. The firmware has
no credentials. The device gets the Wi-Fi credentials through Improv over USB
serial or over BLE.

1. Open the [web installer](https://formatbce.github.io/Respeaker-XVF3800-ESPHome-integration/)
   in Chrome or Edge on a computer.
2. Connect the computer to the USB-C port of the XIAO ESP32-S3 board.
3. Click **Connect**, and select the board.
4. Install the firmware.
5. Set the Wi-Fi credentials in the installer.

As an alternative to step 5, add the device in Home Assistant with the
[Improv via BLE](https://www.home-assistant.io/integrations/improv_ble/)
integration. When Home Assistant asks for authorization, push the **Mute**
button. The button also mutes the microphones. Push the button again to unmute
them.

After the Wi-Fi connection, the device stops BLE. If the device loses the Wi-Fi
connection, BLE starts again.

Home Assistant shows a firmware update when a new release is available. To
change the configuration, adopt the device in the ESPHome dashboard. The
dashboard imports `config/common/respeaker-xvf-satellite-base.yaml` at the
release tag of the device.

To build the same firmware yourself, run
`esphome run config/respeaker-xvf-satellite-factory.yaml`. This build does not
get updates from Home Assistant.

---

## Configuration

Set any of these substitutions in your own configuration. Your value always
wins, because a substitution in the main configuration overrides the same
substitution in a package.

| Substitution | Default | Function |
| --- | --- | --- |
| `name` | `respeaker-xvf3800-assistant` | ESPHome node name |
| `friendly_name` | `reSpeaker XVF3800 Assistant` | Display name |
| `respeaker_ref` | the current release, for example `2026.9.1` | Release for the packages, the components, and the firmware |
| `respeaker_lite_ref` | a Respeaker Lite release, set by each release | Release of the voice satellite core. Read [Package layout](#package-layout) |
| `hidden_ssid` | `"false"` | Set this to `"true"` for a hidden network |
| `log_level` | `INFO` | Logger level |
| `voice_assist_*_phase_id` | `1` to `11` | Phase constants for the LED ring |
| `*_sound_file` | the sounds of the voice satellite core | URL of each notification sound |

**Caution:** All substitutions share one namespace. A name that you set in your
configuration replaces that name in every package.

---

## Change the configuration

The packages merge into your configuration. They do not replace it.

**Change a setting.** Set the substitution:

```yaml
substitutions:
  log_level: DEBUG
```

**Add an entity.** Declare it. Lists join together:

```yaml
sensor:
  - platform: uptime
    name: "Uptime"
```

**Add steps to a script.** Use `!extend`. The new steps go after the existing
steps:

```yaml
script:
  - id: !extend control_leds
    then:
      - logger.log: "LEDs updated"
```

**Delete an entity.** Use `!remove`:

```yaml
button:
  - id: !remove restart_button
```

**Delete a whole component.** Use `!remove` on the key:

```yaml
debug: !remove
```

**Replace a script.** Delete the old script. Then add a new script with a
**different** id. You cannot delete an id and add the same id in one
configuration.

**Caution:** Do not delete a whole package file. Read
[Package layout](#package-layout) first.

---

## Snooze

When a timer or the alarm rings, say "Snooze". You do not say the wake word
first. The ring stops, and starts again after the **Snooze duration** (9 minutes
by default). You can snooze again each time the ring starts again. To stop the
ring, say "Stop" or the wake word.

The device does not listen for "Stop" while it waits to ring again, so a false
"Stop" cannot cancel your alarm. To cancel a snooze, press **Cancel snooze**, or
turn off **Alarm on**.

The device keeps the snooze if it restarts. If the device was off for more than
15 minutes after the ring was due, the device does not ring.

Snooze needs the clock of the device. If the clock is not set, the device
ignores "Snooze" and continues to ring. The "Snooze" model knows English only.

---

## Mute button

When you push the **Mute** button, the DSP mutes or unmutes the microphones.
The DSP does this itself, so the button works also if the ESP32-S3 stops.

To reset the device to the factory settings:

1. Push and hold the **Mute** button for 10 seconds. The device plays a warning
   sound, and the LED ring pulses orange. The mute state goes back to the state
   before the hold.
2. Hold the button until the device plays a second sound and the LED ring
   becomes red, at about 22 seconds.
3. Release the button. The device erases its settings and restarts.

To cancel the reset, release the button before the second sound. The device
then plays a cancel sound, and the LED ring shows the usual state again.

The reset erases the saved settings and the Wi-Fi credentials from Improv. A
configuration with Wi-Fi credentials in YAML connects again with these
credentials.

---

## Releases and updates

Each release has a tag, for example `2026.9.1`. The
[tags page](https://github.com/formatBCE/Respeaker-XVF3800-ESPHome-integration/tags)
shows all releases. The `respeaker_ref` substitution in your configuration
selects the release.

One substitution controls three things: the YAML packages, the C++ components,
and the DSP firmware. So the YAML and the firmware always match. Use
`${respeaker_ref}` also as the `ref:` of the `packages:` block. If you pin only
the `packages:` block, ESPHome takes the YAML from your tag but the C++ from
another ref. That mismatch is difficult to find.

Each release also sets `respeaker_lite_ref` to a release of the
[Respeaker Lite integration](https://github.com/formatBCE/Respeaker-Lite-ESPHome-integration).
That release supplies the voice satellite core. Keep the default value. The
release workflow checks that the two releases work together.

To update the device:
1. Set `respeaker_ref` to the tag of the new release.
2. Install the configuration on the device again.

Use `main` only to test new code. A change in `main` can break the build or the
device. To get the newest code before a release, set `respeaker_ref` to `main`.

**Note:** `esp32.framework.version` is `recommended`, so a new ESPHome release
can still change the ESP-IDF version. A pinned `respeaker_ref` does not give a
fully reproducible build.

---

## Package layout

The voice satellite core comes from the `packages/core/` directory of the
[Respeaker Lite integration](https://github.com/formatBCE/Respeaker-Lite-ESPHome-integration)
at `respeaker_lite_ref`. The core holds the boot and connection logic, the
wake word, the voice pipeline, the media player, the timers, the alarm, and the
sounds. Koala Satellite and the Respeaker Lite use the same core. The
`base.yaml` package includes the core, so your configuration does not change.

| Package | Contents |
| --- | --- |
| `base.yaml` | The voice satellite core, `esphome`, `esp32`, `psram`, the logger level, the Wi-Fi tuning, the `set_led_color` action, `i2c`, `external_components` |
| `hardware.yaml` | `i2s_audio`, `microphone`, `speaker`, `audio_dac`, `respeaker_xvf3800` with the DFU firmware |
| `voice-assistant.yaml` | The beam lock, the voice direction sensor, the wake word gain, the volume range and the volume display of the media player |
| `leds.yaml` | LED globals, the animation interval, the effect scripts, the colour preset |
| `timers-alarm.yaml` | The timer animation of the LED ring |

**Caution:** Include all five packages. They are separate files for maintenance,
but they are not independent. The `leds.yaml` package reads entities from the
core and from `hardware.yaml`. The `control_leds` script runs from `wifi`,
`api`, `on_boot`, and 12 voice assistant triggers. If you remove one file, the
build fails.

To change one part, use `!extend` or `!remove`. Do not delete a package.

---

## First boot and firmware update

The `respeaker_xvf3800` component holds a copy of the XMOS DSP firmware. At boot
the component reads the version from the chip. ESPHome waits at startup until
this read is complete.

**CAUTION: Do not disconnect the power during a DSP firmware update. An
interrupted update can stop the DSP.**

If the version on the chip is different from the version in the build, the
component updates the chip automatically. It sends the firmware in 128-byte
blocks. The device does not connect to the network until the update is
complete. The `on_begin`, `on_progress`, `on_end`, and `on_error` triggers
report the progress.

### Check the DSP status

The **DSP Status** sensor shows the result of the firmware check at boot.

| Value | Meaning |
| --- | --- |
| `OK` | The DSP firmware agrees with the build. |
| `Update failed` | The firmware update did not complete. |
| `No response` | The DSP does not answer on I2C. |

If the value is `Update failed`, restart the device. The component then tries
the update again. If the value stays `Update failed`, or if the value is
`No response`, flash the DSP firmware over USB.

### Flash the DSP firmware over USB

Some older boards have the Seeed USB firmware. This firmware has no I2S audio
output. Flash the Seeed I2S firmware over USB one time. After that, the device
installs the firmware from the build.

1. Install `dfu-util`. Read the [Seeed DFU guide](https://github.com/respeaker/reSpeaker_XVF3800_USB_4MIC_ARRAY/blob/master/xmos_firmwares/dfu_guide.md)
   for the steps on Windows, macOS, and Linux.
2. Download the [Seeed firmware repository](https://github.com/respeaker/reSpeaker_XVF3800_USB_4MIC_ARRAY)
   as a ZIP file. Do not save one file from the browser, because the file can
   become corrupt.
3. In `xmos_firmwares/i2s`, find the file `respeaker_xvf3800_i2s_master_v<version>_48k.bin`.
   Use the 48 kHz master firmware only.
4. Connect the computer to the XMOS USB-C port. This port is nearer to the
   3.5 mm jack.
5. Run `dfu-util -l`. Make sure that the list shows `reSpeaker DFU Upgrade`.
6. Run `dfu-util -R -e -a 1 -D <path to the firmware file>`.
7. Disconnect the USB cable. Then connect the power to the board again.

At the next boot, the component installs the firmware version from the build.
Make sure that the **DSP Status** sensor shows `OK`.

If `dfu-util -l` does not find the board, start the DSP in Safe Mode:

1. Disconnect the power.
2. Push and hold the **Mute** button.
3. Connect the power, and keep the button pushed.
4. Release the button when the red LED flashes.
5. Do steps 5 to 7 again.

To change the firmware version, change three things together:

1. The `.bin` file in the repository root.
2. The `version` value in `packages/hardware.yaml`.
3. The `md5` value in `packages/hardware.yaml`.

The build fails if the MD5 sum does not agree with the file.

---

## Audio

The XMOS DSP outputs two processed channels over I2S. ESPHome reads one for the
voice assistant (speech to text) and one for `micro_wake_word`. Both taps are
processed, but differently: the **comms** tap is the auto-select beam with noise
suppression and adaptive AGC, while the **ASR** tap is tuned for speech
recognition with a fixed gain. The **DSP route** select sets what each channel
carries:

| Option | STT channel | Wake word channel |
| --- | --- | --- |
| Comms to STT, ASR to wake word (default) | Comms output | ASR output |
| ASR to STT and wake word | ASR output | ASR output |

By default the stock route is kept: the **comms** output — the auto-select beam
with noise suppression and adaptive AGC — feeds STT, and the **ASR** output feeds
wake word. Selecting **ASR to STT and wake word** routes both consumers to the
ASR output instead; use it when far-field STT needs the recognition-tuned tap.

The two gain numbers tune different taps:

- **ASR output gain** (default 1.0) sets the level of the ASR output. It always
  affects the wake word, and affects STT only when the route sends the ASR
  output to STT.
- **AGC max gain (comms)** (default 32) sets the maximum gain of the comms path's
  AGC. The comms output feeds STT on the default route, so it raises STT there;
  it does not affect the ASR output.

Raise them until far-field speech improves, but stop before close speech clips.

The XMOS resets these parameters on power-up. The `respeaker_xvf3800` component
applies the route and the two gains after the version read, and applies a change
as soon as you set it. Home Assistant saves the route and both numbers and
restores them at the next boot.

---

## Home Assistant setup

### LED colour script

Import `ha/change_color_script.yaml` as a Home Assistant script. The script
selects the device and sets the LED ring colour.

### API actions

The device gives four actions to Home Assistant:

| Action | Parameters | Function |
| --- | --- | --- |
| `set_led_color` | `red`, `green`, `blue` | Sets the LED ring colour |
| `start_va` | none | Starts the voice pipeline |
| `stop_va` | none | Stops the voice pipeline |
| `set_time_zone` | `posix_time_zone` | Sets the device time zone |

Call `set_time_zone` one time after you add the device. The device saves the
value and restores it at the next boot.

### Events

The device sends three events to Home Assistant. Use them as automation
triggers.

| Event | Data | Sent when |
| --- | --- | --- |
| `esphome.wake_word_detected` | `wake_word` | The device detects a wake word |
| `esphome.stt_text` | `text` | Speech to text gives a result |
| `esphome.tts_uri` | `uri` | Text to speech gives a response URL |

### Play the response on an external speaker

You can trigger an automation on `esphome.tts_uri` and send the `uri` to another
speaker, for example a Sonos. This works, but you must know three limits first.

Set the conversation agent to ask no follow-up questions. An external speaker
gives no echo signal to the XVF3800 canceller. If the agent sets
`continue_conversation`, the microphone stays open, and the device can hear its
own response as a new command.

The local media player still gets the same URL. It downloads the audio and it
decodes the audio a second time. To stop this path, add
`voice_assistant: media_player: !remove` to your configuration. To hide the
entity from Home Assistant, set `internal: true` on `external_media_player`.

The stop word gets its timing from the **local** announcement state. So the stop
word can start late, or stop early, when the audio plays on an external speaker.

---

## LED effects

The ring has 12 RGB LEDs. The DSP controls them over I2C. The driver writes the
ring as one 12-entry array.

All animation is in YAML, in `packages/leds.yaml`:

1. A 50 ms interval reads the `current_led_effect` global.
2. The interval calls one `update_*_effect` script.
3. The `control_leds` script decides which effect the device state needs.

To add an effect, add a script and add a branch in the interval. Do not change
the C++ component.

The `update_rainbow_effect` script works, but no `control_leds_*` script selects
it. Call `led_set_effect` to use it.

### The LED Ring light

The **LED Ring** light is an ESPHome `rgb` light. The ring is not an addressable
output, so the light does not write the LEDs. The light writes the animation
engine instead.

The light owns the idle look. If you turn the light on, the ring shows the light
colour and the light effect while the satellite waits for the wake word. If you
turn the light off, the ring stays dark. The voice, timer and error animations
always come first.

The light is the one authority for the ring colour and the ring level:

- The light colour is also the accent colour of every animation.
- The light brightness is the master level of every animation. The level stays
  the same when you turn the light off, so the voice animations keep the last
  level.
- The `LED Ring Color Preset` select and the `set_led_color` action write the
  light. The light then writes the colour globals.

The light gives these effects to Home Assistant:

| Effect | Animation |
| --- | --- |
| None | A steady ring |
| Breathe | The ring fades in and out |
| Twinkle | Single LEDs fade in and out |
| Comet CW | A comet turns clockwise |
| Comet CCW | A comet turns counterclockwise |

The light keeps the largest colour component at full, and moves the level to the
brightness axis. The hue stays the same. So `set_led_color` with `(128, 64, 0)`
reads back as `(255, 128, 0)` at half brightness.

A volume change or a mute change shows the volume for 2 seconds. The light
returns after that.

### Beam direction

The DSP reports the direction of the voice. The `update_led_beam_effect` script
shows this direction on the ring. When the wake word starts the pipeline, the
device locks the beam to the current direction. The device releases the beam at
the end of the pipeline. So the ring stays on the speaker during one utterance.

The **Beam lock** switch turns this behaviour on and off. Beam lock reads the
direction from the `led_beam_sensor` poll. Keep that poll on, or beam lock has
no direction to use.

The **Voice Direction** sensor shows the direction of the wake word in degrees,
from 0 to 360. The device sends one value for each wake word, so the sensor
does not load the connection. If Beam lock is on, the value is the locked
direction. The sensor also reads the `led_beam_sensor` poll. Home Assistant
disables the sensor by default. Enable it to turn a camera or a robot to the
speaker.

In your own lambdas, call `id(respeaker).get_azimuth_degrees()`. The call gives
the same direction and does no I2C read. It gives `NAN` if the poll has no
value from the last 500 ms.

---

## Develop the components

The packages get the C++ components from GitHub, so a new checkout builds the
**published** code and not your working tree.

To build your local changes, change the `external_components` block in
`packages/base.yaml`:

```yaml
external_components:
  - source:
      type: local
      path: esphome/components
    components: [respeaker_xvf3800, aic3104]
```

Do not commit this change.

To test a change to the packages, include them from disk:

```yaml
packages:
  base: !include ../packages/base.yaml
  hardware: !include ../packages/hardware.yaml
  voice_assistant: !include ../packages/voice-assistant.yaml
  leds: !include ../packages/leds.yaml
  timers_alarm: !include ../packages/timers-alarm.yaml
```

The `base.yaml` package still gets the core from GitHub at
`respeaker_lite_ref`. To test a change to the core, clone the Respeaker Lite
repository next to this repository. Then replace the `satellite_core` package
in `packages/base.yaml` with local includes:

```yaml
packages:
  core_base: !include ../../Respeaker-Lite-ESPHome-integration/packages/core/base.yaml
  core_voice_assistant: !include ../../Respeaker-Lite-ESPHome-integration/packages/core/voice-assistant.yaml
  core_media_player: !include ../../Respeaker-Lite-ESPHome-integration/packages/core/media-player.yaml
  core_timers_alarm: !include ../../Respeaker-Lite-ESPHome-integration/packages/core/timers-alarm.yaml
```

Do not commit this change.

Useful commands:

```bash
esphome config config/...yaml    # validate the merged configuration
esphome compile config/...yaml   # build the firmware and check the C++
esphome logs config/...yaml      # read the device logs
esphome clean config/...yaml     # clean after a component or firmware change
```

Run `esphome clean` after you change the component Python code or the firmware
file.

---

## Troubleshooting

**The I2C scan does not find address 0x2C.** Check the board power. The XMOS DSP
needs about two seconds to boot. The component waits for this.

**The DSP Status sensor does not show `OK`.** Read
[Check the DSP status](#check-the-dsp-status).

**There is no audio.** Check that the `aic3104` codec is at address 0x18 in the
I2C scan. Check that the media player volume is not zero.

**The wake word does not start the pipeline.** Check that the microphone is not
muted. Lower the value of the **Wake word sensitivity** select.

**The build uses old package files.** ESPHome caches a remote package for the
`refresh` time. Set `refresh: 0s` while you test.

**The response plays many seconds late.** Home Assistant sends the text to
speech audio while the engine still makes it. So a slow engine gives a slow
download. Test with a static audio file on the same host. If the static file is
fast, the cause is the engine. Use a local text to speech engine to make the
delay smaller.

**The LED ring stays off.** The `control_leds` script did not run. Check the
connection to Home Assistant, because the LED state follows the API state.

---

## Known issues

1. The **Mute** button is the only button. So you can only stop a timer or a
   response when you say "stop". You cannot start the pipeline by hand.
2. There is no hardware volume control. Only the software volume works.

---

## Credits

- [formatBCE](https://github.com/formatBCE) — components and configuration.
- The XMOS control protocol follows Respeaker's `xvf_host.py`.
- [dlarkinsPonderosa](https://github.com/dlarkinsPonderosa) trained the "Snooze"
  wake word model and designed the snooze feature
  ([#59](https://github.com/formatBCE/Respeaker-XVF3800-ESPHome-integration/issues/59),
  [#60](https://github.com/formatBCE/Respeaker-XVF3800-ESPHome-integration/pull/60)).
- The notification sounds come from the
  [Home Assistant Voice PE](https://github.com/esphome/home-assistant-voice-pe) project.
  The sounds are © 2024 by Clayton Charles Tapp, under CC BY 4.0. The `sounds/`
  directory of the Respeaker Lite integration holds copies in Ogg Opus format.
  Read [its sounds/LICENSE.md](https://github.com/formatBCE/Respeaker-Lite-ESPHome-integration/blob/main/sounds/LICENSE.md)
  for the changes.
