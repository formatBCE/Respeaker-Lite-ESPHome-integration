### What is it
This repository contains example code to integrate Seeed Respeaker Lite Voice Kit (with XIAO ESP32-S3 on board) with ESPHome.
I'm thankful for continuous help of ESPHome team and Seeed developers, that let me put this thing together. 
It's using [brilliant job of ESPHome team](https://github.com/esphome/home-assistant-voice-pe), as well as firmware and guides from [Seeed Wiki](https://wiki.seeedstudio.com/xiao_respeaker/) and [their repository for Respeaker](https://github.com/respeaker/ReSpeaker_Lite/tree/master).
* This project is using MicroWakeWord. If you want to try training custom wake word for it (and are pretty experienced!), you can use [following notebook](https://github.com/kahrendt/microWakeWord/blob/november-update/notebooks/basic_training_notebook.ipynb) for this.

### *Disclaimer
#### Use this on your own risk. Expect breaking changes.
At the moment, all this is changing at very fast pace. Respeaker board considered alpha, PE (Voice Kit) software is under heavy development. I will try to update YAML and XMOS board firmware as soon as i can, but it can't be instantenuous...
If you encounter a problem, before creating ticket here you may go to PE and Seeed repos, linked above, and see if something changed there. I'll gladly accept pull-requests. :)

### Why?
I ditched all proprietary assistants for good about 6 months ago, and dove into Assist all-hands. 
With this board i finally can build device, that will satisfy me and (more important) my family as their trusted voice assistant.

### What to do with it?
__* Make sure you're using corresponding versions of I2S and ESPHome firmware. There is default one (working on 16kHz), and 48kHz one. Latter is even more experimental, but the sound quality is MUCH better. Thanks to Mike aka @mikey60 and his fork to nabu_microphone!__
1. Get Respeaker Lite with ESP32 soldered to it (you may solder it yourself, pins on the back can remain dry, they're not used)
2. [Solder USR to D2 and MUTE to D3 pins](https://wiki.seeedstudio.com/respeaker_button/)
3. [Flash I2S firmware to the XMOS board](https://wiki.seeedstudio.com/xiao_respeaker/#flash-the-i2s-firmware) (pay attention to USB port, you need the main board port, not ESP32 one)
5. Flash ESPHome firmware (YAML included, adjust to your environment) to ESP32 (use its port)
6. Add device to Home Assistant.

### Current state
- voice communication: works
- playing media: works (with exception of files, containing album art, playing from HA media picker - this is getting addressed by ESPHome guys (i can't thank them enough!!!)
- media ducking while voice communication ongoing: works
- interactions with HA timers: works
- LED: works (also can be exposed as light in HA)
- mute: hardware button works, software button works
- action button: works, multifunctional, based on PE logic

### ToDo
- DONE ~~software mute switch. Software triggering works, but state read doesn't work so far (needs custom ESPHome component for reading mute status with i2c). Check [Arduino example here](https://github.com/respeaker/ReSpeaker_Lite/blob/master/xiao_esp32s3_examples/xiao_i2c_get_register_value/xiao_i2c_get_register_value.ino#L55) if you want to help~~
- OTA software update for Respeaker board. Can snug this functionality from PE project.
- DONE ~~48kHz bitrate. Right now the board works on 16kHz, but Seeed already preoared 48kHz firmware. Great guys!~~
  ~~It doesn't work yet with ESPHome though, since we need to add resampler for the microphone to downsample it to 16kHz (currently there's no way to use 48kHz for mic). Speaker works though.~~
- hardware volume controls. ~~Would be nice to have, but there's no pins left on ESP32... Thinking...~~ I made some cradle with buttons and resistors to have 3 buttons on single pin - but it's bulky and inconvenient to solder, as well as to use in ESPHome. I decided to skip on it.
- bigger LED strip. The problem is same as for hardware volume... But if you want, you can solder strip/ring to the GPIO1 instead of inbuilt LED. I decided that i'm good with one on board so far.

### Casing
I made some casing to improve family approval factor. [Check it out.](casing/Casing.md)


[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/formatbce)
