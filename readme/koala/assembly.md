## Koala assembly process.

### First, check out and order required [parts and tools](parts_and_tools.md).
You don't have to use exactly the parts i used - but make sure that your parts will fit to the casing.
For example, threaded inserts holes are 4mm, count on that.
The rotary encoder is pretty specific, but it's widely accessible.
Same goes to ESP32-S3 board. Make sure it has sticking antenna - the casing made for it. You'll find over9000 clones on Amazon, and mostly (!) they will be decent. Just make sure the board works before proceeding.

### Prepare the casing
Take a look at [images folder](/casing/koala/images) to check how it looks in detail.
You may find files for 3D-printing here:
  - [STEP](/casing/koala/step);
  - [STL](/casing/koala/stl) (by @systemik);
  - [3MF](/casing/koala/3mf) (by @systemik).

_HINT: If you print with dark/non-translucent filament, consider printing face plate for top LED cover, and print top LED cover in white/clear filament to make mute LED and Respeaker LED visible. Otherwise just print regular face plate - it has thin layer that will make them visible._

#### Some tips and tricks on printing:
- Use as much infill as you can for main casing, speaker plate and face plate. Ideally, make it 100%. This will make sound better, and ensure you don't have broken parts (especially with face plate).
- Use tree supports for main casing and speaker plate. Place supports on build plate only.
- Print rotary with fuzzy skin option. It looks way better than usual seam.
- Print LED diffuser with white plastic, with ~40% infill. More infill will diffuse light better, but lower the brightness.
- Clean bottom part of main casing with prints cleaning tool. It has the pocket for LED diffuser to slide into (that was made to make LED strip look thinner than it is). Make sure bottom is clean and smooth.
- Place threaded inserts into prepared holes:
  - 4x inserts for face plate legs;
  - 4x inserts for speaker plate (from the bottom);
  - 2x inserts for Respeaker Lite bracket mounting.
- Use rubber pads for casing legs. It will greatly reduce vibrations and will make sound undisputably better!

### Prepare guts
#### Thanks to [@rodrigo](https://github.com/rodrigosiviero), we have full [wiring diagram on Cirkit](https://app.cirkitdesigner.com/project/69192877-c1b3-4971-989a-3abfdbbd7a87). Check it out!
- Bridge buttons on Respeaker Lite to corresponding pins (`USR` to `D2`, `Mute` to `D3`). _**ATTENTION! This step is mandatory, as without it the buttons on Koala won't work as intended.**_
- Some ESP32-S3 boards have `IN-OUT` and `RGB` bridges, that aren't bridged. Bridge them with solder. **(Hint: `IN-OUT` enables 5V pin, and `RGB` pad makes onboard LED working.)**
- You may use Respeaker Lite JST connector for speaker directly. But it's pretty inconvenient with my enclosure. So i do the following (feel free to skip this if you don't want to hassle):
    - Solder the male Jst 2.0 Ph 2-Pin Connector to the speaker. Keep the cable short, ~3cm.
    - Solder the female Jst 2.0 Ph 2-Pin Connector to the Respeaker Lite. You will find corresponding pads on Respeaker board near the soldered JST connector (marked + and -). Check the cables polarity. Make the cable long, so it sticks from the bottom of main casing, when Respeaker is on top (~12cm).
- Cut 43 LED long piece of WS2812 strip. Make sure it will fit LED diffuser slot nicely. Start from round hole, and move clockwise when placing the strip into diffuser.
- Solder cables of 3-wire connector male part to the LED strip. Keep these cables ~5cm long. Check the wiring. I prefer to color-code (5v -> red, GND -> black, data -> other colour).
- Solder cables of 3-wire LED connector female part to the ESP board. Make these cables ~7cm long.
  - power -> `5V`
  - ground -> `GND`
  - data -> `GPIO13`
- **Caution: connect the encoder `+` to `3V3`, not to `5V`.** Most KY-040 modules have pull-up resistors to `+`. With `5V` on `+`, the `SW`, `CLK`, and `DT` lines get 5 V, and the ESP32-S3 pins are not 5 V tolerant.
- Solder the rotary encoder to ESP board. Make sure cables length is enough for mounting, but not too long.
  - `+` -> `3V3`
  - `GND` -> `GND`
  - `SW` -> `GPIO20`
  - `CLK` -> `GPIO47`
  - `DT` -> `GPIO21`
- Solder all Respeaker Lite pins to the ESP board. This will take some time.
  - Check pinout on [this image](images/respeaker-esp-pinout.jpg).
#### At this stage, you can [install the firmware](software.md), connect the peripherals and check that everything works.

### Final assembly process
- Place the face plate face down on the table.
- Put buttons part into the slot in main casing.
- Bring Respeaker Lite to its place on face plate. Fix it in place with bracket, and screw 2 short M3 screws into corresponding inserts. _Tip: don't screw it all the way down. Make sure buttons are clickable and the board isn't rattling._
- Put the rotary encoder to its place, and screw it in place with accompanying nut from the face side.
- Place ESP board antenna to corresponding slot in face plate.
- Slide the main casing on top of face plate posts. Make sure ESP board is set in place, and USB connector is completely visible in its slot. Use screwdriver or tweezers to guide it while sliding main casing in.
- Through the holes in bottom part of main casing, screw 4 longer M3 screws to the inserts in face plate legs. Use magnetized screwdriver for better handling.
- Place LED strip to the diffuser, starting from the cables hole, clockwise.
- Push  LED strip connector through corresponding (back left) hole inside of the main casing. Use tweesers.
- Connect LED strip to the ESP board.
- Slide the LED diffuser with the strip onto its place. Make sure that connecting cables are free!
- Put speaker into the hole in speaker plate, and screw 4 short M3 screws into corresponding threaded inserts to mount speaker firmly in place. You can use some rubber glue to seal the connection, or just screw it in very good.
- Connect speaker to the Respeaker board with JST connector.
- Slide speaker plate into main casing, and turn it in place. It's using twist-and-snap connection.
- Push the knob onto the rotary encoder post. Make sure it is set to the end.

#### That's it. If you didn't install firmware to the device before, it's right time to [do it](software.md).
