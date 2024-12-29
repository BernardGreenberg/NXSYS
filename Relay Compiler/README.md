# Relay Compiler Status -- Christmas 2024

The NXSYS Relay Compiler has been resurrected, both in general and specifically for the arm64 architecture (Mac M1/M2/M3 etc.) 
There are now three architectures to deal with, Windows Intel X86, Mac Intel X86, and Mac arm64 ("New Macs"). Only the last is currently (26 December 2024) fully implemented and tested.  There is residual 32-bit Windows code generation available, but it is mainly a joke, because current Windows is 64-bit.  It might be salvageable, though, and the compiler thinks it is multi-homed/multi-targeted.

The Relay Compiler takes the same "pseudo-Lisp" descriptions of interlockings as does NXSYS proper (henceforth, "the simulator") and creates one "object file" (**.tko**) for an entire interlocking.  It compiles the specifications of relays into directly executable functions in the appropriate machine language, and copies other "forms" (e.g., layout and signal definitions) into a compressed format in its output file.  It can also be asked to produce a listing, as well ("**-L**"), and compiled object files can be
listed *ex post facto*.

The whole idea is mainly for fun; ***compilation accrues no real benefit!***.  Although relay operations are
theoretically many times faster than the extant "interpreted" operations, the latter were already instantaneous as perceived by a human digital tower operator.  Loading may be a little faster, as there is no more interpretive evaluation of Lisp expressions of logic, and a considerable amount of storage of logic nodes and relay pointers is obviated, but this is of no consequence in a 64-bit address space.  Interestingly, the compiled code files seem 30 to 50 percent *larger* that the source code that produced them. One can see a typical logic term (i.e., relay contact) takes about 7 or 8 bytes, e.g., "561RWZ<code>&nbsp;</code>" (7 characters including the space), while any logic term on arm64 requires three instructions (each of which is a procrustean 32 bits, i.e., 4 bytes), hence 12 bytes.  x86 code is considerably more space-efficient, but arm64 gains speed and lookahead by this design choice.

Compiling code also renders it more difficult to debug interlockings, as there are no more circuits to be drawn or seen, but we have a
partial solution for that.  When the simulator is asked to draw a relay which is implemented by compiled code, it now produces,
to the Relay Draftsperson window, an annotated disassembly of the function -- minimal knowledge of arm64 is required to understand it.  Again, the whole feature benefits you nothing by its use.

The compiler and runtime support have been tested with Progman St., Atlantic Avenue, and IRT 240th St, the last of which is quite large.  Although all this work is checked in to this repository, no built (Mac) application has been uploaded to my site yet. Nor has Windows compilation been addressed (it should compile and work as it did before, even if compiled objects are not supported).

While either Mac build will offer to load **.tko** compiled objects in the **Open** dialog, the Intel build will validly complain that they were (of necessity) compiled for the wrong architecture, and not attempt to load any.  (The Mac app executables are "universal", supplying their corresponding "personality" on either (Mac) architecture).

There is a new console application, **DumpTko**, which can be used to inspect the sections of a compiled object, including the **TXT**
section, which contains the machine code (the others are needed symbol dictionaries, identification/time stamps, and so forth).  Ask **DumpTko** for its help info. The **DumpTko** listings are superior to those produced by the compiler itself, because the latter operates in one pass, and forward branches are missing their "displacements" until their targets are found, and appear so in the listing, with "fixups" later down the assembly.  **DumpTko** knows exactly what things are referenced (its disassembly skills are limited to the sequences currently emitted by the compiler).

The disassemblies produced by the simulator are the most useful, because they can be requested on a per-relay basis, by using the same
menu and dialog controls that normally produce circuit drawings.  About 300 disassembly lines fit in the virtual window currently available, which is
enough for almost all relays the lack of a way to exceed this is a current problem, although the largest relay known, **244H** at Atlantic Avenue fits *twice*..  This would be of little consequence, as full disassemblies are available via **DumpTko**, but in the live simulator context, both the relay requested and all its references to others are dynamically tagged with **(DROPPED)** or **(PICKED)** in the display, which updates dynamically as do the contacts in the drawings of interpreted scenarios, so this display can be used for debugging.

Reading the object code is not rocket science ; each logic term generates a three-instruction sequence consisting of a **ldr** instruction which loads a pointer to a referenced relay into register **x0**, a **ldrb** instruction that loads into **x0** the "State" byte of that relay, and a **tbz** or **tbnz** instruction that tests **x0**'s low bit and branches or not as asked. It is quite easy to reconcile it with S-expression code (or its circuit image) from which it was compiled.

    0001069ECAC4 F95BF840    ldr     x0, [x2, #0x37f0]        ; 357BNS  (DROPPED)
    0001069ECAC8 39400000    ldrb    x0, [x0, #0]
    0001069ECACC 37043320    tbnz    x0, #0, 0x1069E5130

*Ursus in mente* that **.tko**'s are code-containing files; the whole point is that the simulator executes machine instructions provided by them.  They are thus as corruptible and "dangerous" as any executable application, including the simulator, and must be treated with the same degree of caution and trust.  (It is not known if such activity is available on Windows; it once was, and stopped at one point.)

## Current deficiences

Lack of support (or even buildability/runnability testing) on Windows and on Intel Macs (both planned).

There is a current hard limit on 4K (4096) relays because of a 14-bit addressing limit to an array 8-byte (64 bit) pointers. Fixing this limit to be twice as large (8192), leveraging the signed nature of this 14-bit instruction field, would require a more complex strategy for the array of pointers by which compiled code references other relays, but the largest interlocking (240th St) today has only half that many (~2K).  Removing the limit entirely would require a serious redesign of the code strategy which will undoubtably incur costs in all compilations (e.g., additional instructions per logic term).  Like many ancient architectures, ARM64 draws a distinction between "easy" references to storage and "difficult" ones in order to optimize instruction bits.
