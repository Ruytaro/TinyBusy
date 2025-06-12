
# TinyBusy

## RGB status indicator

Tell everyone your availability to meet, or signal the meeting room status. All without the need of any cable. You can even pair your remote (~~sold separately~~, *use the DiY version*) and switch the light without leaving from your desk.

You *still* need a usb cable in order to recharge the battery, ~~but hey is better than nothing~~.

## The RX ingredients (indicator)

- an Attiny13A (1kB flash, 64B RAM, 64B EEPROM) @ 9.6MHz
- 2 "bare" SMD 5050 RGB leds (no fancy neopixels here)
- TP4056 Li-ion battery charger.
- Some smd resistors and capacitors
- one Li-ion battery (capacity and format to determinate)
- a custom PCB
- a 3D printed case
- an OOK receiver module

## The TX ingredients (controller)

- an Attiny13A (1kB flash, 64B RAM, 64B EEPROM) @ 9.6MHz
- one "bare" SMD 5050 RGB led (to show the remote status)
- Some smd resistors and capacitors
- a custom PCB
- a 3D printed case
- an OOK transmiter module
- an USB cable to power it
