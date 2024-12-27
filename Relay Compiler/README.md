# Relay Compiler Status -- Christmas 2024

The NXSYS Relay compiler has been resurrected, both in general and specifically for the arm64 architecture (Mac M1/M2/M3 etc.) 
There are now three architectures to deal with, Windows Intel X86, Mac Intel X86, and Mac arm64 ("New Macs"). Only the last is currently
(26 December 2024) implemented and tested.  There is residual 32-bit Windows code generation available, but it is mainly a joke,
because current Windows is 64-bit.  It might be salvageable, though, and the compiler thinks it is multi-homed/multi-targeted.

The Relay Compiler takes the "pseudo-Lisp" descriptions of interlockings the same as NXSYS proper (henceforth, "the simulator"),
compiling the descriptions of relays into functions in the appropriate machine language, and copying other "forms" into a compressed
format in the output file it constructs.  It can be asked to create a listing, as well, and compiled object files can be
listed *ex post facto*.The whole idea is mainly for fun; compilation brings no real benefit.  Although relay operations are
theoretically many times faster, they were already instantaneous as observed.  Loading may be a little faster, as there is no more
interpretive evaluation of Lisp expressions of logic, and a considerable amount of storage of logic nodes and relay pointers is 
obviated.  Interestingly, the compiled code files seem 30 to 50 percent *larger* that the source code that produced it. One can see
a typical logic term takes about 7 or 8 bytes, e.g., "561RWZ " (including the space), while any logic term on arm4 takes three
instructions (each of which is 4 bytes), hence 12 bytes.  x86 code is considerably more space-efficient, but arm4 gains speed and
lookahead by this move.

With compiled code, it is more difficult to debug interlockings, as there are no more circuits to be drawn or seen, but we have a
partial solution for that.  When the simulator is asked to draw a relay which is implemented by compiled code, it now produces,
to the Relay Draftsperson window, an annotated disassembly of the function (some knowledge of arm64 required).  Again, it gains
you nothing to use it.

It has been tested with Progman St., Atlantic Avenue, and IRT 240th St, which is quite large.  This is all checked in to this
repository, but no built (Mac) application has been uploaded to my site yet.

Either Mac build will offer to load **.tko** compiled objects, but the Intel Mac build will complain that they were (of necessity)
compiled for the wrong architecture, and go no further.  (The executable is "universal" and supplies its corresponding "personality"
on either architecture.

There is a new console application, **DumpTko**, which can be used to inspect the sections of a compiled object, including the **TXT**
section, which contains the machine code (the others are needed symbol dictionaries and so forth).  Ask it for its help info. 
The **DumpTko** listings are superior ton those produced by the compiler itself, because the latter operates in one pass, and forward
branches are incompletely compiled until their targets are found, and appear so in the listing, with "fixups" later down
the assembly.  **DumpTko** knows exactly what things are referenced (its disassembly skills are limited to the operations
emitted by the compiler).

The disassemblies produced by the simulator are the most useful, because they can be requested on a per-relay basis, from the same
controls that normally produce circuit drawings.  About 100 disassembly lines fit in the virtual window available, which is
enough for almost all relays (33 logic terms); the lack of a way to exceed this is a current problem.

Reading the object code is very easy; each three-instruction sequence consists of a **ldr** instruction which loads a pointer to
a referenced relay into register x0, a **ldrb** instruction that loads into x0 the "State" byte of that relay, and a **tbz** or **tbnz**
instruction that tests its low bit and branches or not. It is quite easy to reconcile it with S-expression code or circuit
image from which it was compiled.

