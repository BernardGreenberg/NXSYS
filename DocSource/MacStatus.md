# NXSYS Mac build status
#### 24 February 2022 rev 1 Dec 2024

This code builds completely and cleanly under XCode 16.1 (16840) under macOS Sequoia 15.1.1, for 64-bit M3 (Apple Silicon) Mac; As of 2.7.3, the build is currently configured to create "Universal" binaries, i.e., paired "fat" binaries that work on Intel or Apple Silicon. The target architecture (minimum runtime OS) is configured as 10.15 (Catalina, from year 2020).  An [`xcodeproj` folder](https://github.com/BernardGreenberg/NXSYS/tree/master/NXSYSMac.xcodeproj) resides in the top folder; it contains no external references. **The Apple tool [`PlistBuddy`](https://www.marcosantadev.com/manage-plist-files-plistbuddy/) is expected** to be in `/usr/libexec` (where it should be if XCode is properly installed); it is used for packing build number and date signature into the Info plist.  Tools for packaging the whole for distribution (Bash and Python scripting) are provided in the `MacPackagingTool` directory; you might likely want to change them.  Code Signing is, of course, now your responsibility, if desired.

There remain (as expectable) some minor constraint irritations in the Interface Builder layouts that do not impede building or raise warning flags at build time; there are none. XCode also suggests categoric upgrade to its coding preferences, of which I will opt out, for fear of disturbing the Windows build.

The Mac User Interface artifacts in this tree are in Objective C++ (embedded in C++20), an Apple proprietary storage-management regime and Object-Oriented GUI system which preceded Swift as their   preferred application development language, but still supported. Its syntax extends that of C++ incompatibly; it is well-documented, but difficult to master.  Also in Objective C++ is my original simulation of the Win32 API, which was my solution to maximizing the code that ran compatibly on both platforms.  Perhaps foolishly, I rejected the advice of those wiser than myself suggesting the use of Qt or other cross-platform GUI substrates, **which may or may not have been easier**, but would surely have introduced other problems.

## The XCode project

The Mac Version is targeted in XCode to (minimum) [macOS Mojave (10.14)](https://en.wikipedia.org/wiki/MacOS_Mojave) (released Sept. 2018); that is the SDK level it uses.   While this can easily be set as current as you wish in XCode, and most Mac users keep their systems up-to-date, I see no reason to bring it closer to currency: back-compatibility is a virtue. 
It all builds and works without error or warnings, as far as I know (and my goal is to keep that true).

I may or may not fix reported bugs and post changes, but I want to know if you can't build it; contact me via GitHub. I expect to post fixes to bugs I encounter and gratuitous enhancements from hereon in.

The entire `Documentation` and `Interlockings` directories get dumped into `Resources` and “shipped”. Be careful what you put there.  As on Windows, the files `InterlockingLibrary.xml` and `Help.xml` present the intended contents in the application menu.

## Build targets

There are five in this project:

- **NXSYSMac**. This is the Mac GUI application, complete with resources when built, invocable as a standard `.app` Mac GUI application.

- **TLEdit**, the track-layout editor.  A separate Mac GUI application that allows the creation and maintenance of `.trk` files containing layout information (and only layout information).

- **Relay Compiler**. This is presently a joke, because even though this command-line application runs perfectly on the 64-bit Mac, *it only produces Windows object code*, 32- as well as 16-bit, actually — NXSYS Windows is now **64 bit** (as is the Mac system) (but Intel 64 and 32 architectures are very similar).  It is simply no longer necessary with the speed of modern machines; interlockings run rapidly enough “interpreted” (Lisp taxonomy).  Prior to Mac NXSYS, though, it was quite neat, and it hurts me to delete it. It is a tribute to Apple that I have never had to learn the Mac machine-language environment.  (Also, modern operating systems are averse to apps creating executable code).

- **BLISP** (B for Bernie).  A command-line program, being a test-build of its native reader (`readsexp.cpp`) for the quasi-Lisp used for interlocking definitions.  `READ-PRINT` only, though, C(++)-no-`EVAL`. 10 cdr trains are handled :).

- **Relay Indexer**. There is now an [extensive separate help file on this](https://github.com/BernardGreenberg/NXSYS/blob/master/Relay%20Index/RelayIndex.md) now. It now works on Windows and Mac (Universal, i.e., Intel and arm64).

The distribution of the sources into XCode "groups" is somewhat chaotic, and I apologize.  The [folder `NXSYS`](https://github.com/BernardGreenberg/NXSYS/tree/master/NXSYS) contains all code of the main application (and headers) that is shared with the Windows build.  `TLEdit/tled` contains the *additional* code in TLEdit shared with Windows version. And `NXSYSWindows` contains all Windows-only code and other artifacts.  All other folders contain Mac-only code and headers.  There are still some glitches with TLEdit Windows-only code, so it does not appear in XCode, but does appear in the Repository.

The TLEdit `buttons` folder (in the Repository) contains, in addition to .png's for its (Mac) tool-panel buttons, Pixelmator (pre-Pro) files from which they were created.

