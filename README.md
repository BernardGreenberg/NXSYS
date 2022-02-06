#  NXSYS sources, complete
###  By and ©Copyright 1994-2022, Bernard S. Greenberg
#### First Posted 4 February 2022

This is the buildable, runnable source code for **NXSYS** (*enn-ex-sys*) (Version 2), my New York City Subway relay-logic and interlocking panel simulator, and its five offered interlockings, as it stands.  It is mine, and mine alone, although Dave Barraza's contributions to its debugging and development deserve mention.  Feel free to continue it, improve it if you like, or just build it for yourself.  I want this repository to be its home.  I'm not soliciting or merging branches at this time.

This application is posted and offered under the terms of the GNU General Public License Version 3. See the file `LICENSE` for details. These sources are offered "AS-IS", with no warranty or guarantee of operability or suitability for any purpose made or implied, and use is at your own risk — all the code is available here for inspection.  **Do not** attempt to use this code to control actual railroads with live crews and passengers (I would be *extremely* impressed and flattered, but not responsible for the outcome).

This codebase is presently maintained and targeted for macOS, using XCode as a build platform.  While the MSWindows (10) version is available, it is not now buildable. See [Code languages and the Windows version](#code-languages-and-the-windows-version) below for more info on its status and future.

This code builds completely and cleanly under XCode 13.2.1 (13C100) under macOS Monterey 12.1, for 64-bit Intel Mac; no configurations for M1 Mac are present. An `xcodeproj` file-set resides in the top folder; it contains no external references. **The Apple tool `PlistBuddy` is expected** to be in /usr/libexec (where it should be if XCode is properly installed); it is used for packing build number and date signature into the Info plist.  Tools for packaging the whole for distribution (BASH scripting) are not provided (XCode `Product>Archive` already does most of the job).  Code Signing is, of course, now your responsibility, if desired.

There remain (as expectable) some minor constraint irritations in the Interface  Builder layouts that do not impede building or raise warning flags at build time.

The file `NXSYS.html` in the Documentation folder is a comprehensive description of the application and its capabilities. It is the source for the distributed PDF (although building that PDF so that external and internal links work seems to be a challenging research problem that depends on one's own computer). Read everything in that folder.  See [Related Resources](#related-resources) below for a video demo.

Please ignore admonitions in the interlocking and other help texts about not redistributing without permission (it's public now), and old email addresses there as well; contact me via GitHub if need be.  Unless you do real development, there shouldn't be any need to redistribute as long as this repository exists.

I am not, and have not been, associated with any transportation provider nor signal engineering contractor, nor has any endorsed this work, although during its history quite a few people such people have contributed knowledge, critique, and fanship.

## Code languages and the Windows version

This application is currently in C++11 (the 2011 standard of C++), exploiting STL.  The core of the functionality is in that language only, and, in principle, can run on MS Windows, for which it was originally written  (Win16 then Win32). It presently contains preprocessor conditionalizations to that end; it was ported to the Mac in late 2014, and back to Windows in 2016.  The User Interface artifacts in this tree are in Objective C++ (embedded in C++11), an Apple proprietary storage-management regime and Object-Oriented GUI system which preceded Swift as their   preferred application development language, but still supported. Its syntax extends that of C++ incompatibly; it is well-documented, but difficult to master.  Also in Objective C++ is my original simulation of the Win32 API, which was my solution to maximizing the code that ran compatibly on both platforms.  Foolishly, I rejected the advice of those wiser than myself suggesting the use of Qt or other cross-platform GUI substrates, which may or may not have been easier, but would surely have introduced other problems.

The Windows build was last offered in 2016, and while fully operable, is not now buildable.  See [Related Resources](#related-resources) below for a download of the executable.  For a more detailed description of the status of the Windows version, including its complete source tree, see the [MSWindows2016 folder](https://github.com/BernardGreenberg/NXSYS/tree/master/MSWindows2016) in this repository, and read the [README.md](https://github.com/BernardGreenberg/NXSYS/tree/master/MSWindows2016#readme) in it. It is thus frozen in time in 2016 for now (the Mac version has surpassed it).

I do take pains in distributed interlockings to avoid incompatible use of new features; the scenario language makes it possible to exploit newer features only in newer builds, downward compatibly.  All five of these interlockings are thus fully functional in the distributed (2016) Windows build.  The interlockings here in the eponymous folder are newer and better than those in the distributions.

None of the OpenGL code of the popular erstwhile Cab View feature of Version 1 is present in this repository.  It cannot handle, nor can it be obviously extended to handle, the arbitrary track geometries of Version 2. And in a world where spectacular interactive animated graphics such as [this (threejs)](https://threejs.org/examples/#webgl_animation_keyframes) are available, Cab View's black, grey-walled tunnels with only rails and signals visible would be ... inadequate. But becoming the video-game designer required is not a credible use of my time, nor would the result be a reasonable extension of the purview of this application.
 
## Status of the Mac Version

The Mac Version is targeted in XCode to (minimum) [macOS Sierra (10.12)](https://en.wikipedia.org/wiki/MacOS_Sierra) (released Sept. 2019); that is the SDK level it uses.   While this can easily be set as current as you wish in XCode, and most Mac users keep their systems up-to-date, I see no reason to bring it closer to currency: back-compatibility is a virtue. See [Related Resources](#related-resources) below for the latest "released" Mac installable. I may move it here and use the GitHub "release" mechanism.

I'm satisfied with the operability and reliability of the sources posted here.  I am actively thinking about changes necessary to support relay and track-section names more complicated than (the current) digits followed by alphanumerics, for example `A1-708, A1-708H` instead of `1708/1708H,` which would facilitate the representation of interlockings where two or more subway lines, with distinct stationing letters and origins, join, such as the incomparable E. 180th St. rebuild of 2013. If I successfully implement this or other new features, they will not break extant interlockings, and I won't "push" until I have solid code.

I may or may not fix reported bugs and post changes, but I want to know if you can't build it; contact me via GitHub. I expect to post fixes to bugs I encounter and gratuitous enhancements from hereon in.

## Build targets

There are five in this project:

- **NXSYSMac**. This is the Mac GUI application, complete with resources when built, invocable as a standard `.app` Mac GUI application.   It opens interlocking scenarios, displays control panels, accepts their operation, and runs them.

- **TLEdit**, the track-layout editor.  A separate Mac GUI application (a Windows version exists, too, but not yet buildable here) that allows the creation and maintenance of `.trk` files containing layout information (and only layout information).  This is how you create the track-maps/panel layouts of scenarios, not any part of their logic. It is (poorly) self-documenting.

- **Relay Compiler**. This is presently a joke, because even though this command-line application runs perfectly on the 64-bit Mac, *it only produces Windows object code*, 32- as well as 16-bit, actually.  It is simply no longer necessary with the speed of modern machines; interlockings run rapidly enough "interpreted" (Lisp taxonomy).  Prior to Mac NXSYS, though, it was quite neat. It is a tribute to Apple that I have never had to learn the Mac machine-language environment.

- **BLISP** (B for Bernie).  A command-line program, being a test-build of its native reader (`readsexp.cpp`) for the quasi-Lisp used for interlocking definitions.  `READ-PRINT` only, though, C(++)-no-`EVAL`. 10 cdr trains are handled :).

- **Relay Indexer**.  This recent innovation is another command-line C++11 program (no Objective C/C++) built from this tree that produces a text-format "relay index", a cross-reference of which relays are referenced as logic inputs by other relays, including "built-in" relays to the system. While this can be useful, it even more usefully produces an Emacs/Aquamacs `TAGS` table for all of the relays in the interlocking, allowing `meta-.` to be used with relays, and allowing relay source to be located from NXSYS' Relay Draftsperson.

The distribution of the sources into XCode "groups" is somewhat chaotic, and I apologize.  The subdirectory `NXSYS` is supposed to contain all sources (and headers) that are shared with the Windows build, although it includes some inexcusable others. But other directories contain Mac-only code and headers.

The TLEdit `buttons` directory contains, in addition to png's for its tool-panel buttons, Pixelmator (pre-Pro) files from which they were created.


## *Et in fine* ...

Relay-based wayside color-light block signalling was in use in the New York Subways before *Titanic* sailed, before anyone knew that silicon could be used for building anything except deserts and glass, and when "computer" meant an accountant. In the present century, newer, computer- and communications-based technologies have finally begun to make inroads in New York, and this stuff is ... *como se dice*, "a bit long in the tooth", like this author, who grew up with and learned to understand and admire it.  Nevertheless, the principles of safety design herein embodied survive even in the latest programmed-logic controller (PLC) plants, in New York and globally.  NXSYS has already brought pleasure and learning to many, and even inspired imitators.

Go to town, or Coney Island Yard, with it, but keep my name and credit as you add your own.  It is my invention, my "baby", my work, my gift, and, with my music, my legacy.

Thank you for interest ... Enjoy,

### Bernard Greenberg
### 4 February 2022

===

## Related Resources

- [https://BernardGreenberg.com/Subway](https://BernardGreenberg.com/Subway) - my [personal site](https://BernardGreenberg.com) New York subway page, including downloads of the latest installables for Windows and macOS, as well as ...
- [A lengthy memoir](https://bernardgreenberg.com/Subway/bsg-subway.html) detailing my involvement with the New York City subway over many decades, including the story of NXSYS.
- [nycsubway.org](https://www.nycsubway.org/wiki/Main_Page), a massive site by David Pirmann on all aspects of the system, including thousands of current and historical photos and historical books, and, most relevantly, ...
- [A full primer/tutorial](https://www.nycsubway.org/wiki/Subway_Signals:_A_Complete_Guide) on "classic" New York City wayside color-light block signalling, written by me in the 1990s.  NXSYS was first posted there.
- Two recent video tutorials on NXSYS (YouTube): [Basic demo/introduction](https://www.youtube.com/watch?v=nAgy_TZ5Dcs) and [Second lesson](https://youtu.be/Bppq4wbgBxs).  If you visit, check out my musical work there, too.
- New York Metropolitan Transit Authority (MTA) [page on CBTC](https://new.mta.info/projects/cbtc) (Communications-Based Train Control), the modern technology replacing this entire superannuated colossus.


