### Daily alarm functionality

#### There are some limitations to the ESPHome time support, so please read carefully - it will require some setup.

The setup contains several sensors and switches, and two services.

First, let's setup the time zone on your satellite.

By default, ESPHome has time zone, that was set on your build environment (add-on, Docker container etc.). We don't want your alarm to ring at wrong time, so we need to set correct time zone to the device.
It's done once, and will be preserved on reboots.

- Go to [this page](https://github.com/nayarsystems/posix_tz_db/blob/fb5fa340cfa7599467358a347e5d6e6724d92bb2/zones.csv) and find your time zone.
- Copy the POSIX value from the right column. For me (Vancouver) it's `PST8PDT,M3.2.0,M11.1.0`.
- Go to `Developer tools - Actions` in your Home Assistant. We need to call action `esphome.{your_device_name}_set_time_zone`. Use copied POSIX TZ value for `posix_time_zone` field.
- After calling that action, proceed to your satellite device page and check that sensor `Current device time` is showing correct time.

That's it, initial setup is ready.

To set the alarm time, use action `esphome.{your_device_name}_set_alarm_time`. 
- The time should be set in 24-hour format `HH:MM` (for example, `07:30`, please pay attention to leading zero).
- The sensor `Alarm time` will reflect current value.

To activate alarm, turn on the switch `Alarm on`. *ATTENTION! If it's off, alarm won't ring!*

Now to the action. Find drop-down `Alarm action` and adjust it to your preference, what action should happen on alarm ring:
- `Play sound`: satellite will play same sound that is played when timer is ringing. This can be stopped with "stop" word.
- `Send event`: satellite will sent event `esphome.alarm_ringing`, which you can use in your automations to play music, open your window blinds or any other action of your choice. Check the event data to get `deviceId` of exact satellite, if you need it.
- `Sound and event` - both local sound playback and event sending will be executed.

That's it! Enjoy your new alarm!

### Snooze

When a timer or the alarm rings, say "Snooze". You do not say the wake word
first. The ring stops, and starts again after the **Snooze duration** (9 minutes
by default). You can snooze again each time the ring starts again. To stop the
ring, say "Stop" or the wake word.

The device does not listen for "Stop" while it waits to ring again, so a false
"Stop" cannot cancel your alarm. To cancel a snooze, press **Cancel snooze**, or
turn off **Alarm on**. The **Snooze pending** sensor shows that a ring waits to
start again. To disable snooze, turn off **Snooze enabled**.

The device keeps the snooze if it restarts. If the device was off for more than
15 minutes after the ring was due, the device does not ring.

Snooze needs the clock of the device. If the clock is not set, the device
ignores "Snooze" and continues to ring. The "Snooze" model knows English only.
[dlarkinsPonderosa](https://github.com/dlarkinsPonderosa) trained the model.
