# Sounds

The sounds in this directory come from the
[Home Assistant Voice Preview Edition Sounds](https://github.com/esphome/home-assistant-voice-pe/tree/dev/sounds)
© 2024 by [Clayton Charles Tapp](https://www.cctaudio.com/). The sounds are
licensed under
[Creative Commons Attribution 4.0 International](https://creativecommons.org/licenses/by/4.0/).

Changes: the files are encoded again to Ogg Opus, 48 kbps, mono, 48 kHz, so
that the firmware is smaller. The sound content is not changed.

| File | Source file |
| --- | --- |
| `center_button_double_press.opus` | `center_button_double_press.flac` |
| `center_button_long_press.opus` | `center_button_long_press.flac` |
| `center_button_press.opus` | `center_button_press.flac` |
| `center_button_triple_press.opus` | `center_button_triple_press.flac` |
| `error_cloud_expired.opus` | `error_cloud_expired.mp3` |
| `factory_reset_cancelled.opus` | `factory_reset_cancelled.mp3` |
| `factory_reset_confirmed.opus` | `factory_reset_confirmed.mp3` |
| `factory_reset_initiated.opus` | `factory_reset_initiated.mp3` |
| `mute_switch_off.opus` | `mute_switch_off.flac` |
| `mute_switch_on.opus` | `mute_switch_on.flac` |
| `timer_finished.opus` | `timer_finished.flac` |
| `wake_word_triggered.opus` | `wake_word_triggered.flac` |

To make a file again, use this command:

```bash
ffmpeg -i <source file> -map_metadata -1 -map 0:a -ac 1 -ar 48000 -c:a libopus -b:a 48k -fflags +bitexact -flags:a +bitexact <name>.opus
```
