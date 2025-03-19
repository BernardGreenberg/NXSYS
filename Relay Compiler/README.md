# Relay Compiler Status -- Christmas 2024

## Last update 18 Mar 2025

## Prefatory Notes

1. This feature, although first dating from 1994, **adds no useful functionality to the NXSYS application at all**.  It is all for "fun".  There are no relays with hundreds or thousands of logic terms in their formulae that would stand to benefit significantly in computation speed from compilation.

2. In the present document I use the term **simulator** to refer to the application NXSYS proper, that simulates electrical interlocking panels, relay systems, and signalling, to distinguish it from **TLEdit** or the relay compiler or disassembler (**DumpTko**) I use term **emulator** (and **emulation**) to refer to the software code, or its action, that simulates one processor through instructions for another. There are three such involved here, one written by Apple on MacOS (**Rosetta2**), one by Microsoft in Windows 11(**Prism**), and one inbuilt to NXSYS.  In all three cases, the architecture being emulated in the Intel 64-bit X86 by code running in the ARM architecture.

3. Note that this stuff all works in the checked-in Github tree right now; if you clone the tree and build the apps, which works for both platforms, this is all usable.  This affects the NXSYS app proper (henceforth, "the simulator"), and the "new" apps **Relay Compiler** and **DumpTko**, which are Schemes on the Mac and VC projects on Windows. There are no built executables or installers yet.

## NXSYS Compiled Code system


The NXSYS Relay Compiler has been resurrected. It can generate operative compiled interlockings for both 64-bit architectures, Intel X86 (aka X86-64, X64 etc.) and Apple Silicon (ARM), thus, Mac and Windows.  "Intel X86" applies to both "older Macs" and 64-bit MS-Windows (Windows 10, Windows 11) -- one binary serves for both the latter, and can be produced on either Platform or architecture. There is residual 32-bit (and even 16-bit, as the compiler was born in 1996) Windows code generation available, but it is mainly a joke, because there are no longer any 32- or 16-bit builds of NXSYS extant with the requisite loading code. Interlockings as complex as 240th St and Atlantic Avenue work properly compiled.

The Relay Compiler takes the same "pseudo-Lisp" descriptions of interlockings as does the simulator and creates one "object file" (**.tko**) for an entire interlocking.  It compiles the specifications of relays into directly executable functions in the appropriate machine language, and copies other "forms" (e.g., layout and signal definitions) into a compressed format in the that file.  It can also be asked to produce a listing, as well ("**-L**"), and compiled object files can be listed *ex post facto* with the new **DumpTko** tool.

Via compilation, a considerable amount of storage of logic nodes and relay pointers is obviated, but this is of no consequence in a 64-bit address space.  Interestingly, the compiled code files seem 30 to 50 percent *larger* than the source code that produced them. One can see a typical logic term (i.e., relay contact) takes about 7 or 8 bytes, e.g., "561RWZ<code>&nbsp;</code>" (7 characters including the space), while any logic term on arm64 requires three instructions (each of which is a procrustean 32 bits, i.e., 4 bytes), hence 12 bytes.  x86 code is only slightly more space-efficient, requiring typically 11 bytes per logic term, but arm64 gains speed and lookahead by this design choice.

Compiling interlockings also renders them more difficult to debug, as there are no more circuits to be drawn or seen, but we offer a partial compensation for that.  When the simulator is asked to draw a relay which is implemented by compiled code, it now produces, to the Relay Draftsperson window, an annotated disassembly of the function -- minimal knowledge of the involved machine code is required to understand it (see below). It is not difficult to imagine reconstructing the circuit diagrams from the compiled code, for what little that is worth. (It can't yet be scrolled on Windows).

The compiler and runtime support have been tested with Progman St., Atlantic Avenue, and IRT 240th St, the last of which is quite large.  Although all this work is checked in to this repository, no built  application or installer for either platform has yet been uploaded to my site.

While either Mac build will offer to load **.tko** compiled objects in the **Open** dialog, the Intel build will refuse to run arm64-compiled interlockings, while the arm64 build will run X86-compiled interlockings in emulation (see below).  While the Mac app executables are "universal", supplying their corresponding "personality" on either (Mac) architecture, the TKO's are not - it would double their size for very little benefit.

Note also that very modern Windows platforms, such as the Microsoft "Surface", actually employ ARM processors, but Microsoft supplies an in-operating-system emulator that is so terrific that users are unaware of it. NXSYS for Windows is an Intel application, and it uses (if you use the feature) Intel **.tko**s.  There is no native Windows ARM NXSYS, or corresponding executable to a Mac "Universal binary".

There is a new console application, **DumpTko**, which can be used to inspect the sections of a compiled object, including the **TXT** section, which contains the machine code (the others are needed symbol dictionaries, identification/time stamps, and so forth).  Ask **DumpTko** for its help info (**--help**). The **DumpTko** listings are superior to those produced by the compiler itself, because the latter operates in one pass, which deprives forward branches of their "displacements" until their targets are found, and appear so in the listing, with "fixups" later down the assembly.  **DumpTko** knows exactly what things are referenced (its disassembly skills are limited to the sequences currently emitted by the compiler). It can disassemble X86-64 or arm64 **tko**'s'.

The disassemblies produced by the simulator are the most useful, because they can be requested on a per-relay basis, by using the same menu and dialog controls that normally produce circuit drawings.  About 300 disassembly lines fit in the virtual window currently available (on the Mac), which is enough for almost all relays the lack of a way to exceed this is a current problem, although the largest relay known, **244H** at Atlantic Avenue fits *twice*.  This would be of little consequence, as full disassemblies are available via **DumpTko**, but in the live simulator context, both the relay requested and all its references to others are dynamically tagged with **(DROPPED)** or **(PICKED)** in the display, which updates dynamically as do the contacts in the drawings of interpreted scenarios, so this display can be used for debugging. It works for either architecture or platform.  Relays running in Intel simulation (of both kinds) will be correctly disassembled as Intel code.

Reading the object code is not rocket science ; each logic term generates (on ARM) a three-instruction sequence consisting of a **ldr** instruction which loads a pointer to a referenced relay into register **x0**, a **ldrb** instruction that loads into **x0** the "State" byte of that relay, and a **tbz** or **tbnz** instruction that tests **x0**'s low bit and branches or not as asked. It is quite easy to reconcile it with S-expression code (or its circuit image) from which it was compiled.

    0001069ECAC4 F95BF840    ldr     x0, [x2, #0x37f0]        ; 357BNS  (DROPPED)
    0001069ECAC8 39400000    ldrb    x0, [x0, #0]
    0001069ECACC 37043320    tbnz    x0, #0, 0x1069E5130

Intel code is a bit simpler, because it does not need the arcane **ldrb**. **cl** alwas has **1** in it.  Windows and Mac use different 17-word "entry thunks" to compiled code, which account for differing ABI (Application Binary Interface) calling sequences, but the relay functions are identical.

    00000139 498B9088000000 c$267RWK: mov     rdx,QWORD PTR [r8+v$267RWC]
    00000140 840A                     test    BYTE PTR [rdx],cl
    00000142 74F4                     jz      retval

*Ursus in mente* that **.tko**'s are code-containing files; the whole point is that the simulator executes machine instructions provided by them.  They are thus as corruptible and "dangerous" as any executable application, including the simulator, and must be treated with the same degree of caution and trust. See below --

## What is "JIT" and why is it relevant?

**JIT** stands for "just in time", which is jargon for the technique, usually used by a debugger, of generating new machine instructions, and having them executed, usually not immediately, but more often in the instruction stream of the program being debugged.  Of course, this can be very dangerous if a malicious program generates such instructions and patches them into your instruction stream, but the malicious program could simply wreak the intended havoc without generating instructions. NXSYS uses JIT techniques (JIT-jiusu?) to run its compiled code.

## Current restritions

**INTEL MAC** Intel-compiled (compiled *for* Intel) interlockings will run on the Intel NXSYS build when loaded from the **Open** dialog or **Recent Files** menu. It cannot run arm64-compiled interlockings, and will complain if you try to load one.  There are no known problems in this path.  Windows won't load them, either.

**ARM MAC** There is a current hard limit on 4K (4096) relays because of a 14-bit addressing limit on ARM to an array 8-byte (64 bit) pointers. Fixing this limit to be twice as large (8192), leveraging the signed nature of this 14-bit instruction field, would require a more complex strategy for the array of pointers by which compiled code references other relays, but the largest interlocking (240th St) today has only half that many (~2K).  Removing the limit entirely would require a serious redesign of the code strategy which will undoubtably incur costs in all compilations (e.g., additional instructions per logic term).  Like many ancient architectures, ARM64 draws a distinction between "easy" references to storage and "difficult" ones in order to optimize instruction bits.

## Emulation

There are two forms of Intel-emulation available to arm64 NXSYS, one provided by Apple ("Rosetta2") and one provided by the application itself.  The compiler will cross-compile for any supported architecture on any supported platform:  **-arch:X86-64** will request a compilation for that platform.  Either mode of emulation will allow you to run X86-compiled TKO's on arm64 NXSYS.

First, you can run NXSYS itself in emulation  with Rosetta2. Use the Apple-suppled **arch** command to run the (Universal!) NXSYS executable under Rosetta2 simulation directly (**..../NXSYSMac.app/Contents/MacOS/NXSYSMac**) by pathname,specifying as a first argument **-X86_64**, (leading hyphen, middle underscore). The NXSYS title bar will confirm that is running the **(Intel)** build!  From there, you can load Intel-compiled track objects, but not native ARM ones! (Note that XCode debugging executables are not "Universal" (i.e., two executables packed as one); you must use **Product | Archive** to build a universal executable).

Windows ARM platforms have such an emulator built in and active at all times.

Second, it is possible to ***run X86-compiled relay code in the Mac Arm application version with its native emulation***. If you try to load such an interlocking into NXSYS (i.e., the Arm build) it will do so without need for being told. The emulation is extremely efficient and robust, as both the simulator (i.e., the NXSYS program itself) and the two disassemblers are acutely aware of the limited output vocabulary of the compiler, and exploit it in time and space.  The title bar will say **X86/Sim** when this is in effect.

Under either kind of emulation, NXSYS **Draw Relay** will show X86 code, just as in the Intel build. Note that such displays use the Intel notation (e.g., **mov rdx,rsi**) not the AT&T notation preferred by Apple (**movq %rsi,%rdx** -- note the reversed operands!).

The Intel builds (Mac or Windows) do not offer access to the emulator (which only emulates Intel, not ARM -- Apple doesn't supply a "2attesoR", either).  Remember that the traditional way to use NXSYS is with interpreted list-structure code (**.trk**), not compiled objects at all!

