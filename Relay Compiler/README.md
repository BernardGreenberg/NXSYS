# Relay Compiler Status -- Christmas 2024

The NXSYS Relay Compiler has been resurrected, both in general and specifically for the arm64 architecture (Mac M1/M2/M3 etc.) 
There are now three architectures to deal with, Windows Intel X86, Mac Intel X86, and Mac arm64 ("New Macs"). Only the last is currently (26 December 2024) fully implemented and tested.  There is residual 32-bit Windows code generation available, but it is mainly a joke, because current Windows is 64-bit.  It might be salvageable, though, and the compiler thinks it is multi-homed/multi-targeted.

The Relay Compiler takes the same "pseudo-Lisp" descriptions of interlockings as does NXSYS proper (henceforth, "the simulator"),
compiling the specification of relays into functions in the appropriate machine language, and copying other "forms" into a compressed
format in the output file it constructs.  It can be asked to produce a listing, as well ("**-L**"), and compiled object files can be
listed *ex post facto*.  The whole idea is mainly for fun; ***compilation accrues no real benefit!***.  Although relay operations are
theoretically many times faster, they were already instantaneous as observed by a human digital tower operator.  Loading may be a little faster, as there is no more interpretive evaluation of Lisp expressions of logic, and a considerable amount of storage of logic nodes and relay pointers is obviated.  Interestingly, the compiled code files seem 30 to 50 percent *larger* that the source code that produced it. One can see a typical logic term takes about 7 or 8 bytes, e.g., "561RWZ " (7 characters including the space), while any logic term on arm64 requires three instructions (each of which is  procrustean 32 bites, i.e., 4 bytes), hence 12 bytes.  x86 code is considerably more space-efficient, but arm64 gains speed and lookahead by this design choice.

With compiled code, it is more difficult to debug interlockings, as there are no more circuits to be drawn or seen, but we have a
partial solution for that.  When the simulator is asked to draw a relay which is implemented by compiled code, it now produces,
to the Relay Draftsperson window, an annotated disassembly of the function -- minimal knowledge of arm64 is required to understand it.  Again, the whole feature benefits you nothing by its use.

It has been tested with Progman St., Atlantic Avenue, and IRT 240th St, the last of which is quite large.  This is all checked in to this repository, but no built (Mac) application has been uploaded to my site yet.

While either Mac build will offer to load **.tko** compiled objects in the **Open** dialog, the Intel build will complain that they were (of necessity) compiled for the wrong architecture, and go no further.  (The Mac app executables are "universal", supplying their corresponding "personality" on either (Mac) architecture).

There is a new console application, **DumpTko**, which can be used to inspect the sections of a compiled object, including the **TXT**
section, which contains the machine code (the others are needed symbol dictionaries, identification/time stamps, and so forth).  Ask **DumpTko** for its help info. The **DumpTko** listings are superior to those produced by the compiler itself, because the latter operates in one pass, and forward branches are missing their "displacements" until their targets are found, and appear so in the listing, with "fixups" later down the assembly.  **DumpTko** knows exactly what things are referenced (its disassembly skills are limited to the sequences currently emitted by the compiler).

The disassemblies produced by the simulator are the most useful, because they can be requested on a per-relay basis, from the same
controls that normally produce circuit drawings.  About 100 disassembly lines fit in the virtual window available, which is
enough for almost all relays (33 logic terms); the lack of a way to exceed this is a current problem (**244H** at Atlantic Avenue is a distressing example of one that does not).  This would be of little consequence, as full disassemblies are available via **DumpTko**, but in the live simulator context, both the relay requested and all its references to others are dynamically tagged with **(DROPPED)** or **(PICKED)** in the display, which updates dynamically as do the contacts in the drawings of interpreted scenarios, so this display can be used for debugging and the current length limitation can be a problem.

Reading the object code is not rocket science ; each logic term generates a three-instruction sequence consisting of a **ldr** instruction which loads a pointer to a referenced relay into register **x0**, a **ldrb** instruction that loads into **x0** the "State" byte of that relay, and a **tbz** or **tbnz** instruction that tests **x0**'s low bit and branches or not as asked. It is quite easy to reconcile it with S-expression code (or its circuit image) from which it was compiled.

*Ursus in mente* that **.tko**'s are code-containing files; the whole point is that the simulator executes machine instructions provided by them.  They are thus as corruptible and "dangerous" as any executable application, including the simulator, and must be treated with the same degree of caution and trust.  (It is not known if such activity is available on Windows; it once was, and stopped at one point.)

