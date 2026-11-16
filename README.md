# Respeaker ESPHome integration

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/formatbce)

This repository holds ESPHome firmware that makes Home Assistant voice satellites from Seeed ReSpeaker boards. All
devices use one voice satellite core and one release.

## Devices

| Device | Hardware | Web installer |
| --- | --- | --- |
| [Respeaker Lite](readme/lite/README.md) | Seeed ReSpeaker Lite Voice Kit with the XIAO ESP32-S3 | [Install](https://formatbce.github.io/Respeaker-ESPHome-integration/lite/) |
| [Respeaker XVF3800](readme/xvf3800/README.md) | Seeed ReSpeaker XVF3800 USB 4-Mic Array with the XIAO ESP32-S3 | [Install](https://formatbce.github.io/Respeaker-ESPHome-integration/xvf3800/) |
| [Koala Satellite](readme/koala/README.md) | An ESP32-S3 board and the ReSpeaker Lite board in a 3D-printed casing | [Install](https://formatbce.github.io/Respeaker-ESPHome-integration/koala/) |

The web installer needs Chrome or Edge on a computer. The page of each device tells you how to build the firmware
yourself.

## Shared features

The voice satellite core gives these features to all devices:

- [Daily alarm and snooze](readme/alarms.md)
- [TTS URI event](readme/tts_uri.md)
- [Home Assistant blueprints](readme/blueprints.md)

## Releases

All devices use one release tag, for example `2026.11.0`. The [tags page](https://github.com/formatBCE/Respeaker-ESPHome-integration/tags)
shows all releases. The `respeaker_lite_ref` substitution in your configuration selects the release.

A release can have no changes for your device. Home Assistant then shows an update that changes nothing.

## Old repositories

The XVF3800 firmware and the Koala firmware had their own repositories before. Their old releases stay there, so a
configuration that uses them continues to build. But that configuration gets no new releases. To get new releases, use
the example configuration on the page of your device.

A device that you installed with the web installer moves to this repository with its next update. You do not need to
do a step.

## Licenses

The code in this repository uses the ESPHome license. Read [LICENSE.md](LICENSE.md).

The built-in sounds come from the Voice PE project. The sounds are © 2024 by Clayton Charles Tapp, under CC BY 4.0.
The `sounds/` directory holds copies in Ogg Opus format. Read [sounds/LICENSE.md](sounds/LICENSE.md) for the changes.
