# SX70 back

## This Arduino sketch is for managing a SX70 instant film back.

The back is made of a SX70 which only the ejection mecanism was used.
It's included in a wooden enclosure made to fit a Busch Pressman C 3.5x4.5 press camera.

The mecanism includes motor, original switches, gear train. The shutter, bellows, mirror, and original enclosure has been removed, as well as the picture counter, which has been embbeded in the arduino program.

More informations about the functions, switches management, etc. in the .ino file.

### Be kind :

If you use this code and files for a project, consider letting me know, drop a line : lelabodu troisieme (at) gmail.com. Pictures will be welcome, and a donation as well ! :)

### About 3D files :

The STL for the back are included in the STL folder.

*Inner frame* is the only part to be 3D printed, all the enclosure is made of wood, and trigger button and door latch made of brass. They probably can be 3D printed too.

*Main shell* has holes for 3mm leds. On mine the small holes have been filed with clear PU resin.

*Door shell* is composed of several parts assembled, there is an inside slot for mounting a "light tongue", altough maybe not sctrictly necessary.

### About Arduino and electronics :

The flex circuit has been removed from the back chassis. Switches have been connected to ground, and to Arduino using internal pull-up resistors. S3 and S5 (see SX70 repair manual) are used to detect different ejections steps. S7 is the main power switch, which closes and powers the system when a pack is inserted and the door closed. The Arduino is powered as long as there is a pack, but is put asleep after 20 seconds (can be modified) idle.

An Arduino pro-mini has been used, it's placed in a 2mm thick recess in the inside of the main shell near the leds, and simply mounted with double-side tape

### The program uses two libraries :

#### Push button
https://github.com/troisiemetype/PushButton
Simplifies the switches and button management, by providing functions to update, debounce short and long clic, etc. Once a button / switch is created, it just have to be updated as soon as possible (polled in the main loop), and tested whenever needed)

#### Timer
https://github.com/troisiemetype/Timer
Timer made from millis() core Arduino function, with convenient functions for setting times, make loops, etc. Non blocking.