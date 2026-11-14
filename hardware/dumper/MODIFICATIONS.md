# Modification notice (CERN OHL v1.2 section 3.4)

Date: 2026-11-14.

This directory is modified documentation derived from Gregor Riepl,
`seto/brother`, commit `fae0a1b65c6263e2575256fa544c826373d54b85`,
file `brothercart.sch` (and the functional renames on the same connector in
`emulator/emulator.sch`). Upstream: <https://git.kabelsalat.ch/seto/brother>.
The unmodified files are in `knitting-cart/`. The licence text is `LICENSE`.

What changed relative to that schematic:

- A separate read-only dumper for an Arduino Mega 2560, not seto's ATmega165PA emulator.
- Edge pin 31 (`P31` / emulator name `WERAM1`) is tied to GND through 470 ohm and is not connected to a GPIO.
- Edge pin 34 (`P34` / `~WERAM2`) is pulled up to the dumper's 5 V and is only driven high.
- Edge pin 40 (`VMEM` on the retrace; `+5V` on the emulator sheet) is not powered. It is only sensed through a 100k/100k divider.
- 470 ohm series resistors on every other signal.
- VCC (edge pin 2) is fed through 47 ohm after GND is connected.
- No write path, no WE pulse, no connection to the embroidery-card PCB in `legacy/`.

The Arduino sketch and `tools/save_dump.py` are software. CERN OHL v1.2 does not
cover them unless stated. They are MIT. The schematic, the SVG, and this notice
are CERN OHL v1.2.
