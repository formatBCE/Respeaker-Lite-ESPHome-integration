<img src="step_10_insert_to_main_casing.png" width=600 />

## Casing asembly instructions.

This is the casing i made for myself. Sorry if something goes wrong or too complicated with it - i use it around the house now and it suits me well.
#### Advantages: 
- relatively easy to assemble/disassemble
- looks pretty good
- not the worst sound thanks to big-ish acoustic camera (although PLA isn't actually good for sound...)
#### Disadvantages:
- uses some other parts, that you'll have to buy...
- doesn't give access to ESP and XMOS board USB ports. I tried, but it starts looking ugly...

### Parts used
1. USB female port like this: https://www.aliexpress.com/item/1005007469691687.html (for polished look). You can use simple USB-C cable instead...
2. Speaker: 2 inch 4 Ohm 5 W speaker like this: https://a.co/d/cEpEeNt.
3. Threaded inserts. I use https://a.co/d/bMeMx9B. You may screw some bolts into the holes instead, but i strongly recommend using inserts.
4. M3 screws set like this: https://a.co/d/e6pNfBu.

### Preparation
0. Make sure you have flashed latest (and working!) versions of software to ESP and XMOS board. I'm serious, you will have to unscrew at least one screw to get access to the USB.
1. Print all STEP files. Use vibrant colours. :)
   - If you don't want to use fabric finish on your speaker, skip printing Applicator.
   - Supports needed only for main casing. Print it face down.
3. Solder the speaker to Respeaker board with ~10cm wires.
4. Solder female Dupont wires ~3cm length to 5V and GND pads on Respeaker bottom back.
5. Solder male Dupont sockets to USB female port cables.
6. Place threaded inserts into corresponding holes:
   - 2x inserts from back of face plate - for Respeaker bracket.
   - 1x insert from back of face plate on long pole - for final assembly.
   - 4x inserts from front of face plate - for speaker.

### Assembly
1. Face plate is main part:
<img src="step_1_face_plate.png" width=400 />
2. This is how it looks from the back:
<img src="step_2_face_plate_back.png" width=400 />
3. Insert LED covers (both center LED and power LED):
<img src="step_3_face_plate_with_led_covers.png" width=400 />
4. Place Respeaker board on its slot:
<img src="step_4_face_plate_with_respeaker.png" width=400 />
5. Cover board with bracket and screw it in with two M3 screws:
<img src="step_5_add_respeaker_bracket.png" width=400 />
6. Insert the speaker and screw it in with 4x M3 screws:
<img src="step_6_add_speaker.png" width=400 />
7. Glue the grill or acoustic-transparent fabric to the face plate (if you choose fabric, you can use Applicator to hold it in place while it's drying):
<img src="step_7_glue_grill.png" width=400 />
8. Glue mute and action buttons (if you use fabric finish, cut small holes for button stems):
<img src="step_8_glue_buttons.png" width=400 />
9. Prepare the casing. Put the USB female port cables through back hole in casing, but don't pop it in yet.
Attach 5V and GND to Respeaker with Dupont.
If you decided to use plain USB cable instead, put it through the hole and insert into ESP32 USB port.
Make sure that you're using cable that doesn't stick outside the face plate (probably, using 90-degree connector will be better).
<img src="step_9_main_casing.png" width=400 />
11. Insert main assembly into the casing. The chin on the bottom will help you guide the plate correctly. Also make sure the 3.5mm port is aligned with the hole in main casing.
<img src="step_10_insert_to_main_casing.png" width=400 />
12. Insert M3 screw through the hole in main casing and screw it into face plate pole. Tighten it up.
<img src="step_11_screw_main_casing.png" width=400 />
13. Pop the USB port into the place. That's it!
