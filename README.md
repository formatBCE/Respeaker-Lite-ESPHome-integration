### _Check out [Koala Satellite](https://github.com/formatBCE/Koala-Satellite), the next step of evolution for this project!_


## What is it
This repository contains example code to integrate Seeed Respeaker Lite Voice Kit (with XIAO ESP32-S3 on board) with ESPHome.
I'm thankful for continuous help of ESPHome team and Seeed developers, that let me put this thing together. 
It's using [official code for Voice Preview Edition by ESPHome team](https://github.com/esphome/home-assistant-voice-pe), as well as firmware and guides from [Seeed Wiki](https://wiki.seeedstudio.com/xiao_respeaker/) and [their repository for Respeaker](https://github.com/respeaker/ReSpeaker_Lite/tree/master).

## *Disclaimer
### Use this on your own risk. Expect breaking changes.
Although Voice PE is released, parts of its code are still under development and merging to ESPHome core repository with breaking changes. I will try to update YAML and XMOS board firmware as soon as i can, but it can't be instantenuous...
If you encounter a problem, before creating ticket here you may go to PE and Seeed repos, linked above, and see if something changed there. I'll gladly accept pull-requests. :)

## Why?
I just wanted to play around with the code tweaking and customizing it for my own Home Assistant.

## What to do with it?
__* Thanks to Mike aka @mikey60 and his fork to nabu_microphone, this project is using 48kHz sample rate for better music playback quality.*__
1. Get Respeaker Lite with ESP32 soldered to it (you may solder it yourself, pins on the back can remain dry, they're not used).
2. [Solder USR to D2 and MUTE to D3 pins](https://wiki.seeedstudio.com/respeaker_button/). _**ATTENTION! This step is mandatory, as without it the buttons on satellite won't work as intended.**_
2a. [Solder 12 RGB WS2812 5050 LED to 5v, GND, and LED Signal wire to GPIO2 - D0 on SeeedStudio ESP32S3 board] - **OPTIONAL! If adding LED lights one of the main reasons for this fork. I used https://amzn.to/4hPLEgw
3. [Flash 48kHz I2S firmware of version **not lower than 1.1.0** to the XMOS board](https://wiki.seeedstudio.com/xiao_respeaker/#flash-the-i2s-firmware) (pay attention to USB port, you need the main board port, not ESP32 one). Make sure you're using 48kHz version, as 16kHz version won't work with this repo. You can use included [firmware file](/respeaker_lite_i2s_dfu_firmware_48k_v1.1.0.bin) to be sure.
5. Flash ESPHome firmware (YAML included, place `/config/common/respeaker-satellite-base.yaml` into ESPHome `common` directory, and adjust `/config/respeaker-satellite-dashboard-example.yaml` to your needs) to ESP32 (use its port).
6. Add device to Home Assistant.

## Current state
- voice communication: works
- playing media: works (with exception of files, containing album art, playing from HA media picker - this is getting addressed by ESPHome guys (i can't thank them enough!!!)
- media ducking while voice communication ongoing: works
- interactions with HA timers: works
- LED: works (also can be exposed as light in HA)
- mute: hardware button works, software button works
- action button: works, multifunctional, based on PE logic
- default HA wizard for satellites: works
- drop-down with wake word selection: works
- added switches to turn off button sounds, mute/unmute sounds
- added sensors for next device timer properties (time is updated once per 5 sec, name is available if set)

## DFU software auto-update
Since version 2025.2.2, the firmware includes corresponding DFU firmware for Respeaker Lite board. On first start after update, new firmware will be installed to Respeaker automatically. You will see Respeaker LED flashing yellow, while installing, and green on successful install. So now there's no need to update DFU firmware. Woohoo!
