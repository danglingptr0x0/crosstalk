The mission statement of Crosstalk is that I would like it to bring back the joy of tinkering, designing things from near-scratch, and taking one's time to create something new or recreate retro technology/port cool software. I don't see that much anymore in computing.

<br>

**Crosstalk** is a set of tools to create custom computer architectures, RTL designs, and software. It contains a sandbox with a simulator and an emulator for building electronics using HDL. Players can construct and program devices like calculators, LED controllers, and complex computers. It comes with a gate-level SystemVerilog simulator, an elaborator, a synthesizer, and a C emitter, plus the Clang compiler, which compiles the emitted C code and runs it for faster design execution. The C code is derived from players' own SystemVerilog.

## **The default workflow is opening the RTL workspace, entering a shell, describing a design in SystemVerilog, programming it, and then writing software for it. There is currently no RTL-related guidance or tutorial. Players get a tool guide when they create their first project, but except for the included examples, a player will have to write own SystemVerilog or load an existing SystemVerilog project.**

The game supports a networked mode, in which one collaborates with other players.

Designs, software for them, and emulators can all be shared via the Workshop (info on how this will be done without Steam coming in the near future)

<br>

<h2 align="center"><strong>Serial interface</strong></h2>

Crosstalk can interface with devices using virtual in-game serial ports or the running machine's physical serial interface, which is */dev/ttyUSB&lt;n&gt;* and */dev/ttyACM&lt;n&gt;* on Unix and *COM&lt;n&gt;* on Windows, or its own shell interface. External devices can thus be plugged directly into designs and be represented by stand-in parts. Players could use microcontrollers, sensors, physical buttons, or other computers.

<p align="center"><img src="img/extras/3cb08693d03aed5c4375812f3630a12e.avif" width="1506"></p>

<p align="center"><a href="img/extras_big/72b8f65f4a59c715964afe42bd8e38af.webp"><img src="img/extras/72b8f65f4a59c715964afe42bd8e38af.avif" width="1560"></a></p>

<p align="center"><img src="img/extras/7e3fd71bd9e3f1386b9605082aa601c0.avif" width="1100"></p>

<p align="center"><i>The supplied truecolor terminal emulator can be used as a serial terminal I/O device. It is ANSI/VT100/xterm/kitty-compatible and can interact with the host over a PTY/ConPTY</i></p>

<br>

<p align="center"><a href="img/extras/d0cf2f72b965d547d1f691957a9ac9ce.webm"><img src="img/extras/d0cf2f72b965d547d1f691957a9ac9ce.poster.avif" width="960"></a></p>

<p align="center"><i>Serial I/O works with physical devices. The above is TX.</i></p>

<br>

<p><a href="img/extras/21eef8218ae16628fe8d7f26adb59f3f.webm"><img src="img/extras/21eef8218ae16628fe8d7f26adb59f3f.poster.avif" width="960"></a></p>

<p align="center">RX</p>

<br>

<h2 align="center"><strong>Scopes</strong></h2>

Memory view of the running design and an oscilloscope are necessary tools while building, so they're both included and easily accessible to players.

<p><a href="img/extras_big/03e1c0d9077b70f8fd253b628d1c2536.avif"><img src="img/extras/03e1c0d9077b70f8fd253b628d1c2536.avif" width="1560"></a></p>

<p align="center"><img src="img/extras/dc417a154d16242f317ffc3c459e2c5c.avif" width="962"></p>

<p align="center"><a href="img/extras_big/53b5ad07e1fb050301f6942b10186e99.webp"><img src="img/extras/53b5ad07e1fb050301f6942b10186e99.avif" width="1560"></a></p>

<p align="center"><i>Memory view and runtime activity indication</i></p>

<br>

<p align="center"><img src="img/extras/1ade05d4fca19870b98920706ca6b7a6.avif" width="1132"></p>

<p align="center"><i>An oscilloscope attached to a signal, like sound output</i></p>

<br>

<h2 align="center"><strong>Display output</strong></h2>

If a design has a video display output, then players can use one of the three built-in display modes: CRT, gas-plasma, and modern LCD. The concrete interfaces used are CGA (for CRT and gas-plasma) and VGA.

<p align="center"><img src="img/extras/2fc0bdf14a81f39e07ea9919f073ef20.avif" width="664"></p>

<p align="center"><i>A CGA gas-plasma display output type showing the DANGLING kernel booting on the DLU, in-game</i></p>

<br>

<h2 align="center"><strong>CPU architecture design</strong></h2>

Players can define ISAs and then view a help banner in the game while developing. Later on, a guided ISA design feature set will be released.

<p align="center"><img src="img/extras/58ae9875cfc74591bff54ba9d3359d46.avif" width="1008"></p>

<p align="center"><i>The DANGLING Computer Superarchitecture ISA</i></p>

<br>

<p align="center"><img src="img/extras/9579fe5583bdafae359481f48725af4d.avif" width="344"></p>

<p align="center"><i>Runtime register state overview</i></p>

<br>

<p align="center"><img src="img/extras/c84af657d3701ce6819376d1dd70656a.avif" width="571"></p>

<p align="center"><i>A stack view that shows a trace on faults</i></p>

<br>

<p align="center"><img src="img/extras/7e6eabd25794352e8b4e8f862bd582c0.avif" width="360"></p>

<p align="center"><i>Program locator overview</i></p>

<br>

<p align="center"><img src="img/extras/7db745059d49c346f2534c99ddf40fea.avif" width="772"></p>

<p align="center"><i>The program-level Profiler mode shows where time is going during a program's runtime</i></p>

<br>

<h2 align="center"><strong>SystemVerilog</strong></h2>

FPGA designs can be described using vendor-neutral SystemVerilog. The designs can be RTL-simulated. For accelerated simulation, there is a built-in option to emit C99 from the supplied SystemVerilog using Crosstalk's toolchain and the enclosed Clang compiler, or run it in emulation mode, which disables design accuracy guarantees and reproduces the design behaviorally instead, using the built-in emulator. For high-resolution simulation, a gate-level simulator is available, which runs a synthesized variant of the design. This is an expensive operation, so it takes much longer and is capped to smaller designs.<img src="img/extras/fc8f75136005b97fc5550705de73fbb2.avif" width="772">

<p align="center"><i>The included Profiler shows an overview of where cycles within the SV design are spent</i></p>

<br>

<p align="center"><img src="img/extras/b27586a44179446213165f0d98aa9962.avif" width="1101"></p>

<p align="center"><img src="img/extras/808cb5f4baae0f705a3358d2cf9d54f4.avif" width="1110"></p>

<p align="center"><img src="img/extras/345f1426338c5e6d860d29a87e1e3146.avif" width="1096"></p>

<p align="center"><i>The interactive Schematic viewer shows both the RTL and the gate-level schematic and in profile mode, highlights hot cells</i></p>

<br>

<h2 align="center"><strong>The DANGLING Computer Superarchitecture ISA + The DANGLING Logic Unit CPU</strong></h2>

The game comes with a custom instruction set architecture and its first CPU called the DLU, which serve as both a reference SystemVerilog implementation and a target for players who don't want to make a custom one. DANGCSA/DANGXA (it has two names due to how "C" and "S" sound when spoken, so both are used) is already capable of running complex programs, despite being very minimal. The DLU is DANGCSA/DANGXA-compliant and supports serial communication, text mode output, video, audio, and MIDI. The full title of the DLU is "DANGLING Logic Unit: Think Correctly."

<p align="center"><img src="img/extras/fa62754445bbd8821df3251986d296e2.avif" width="751"></p>

<p align="center"><i>The LLVM that ships with the game can compile for the DANGCSA/DANGXA architecture</i></p>

<br>

<p align="center"><img src="img/extras/b025d74feef5178a795001a5da47007c.avif" width="720"></p>

<p align="center"><i>DANGCSA/DANGXA's own binary format is also recognized by the enclosed LLVM and is what's natively used on the platform. ELF only serves as a compatibility layer</i></p>

<br>

<p align="center"><a href="img/extras/7d1d67db11e5c4b72049a135bfef5827.webm"><img src="img/extras/7d1d67db11e5c4b72049a135bfef5827.poster.avif" width="1170"></a></p>

<p align="center"><i>A game running native DANGCSA/DANGXA code. Runtime memory manipulation is demonstrated here</i></p>

<br>

<p><a href="img/extras_big/c8cab725b2a8a69fbf9c776a4b99ad48.webp"><img src="img/extras/c8cab725b2a8a69fbf9c776a4b99ad48.avif" width="1560"></a></p>

<p align="center"><i>DANGCSA/DANGXA's first custom native game, <strong>Hardware Sniffers</strong>, coming as a free mod</i></p>

<br>

<p align="center"><a href="img/extras/a24a7ba20c9f5aafbc6a0fe2200f7d7e.webm"><img src="img/extras/a24a7ba20c9f5aafbc6a0fe2200f7d7e.poster.avif" width="1170"></a></p>

<p align="center"><a href="img/extras/604d7f49beb6aa233fe83b2a3702379a.webm"><img src="img/extras/604d7f49beb6aa233fe83b2a3702379a.poster.avif" width="1170"></a></p>

<p align="center"><a href="img/extras/f225bd561fcf012ac15c9bbef125c57a.webm"><img src="img/extras/f225bd561fcf012ac15c9bbef125c57a.poster.avif" width="1170"></a></p>

<p align="center"><i>A DANGCSA/DANGXA-native C99 reimplementation of the popular crafting game running as the client of the on-screen character, looking from the perspective of another client, which is running an original Java client superimposed on Crosstalk for clearer visual demonstration. The C99 client controls the displayed character by manipulating positions and actions in memory. In this case, only the C99 text client runs under DANGCSA/DANGXA, and is independent of the original client and Java</i></p>

<br>

<h2 align="center"><strong>Sandbox mode</strong></h2>

<p align="center"><strong>Self-play submode</strong></p>

Sandbox mode is the default Crosstalk experience. Players can create a custom workspace with resizable, positionable, and alignable floating windows. Most use-cases start with opening a shell session to their host, invoking their editor, and writing SystemVerilog. Once there is a valid design, it can be simulated inside Crosstalk.
