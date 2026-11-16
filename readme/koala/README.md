[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/formatbce)

# Koala Satellite
<div style="display: flex; justify-content: space-between;">
  <img src="/casing/images/assembled.png" width="400">
  <img src="/instructions/images/live_view_old.jpg" width="400">
</div>

### Koala Satellite is a DIY device that works as a satellite for Home Assistant Assist.

### *For pre-built factory firmware flashing check [online flashing tool](https://formatbce.github.io/Koala-Satellite/)*

#### Main characteristics:
- two far-field microphones, coupled with XMOS chip XU-316 for noise cancelation and voice recognition;
- 16bit 48kHz audio output;
- 5W speaker to use for speech responses, announcements, chimes or music;
- hardware mute button;
- rotary encoder for volume change and other adjustments;
- LED strip for status displaying or mood lighting;
- 3.5mm line-out jack to connect external speaker.

#### Appreciation
I'd like to thank to NabuCasa team and Home Assistant project - this project firmware is almost completely based on their work, and created for Home Assistant exclusively.
My dedicated appreciation goes to the  [Voice PE](https://github.com/esphome/home-assistant-voice-pe) team, that made all of this possible.
Also, i'm thankful for continuous support from Seeed developers. We spent days debugging Respeaker Lite software. Now it works great!

#### How is it different from Home Assistant [Voice PE](https://www.home-assistant.io/voice-pe)?
They're actually pretty similar. I used PE ESPHome software as the base for Koala software, and many parts are working identically.
They connect to Home Assistant in the same way and have identical possibilities.
Koala is just something you can assemble yourself from widely accessible parts.
Also it has pretty decent for size audio speaker and doesn't have external sensors connectors.

#### How is it different from [this repository](https://github.com/formatBCE/Respeaker-Lite-ESPHome-integration)?
I started to work with [Respeaker Lite](https://www.seeedstudio.com/ReSpeaker-Lite-p-5928.html) in the Fall of 2024. 
That repository was my attempt to put everything i know together.
All the process is pretty much documented in [this Home Assistant Community thread](https://community.home-assistant.io/t/respeaker-lite-new-seeed-studio-voice-assistant-development-kit-hardware-combine-esp32-with-xmos-xu316-dsp-chip-for-advanced-audio-processing-as-a-esphome-based-home-assistant-assist-satellite-voice-devkit).
At some point i made Echo-Pop-like enclosure to the Respeaker Lite Kit. But it had disadvantages, like gluing face plate (or fabric), no access to ESP and Respeaker USB ports, lack of volume controls and rudimentary LED.
So i started thinking on real device. Here it is.

#### Why not use [Respeaker Lite with pre-soldered XIAO ESP32-S3 board](https://www.seeedstudio.com/ReSpeaker-Lite-Voice-Assistant-Kit-p-5929.html)? Isn't that easier and cheaper?
Yes, it is. But pretty much all exposed ESP GPIO pins are used by Respeaker in that configuration. So there's no way to use more elements like rotary or LED strip.
Also, current configuration hides nicely USB power cable (which is also ESP32 data cable), and allows exposing RESET/BOOT buttons for flashing.
If you're using pre-soldered version, you may check [this PR by @rananna](https://github.com/formatBCE/Koala-Satellite/pull/5) _(Thank you!)_.
Or even go full bananas and use it without dedicated LED strip (top LED should work okay) - just adjust LED diffuser color to your like.

### [Here's how you make one for yourself.](/instructions/assembly.md)

### Check main abilities [here on Respeaker repository](https://github.com/formatBCE/Respeaker-Lite-ESPHome-integration/tree/main?tab=readme-ov-file#current-abilities) - they're identical

### What if i don't want or cannot build it myself?
Create issue in this repository to let me know, that you need fully assembled device (or just printed enclosure for it). I will consider selling if demand is there. :)


[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/formatbce)
