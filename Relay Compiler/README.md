# Relay Compiler Status -- Christmas 2024

## Last update 5 Jan 2025

The NXSYS Relay Compiler has been resurrected, but "certain restrictions apply". It can generate operative compiled interlockings (tested as complex as 240th St.) for both 64-bit architectures, Intel X86 (aka X86-64, X64 etc.) and Apple Silicon (ARM).  "Intel X86" applies to both "older Macs" and 64-bit MS-Windows (Windows 10, Windows 11) -- one binary serves for both the latter! -- but Windows has not yet been tested (simulator not yet built. See below.).  There is residual 32-bit (and even 16-bit (as the compiler was born in 1994) Windows code generation available, but it is mainly a joke, because there are no longer any 32- ior 16-bit builds of NXSYS with the loading code extant. 

The Relay Compiler takes the same "pseudo-Lisp" descriptions of interlockings as does NXSYS proper (henceforth, "the simulator") and creates one "object file" (**.tko**) for an entire interlocking.  It compiles the specifications of relays into directly executable functions in the appropriate machine language, and copies other "forms" (e.g., layout and signal definitions) into a compressed format in the object file.  It can also be asked to produce a listing, as well ("**-L**"), and compiled object files can be
listed *ex post facto*.

The whole idea is mainly for fun; ***compilation accrues no real benefit!***.  Although relay operations are
theoretically many times faster than the extant "interpreted" operations, the latter were already instantaneous as perceived by a human digital tower operator.  Loading may be a little faster, as there is no more interpretive evaluation of Lisp expressions of logic, and a considerable amount of storage of logic nodes and relay pointers is obviated, but this is of no consequence in a 64-bit address space.  Interestingly, the compiled code files seem 30 to 50 percent *larger* that the source code that produced them. One can see a typical logic term (i.e., relay contact) takes about 7 or 8 bytes, e.g., "561RWZ<code>&nbsp;</code>" (7 characters including the space), while any logic term on arm64 requires three instructions (each of which is a procrustean 32 bits, i.e., 4 bytes), hence 12 bytes.  x86 code is only slightly more space-efficient, requiring typically 11 bytes per logic term, but arm64 gains speed and lookahead by this design choice.

Compiling code also renders it more difficult to debug interlockings, as there are no more circuits to be drawn or seen, but we have a
partial solution for that.  When the simulator is asked to draw a relay which is implemented by compiled code, it now produces,
to the Relay Draftsperson window, an annotated disassembly of the function -- minimal knowledge of arm64 is required to understand it. (X86 disassembly, which is famously difficult, is not there yet, either). Again, the whole feature benefits you **nothing** by its use.

The compiler and runtime support have been tested with Progman St., Atlantic Avenue, and IRT 240th St, the last of which is quite large.  Although all this work is checked in to this repository, no built (Mac) application has been uploaded to my site yet. Nor has Windows compilation been addressed (it should compile and work as it did before, even if compiled objects are not supported).

While either Mac build will offer to load **.tko** compiled objects in the **Open** dialog, either build will complain when given a track object for the other architecture, and not attempt to load it.  (While the Mac app executables are "universal", supplying their corresponding "personality" on either (Mac) architecture, the TKO's are not - it would double their size for very little benefit).

There is a new console application, **DumpTko**, which can be used to inspect the sections of a compiled object, including the **TXT**
section, which contains the machine code (the others are needed symbol dictionaries, identification/time stamps, and so forth).  Ask **DumpTko** for its help info. The **DumpTko** listings are superior to those produced by the compiler itself, because the latter operates in one pass, and forward branches are missing their "displacements" until their targets are found, and appear so in the listing, with "fixups" later down the assembly.  **DumpTko** knows exactly what things are referenced (its disassembly skills are limited to the sequences currently emitted by the compiler). Again, it can't disassemble X86 yet.

The disassemblies produced by the simulator are the most useful, because they can be requested on a per-relay basis, by using the same
menu and dialog controls that normally produce circuit drawings.  About 300 disassembly lines fit in the virtual window currently available, which is enough for almost all relays the lack of a way to exceed this is a current problem, although the largest relay known, **244H** at Atlantic Avenue fits *twice*..  This would be of little consequence, as full disassemblies are available via **DumpTko**, but in the live simulator context, both the relay requested and all its references to others are dynamically tagged with **(DROPPED)** or **(PICKED)** in the display, which updates dynamically as do the contacts in the drawings of interpreted scenarios, so this display can be used for debugging. Again, doesn't disassemble Intel code, but stupidly displays it as 32-bit words "in the wrong byte-order", imagining they are unknown ARM instructions.

Reading the object code is not rocket science ; each logic term generates (on ARM) a three-instruction sequence consisting of a **ldr** instruction which loads a pointer to a referenced relay into register **x0**, a **ldrb** instruction that loads into **x0** the "State" byte of that relay, and a **tbz** or **tbnz** instruction that tests **x0**'s low bit and branches or not as asked. It is quite easy to reconcile it with S-expression code (or its circuit image) from which it was compiled.

    0001069ECAC4 F95BF840    ldr     x0, [x2, #0x37f0]        ; 357BNS  (DROPPED)
    0001069ECAC8 39400000    ldrb    x0, [x0, #0]
    0001069ECACC 37043320    tbnz    x0, #0, 0x1069E5130

*Ursus in mente* that **.tko**'s are code-containing files; the whole point is that the simulator executes machine instructions provided by them.  They are thus as corruptible and "dangerous" as any executable application, including the simulator, and must be treated with the same degree of caution and trust.  (It is not known if such activity is available on Windows; it once was, and stopped at one point.)

Intel code is a bit simpler, because it does not need the arcane **ldrb**. **cl** alwas has **1** in it.  Windows and Mac use different 17-word "entry thunks" to compiled code, which account for differing ABI (Application Binary Interface) calling sequences, but the relay functions are identical.

    00000139 498B9088000000 c$267RWK: mov     rdx,QWORD PTR [r8+v$267RWC]
    00000137 840A                     test    BYTE PTR [rdx],cl
    00000139 74F4                     jz      retval

## What is "JIT" and why is it relevant?

**JIT** stands for "just in time", which is jargon for the technique, usually used by a debugger, of generating new machine instructions, and having them executed, usually not immediately, but more often in the instruction stream of the program being debugged.  Of course, this can be very dangerous if a malicious program generates such instructions and patches them into your instruction stream, but the malicious program could simply wreak the intended havoc without generating instructions.  For this reason, modern operating systems impose roadblocks to writing executable code.  The MS-Windows API for doing this stopped working for me in 2003 (11 years before the first Mac version), and that occasioned the decommissioning of the relay compiler at that time.  The Mac APIs for this work perfectly for me om my Apple Silicon Mac, but fail silently (the attempt to write takes a fault) on my Intel Mac running the same OS.

The archival 32-bit Windows **.tko** loader contains this commented-out code:
    #if 0
    /* STarted complaining in Win2k 11 August 2003 */
        if (!VirtualProtect (Text_ptr, Text_len, PAGE_EXECUTE_READ, &oldacc))
	    barff ("VirtualProtect of code area fails: 0x%X", GetLastError());
    #endif

## Current deficiences

**MS-WINDOWS** Lack of building/testing on platform; generated code is expected to work flawlessly, but the current JIT status and issues have neither been investigated, addressed, nor coded for.

The situation may or may not be better now.  It is unreasonable for this feature to require special user action or configuation.

**Intel Mac** As noted, the JIT API (**pthread_jit_write_protect_np**) fails silently. It does not have a return value.  I have tried all nature of tomfoolery with Capabilities and Entitlements and logging in Admin, but have not been able to overcome this, and some wisdom on the web even suggests that Apple disables this feature for Intel Macs, although I can find no documentation attesting to this or ways of dealing with it.  Research continues, but anyone with knowledge of this, please contact me via GitHub here.

It is possible to demonstrate the X86 compiler and correct functioning of X86-compiled interlockings on an Apple Silicon Mac.  It is silly, but it actually works, and demonstrates this path!  The compiler will compile for any supported architecture on any supported platform:  **-arch:X86-64** will request a compilation for that platform.  To run the resultant track object file, use the Apple-suppled **arch** command to run the (Universal!) NXSYS executable directly (**../NXSYSMac/Contents/MacOS/NXSYSMac**) by pathname, and the title bar will confirm that is running the **(Intel)** build!  From there, you can load Intel-compiled track objects, but not native ARM ones! (Note that debugging executables are not universal; you must use **Product | Archive**).

It is possible to imagine creating **.dylib**s whose guts are replaced by compiled code, but this is an awful lot to ask for a feature which is basically useless...

**ARM Mac** Works like a charm, in all respects, but there is a current hard limit on 4K (4096) relays because of a 14-bit addressing limit on ARM to an array 8-byte (64 bit) pointers. Fixing this limit to be twice as large (8192), leveraging the signed nature of this 14-bit instruction field, would require a more complex strategy for the array of pointers by which compiled code references other relays, but the largest interlocking (240th St) today has only half that many (~2K).  Removing the limit entirely would require a serious redesign of the code strategy which will undoubtably incur costs in all compilations (e.g., additional instructions per logic term).  Like many ancient architectures, ARM64 draws a distinction between "easy" references to storage and "difficult" ones in order to optimize instruction bits.
