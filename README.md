# sgtouzen
Sega SG-1000/SC-3000 Emulator

This emulator is specialized to run a BASIC ROM cartridge on the [Sega SG-1000/SC-3000](https://en.wikipedia.org/wiki/SG-1000) systems in a Linux terminal.

Features:
* Works with the Sega BASIC Level II and Level III cartridges.
* RAM is maxed out to 32K, and the upper 16K can be mirrored as 8K.
* Curses based UI with 256-color support if available.
* SDL2 graphical output also available and can run in parallel.
* SK-1100 keyboard emulation.
* TMS9918 text and graphics I and II modes emulated.
* Basic 8x8 and 16x16 sprites are supported in the SDL2 graphic mode.
* Cassette emulation by reading or writing (Mono 8-bit 44100Hz) WAV files.
* Injecting text files (e.g. BASIC programs) as keyboard input.
* Ctrl+C in the terminal breaks into a debugger for dumping data.

Known issues and missing features:
* No joypad emulation.
* No SN76489 sound emulation.
* Sprite coincidence, 5th sprite and sprite magnification not supported.
* TMS9918 multicolor mode not supported.

Information on my blog:
* [Sega SG-1000/SC-3000 Emulator for BASIC](https://kobolt.github.io/article-228.html)

YouTube videos:
* [Sega SC-3000 basic maze program](https://www.youtube.com/watch?v=8sqLQI7qfFQ)

