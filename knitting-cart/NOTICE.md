# Attribution

These hardware files are an unmodified copy of Gregor Riepl's project
`seto/brother`, commit `fae0a1b65c6263e2575256fa544c826373d54b85`
(2021-10-02), from <https://git.kabelsalat.ch/seto/brother>.

The licence is the CERN Open Hardware Licence v1.2, in `LICENSE` in this
folder. It is not the MIT licence. CERN OHL v1.2 does not by itself cover
firmware; that repository contains no microcontroller source, only schematics
and board files.

Copied here:

- `brothercart.sch`, `brothercart.kicad_pcb`, `brothercart.pro`, `brothercart-cache.lib`
- `emulator/emulator.sch`, `emulator/emulator.kicad_pcb`, `emulator/emulator.pro`, `emulator/emulator-cache.lib`
- upstream `README.md`

Not copied: `Cartridge.xcf` (about 54 MB of PCB photographs). It remains at the
upstream URL.

The read-only dumper in `hardware/dumper/` is modified documentation derived
from `brothercart.sch`. Its modification notice is `hardware/dumper/MODIFICATIONS.md`.
That schematic is also under CERN OHL v1.2. The Arduino sketch and the Python
saver are software and are MIT, with the pin names taken from this documentation.

This notice is a factual attribution. It is not an endorsement by the upstream
author.
