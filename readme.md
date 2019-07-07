# SX70 back

## This Arduino sketch is for managing a SX70 instant film back.

The back is made of a SX70 which only the ejection mecanism was used.
It's included in a wooden enclosure made to fit a Busch Pressman C 3.5x4.5 press camera.

The mecanism includes motor, original switches, gear train. The shutter, bellows, mirror, and original enclosure has been removed, as well as the picture counter, which has been embbeded in the arduino program.

More informations about the functions, switches management, etc. in the .ino file.

###The program uses two libraries :
####Push button
https://github.com/troisiemetype/PushButton
Simplifies the switches and button management, by providing functions to update, debounce short and long clic, etc. Once a button / switch is created, it just have to be updated as soon as possible (polled in the main loop), and tested whenever needed)

####Timer
https://github.com/troisiemetype/Timer
Timer made from millis() core Arduino function, with convenient functions for setting times, make loops, etc. Non blocking.