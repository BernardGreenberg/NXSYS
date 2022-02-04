# NXSYS
NXSYS New York Subway signalling and interlocking relay and panel simulator, for MacOS

#  NXSYS (v2)  sources for MacOS, complete
###  By and ©Copyright 1992-2022, Bernard S. Greenberg
#### First Posted 4 February 2022

This (on the **master** branch) is the buildable, runnable source code for **NXSYS** (*enn-ex-sys*), my New York Subway relay-logic and interlocking panel simulator, and its five offered interlockings, as it stands.  It is mine, and mine alone, although Dave Barraza's contributions to its debugging and development deserve mention.  Feel free to continue it, improve it if you like, or just build it for yourself.  I want this repository to be its home.  I'm not soliciting or merging branches at this time.

This application is posted and offered under the terms of the GNU General Public License Version 3. See the file `LICENSE` for details. These sources are offered "AS-IS", with no warranty or guarantee of operability or suitability for any purpose made or implied, and use is at your own risk — all the code is available here for inspection.

This code builds completely and cleanly under XCode 13.2.1 (13C100) under MacOS Monterey 12.1.  It builds for 64-bit Intel Mac; no configurations for M1 Macs are present. An `xcodeproj` file exists in the top directory; it contains no external references. **The Apple tool `PlistBuddy` is expected** to be in /usr/libexec (where it should be if XCode is properly installed); it is used for packing build number and date signature into the Info plist.  Tools for packaging the whole for distribution (BASH scripting) are not provided.  Code Signing is, of course, now your responsibility, if desired.

There remain (as expectable) some minor constraint irritations in the Interface  Builder layouts that do not impede or flag building.

The file `NXSYS.html` in the Documentation folder is a comprehensive description of the application and its capabilities. It is the source for the distributed PDF (although building that PDF so that external and internal links work seems to be a challenging research problem that depends on one's own computer). Read everything in that folder.

## Code languages and Windows version

This application is currently in C++11 (the 2011 standard of C++), exploiting STL.  The core of the functionality is in that language only, and, in principle, can run on MS Windows, for which it was originally written  (Win16 then Win32). It presently contains preprocessor conditionalizations to that end; it was ported to the Mac in late 2014.  The User Interface artifacts in this tree are in Objective C++ (embedded in C++11), an Apple proprietary storage-management regime and Object-Oriented GUI system which preceded Swift as Apple's preferred application development language, but still supported. Its syntax extends that of C++ incompatibly; it is well-documented, but difficult to master.  Also in Objective C++ is my original simulation of the Win32 API, which was my solution to maximizing the code that ran compatibly on both platforms.  Foolishly, I rejected the advice of those wiser than myself suggesting the use of Qt or other cross-platform GUI substrates, which may or may not have been easier, but would surely have introduced other problems.

The Windows build was last offered in 2016, and is fully operable. See [`https://BernardGreenberg.com/Subway`](https://BernardGreenberg.com/Subway) for a download.  There is at this time no `.vcproj` in this source tree, nor the handful of additional files needed to build the Windows version.  I don't have a newer Windows machine, and do not want an expensive Visual Studio subscription to build it.  I may find either or both and add them to this repository in the future, but for now it's "Macs all the way down".

I do take pains not to exploit newer features in distributed interlockings, and the scenario language has features that make it possible only to exploit newer features in newer builds.  All five of these interlockings are fully functional in the distributed (2016) Windows build. 

None of the OpenGL code of the popular erstwhile Cab View feature of Version 1 is present in this repository.  It cannot handle, nor be obviously extended to handle, the arbitrary track geometries of Version 2, and reaches aesthetic limitations at that point.

So Win32 NXSYS is frozen in time at 2016, at least for now, and cannot be easily built.  I don't want to buy another computer and a Visual Studio subscription just to be able to edit out this line.

## Status of Mac Version

I'm satisfied with the operability and reliability of the sources posted here.  I am actively thinking about changes necessary to support relay and track-section names more complicated than (the current) digits followed by alphanumerics, for example `A1-708, A1-708H` instead of `1708/1708H,` which would facilitate the representation of interlockings where two or more subway lines, with distinct stationing letters and origins, join, such as the incomparable E. 180th St. rebuild of 2013.  If I succeed, the changes will be compatible upon existent interlockings.

I may or may not fix reported bugs and post changes. Contact me via GitHub. I expect to post fixes to bugs I encounter from hereon in.

There are five build targets in this project:

- **NXSYSMac**. This is the Mac GUI application, complete with resources when built, and invocable as Mac GUI application.   It opens interlockings, displays control panels, accepts their operation, and runs scenarios.

- **TLEdit**.  The track-layout editor.  A separate Mac GUI application (A Windows version exists, too, but not buildable here) that allows the creation and maintenance of `.trk` files containing layout information (and only layout information).  This is how you create the track-maps/panel layouts of scenarios, not any part of their logic. It is (poorly) self-documenting.

- **Relay Compiler**. This is presently a joke, because even though this command-line application runs perfectly on the Mac, *it only produces Windows object code*.  It is simply not necessary with the speed of modern machines; interlockings run rapidly enough "interpreted" (Lisp taxonomy).  Prior to Mac NXSYS, though, it was quite a feat. It is a tribute to Apple that I have never had to learn the Mac machine-language environment.

- **BLISP** (B for Bernie).  A command-line program, being a test-build of the native quasi-Lisp reader (`readsexp.cpp`) used by NXSYS to read interlocking definitions.  `READ-PRINT` only, though, C(++)-no-`EVAL`. 10 cdr trains are handled :).

- **Relay Indexer**.  This recent innovation is another command-line C++11 program (no Objective C/C++) built from this tree that produces a text-format "relay index", a cross-reference of which relays are referenced as logic inputs by other relays, including "built-in" relays to the system. Direct it to the top-level file of an interlocking.  While this can be useful, it even more usefully produces an Emacs/Aquamacs `TAGS` table for all of the relays in the interlocking, allowing `meta-.` to be used with relays, and allowing relay source to be located from NXSYS' Relay Draftsperson. If you want to use `m-.` on relays, you must change the syntax of exclamation point (logic negation) in `.trk` buffers:  `(modify-syntax-entry ?! ".")` in your startup where appropriate.

The distribution of the sources into folders is somewhat chaotic, and I apologize in advance.  The subdirectory `NXSYS` is supposed to contain all sources (and headers) that are shared with the Windows build, although it includes some inexcusable others. But other directories contain Mac-only code and headers.

The TLEdit `buttons` directory contains, in addition to png's, Pixelmator (pre-Pro) files from which they were created.


## Et in fine....

Relay-based fixed-block wayside color-light signalling was in use in the New York Subways before *Titanic* was built, before anyone knew that silicon could be used for building anything except deserts and glass, and when "computer" meant an accountant. In the present century, newer, computer- and communications-based technologies have finally begun to make their need and presence felt there, and this stuff is ... *como se dice*, "a bit long in the tooth", like this author, who grew up with and learned to understand and admire it.  Nevertheless, the principles of safety design herein embodied survive even in the latest programmed-logic controller (PLC) plants, in New York and globally.  NXSYS has already brought pleasure and learning to many, and even inspired imitators.

Go to town, or Coney Island Yard, with it, but keep my name and credit as you add your own.  It is my invention, my "baby", my work, my gift, and, with my music, my legacy.

Thank you for interest ... enjoy,

### Bernard Greenberg
### 4 February 2022



