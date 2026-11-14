# Brother Pattern Programming Devices

Various electronic experiments with pattern programming on Brother knitting machines.

## brothercart

Reverse engineering of the Stitch Pattern Cartridge II.

This card is equipped with 32KB RAM and 32KB ROM.
The ROM is an EPROM without erase window and probably contains some built-in
knitting patterns. The RAM can be programmed freely.

To ensure data retention on the RAM chip, there is a huge 3V lithium battery
soldered onto the PCB. There is also some discrete logic to support memory
bank switching (probably).

## emulator

A re-implementation of the memory cartridge with a microcontroller, an SPI
flash chip (optional, 64KB..512KB), and a microSD card slot.

The cartridge memory bus is directly attached to the microcontroller.

Memory access will not be as fast as the original RAM and ROM, so make sure
not to run the bus faster than the microcontroller can feed data.
