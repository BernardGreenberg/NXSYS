#  NXSYS sources, complete
###  By and ©Copyright 1994-2022, Bernard S. Greenberg
#### First Posted 4 February 2022 (last 27 Feb 2022)

This is the buildable, runnable source code for **NXSYS** (*enn-ex-sys*) (Version 2), my New York City Subway relay-logic and interlocking panel simulator, and its six offered interlockings, as it stands.  It is mine, and mine alone, although Dave Barraza's contributions to its debugging and development deserve credit.  Feel free to continue it, improve it if you like, or just build it for yourself.  I want this repository to be its home.  I'm not soliciting or merging branches at this time.

This application is posted and offered under the terms of the GNU General Public License Version 3. See the file `LICENSE` for details. These sources are offered "AS-IS", with no warranty or guarantee of operability or suitability for any purpose made or implied, and use is at your own risk — all the code is available here for inspection.  **Do not** attempt to use this code to control actual railroads with live crews and passengers (I would be *extremely* impressed and flattered, but not responsible for the outcome).

The file [NXSYS.html.pdf](https://github.com/BernardGreenberg/NXSYS/blob/master/Documentation/NXSYS.html.pdf) in the [Documentation folder](https://github.com/BernardGreenberg/NXSYS/tree/master/Documentation) is a comprehensive description of the application and its capabilities. See [Related Resources](#related-resources) below for a video demo.

This codebase can build working Mac and Windows (the latter 64 and 32-bit) executables, without error or warning.  Although NXSYS was born on (16-bit!) Windows, the reference implementation is presently that on macOS, hosted and built in XCode.  See [the macOS Readme](https://github.com/BernardGreenberg/NXSYS/blob/master/DocSource/MacStatus.md) for particulars and status of the Mac version and [the Windows Readme](https://github.com/BernardGreenberg/NXSYS/blob/master/NXSYSWindows/WindowsStatus.md) for particulars and status of the Windows version.


Please ignore admonitions in the interlocking and other help texts about not redistributing without permission (it's public now), and old email addresses there as well; contact me via GitHub if need be.  Unless you do real development, there shouldn't be any need to redistribute as long as this repository exists.

I am not, and have not been, associated with any transportation provider nor signal engineering contractor, nor has any endorsed this work, although during its history quite a few people such people have contributed knowledge, critique, and fanship.

## Code base

This application is currently in C++20 (the 2020 standard of C++), exploiting STL.  It is almost all C++11, and, especially in the Windows version, contains code in dialects going all the way back almost to C. The shared core (whose maximization was my goal)  and the Windows-specific code is only in C++.  Some C++17 and 20 features have recently been exploited (e.g., `std::filesystem`).  Note that the Microsoft compiler will lie about the `__cplusplus` version unless told not to with `/Zc:__cplusplus`. The [Mac-only code](https://github.com/BernardGreenberg/NXSYS/blob/master/DocSource/MacStatus.md) exploits Cocoa/Objective C++.

The core of the functionality contains preprocessor conditionalizations for the two platforms—there is a [discussion of the details here](https://github.com/BernardGreenberg/NXSYS/blob/master/NXSYSWindows/CompilerFlags.md). NXSYS was ported to the Mac in late 2014, with very substantial Mac-specific code additions, and back to Windows in 2016, with the last six years of Mac improvements were re-shared and retrofitted in February, 2022.

There are three shared builds (“projects” on Windows, “targets” on the Mac), `NXSYS` the interlocking/relay simulator, `TLEdit` the track layout editor, and [`Relay Index`, the cross-reference and tags file generator](https://github.com/BernardGreenberg/NXSYS/blob/master/Relay%20Index/RelayIndex.md), a console program.  The Mac Xcode project (the overall container, analogous to the Windows “solution") contains two additional targets for debugging/amusement.

I do take pains in distributed interlockings to avoid incompatible use of new features; the scenario language makes it possible to exploit newer features only in newer builds, downward compatibly.  All five of these interlockings are thus fully functional in the previously-distributed 2016 32-bit Windows build as well as the new builds (it is no longer available).  The interlockings here in the [eponymous folder](https://github.com/BernardGreenberg/NXSYS/tree/master/Interlockings) are newer and better (e.g., bugs fixed) than those in the earlier distributions.

None of the OpenGL code of the popular erstwhile Cab View feature of Version 1 is present in this repository.  It cannot handle, nor can it be obviously extended to handle, the arbitrary track geometries of Version 2. And in a world where spectacular interactive animated graphics such as [this (threejs)](https://threejs.org/examples/#webgl_animation_keyframes) are available, Cab View's black, grey-walled tunnels with only rails and signals visible would be ... inadequate. Becoming the video-game designer required is not a credible use of my time, nor would the result be a reasonable extension of the purview of this application.

There are various degrees of ... ummm ... "beauty and professionalism evident in the code-base".  The oldest code dates from 25 years ago, when C++, and Windows were a lot more cumbersome.   The newest code, including the Mac-specific code, is considerably more pleasing.  I occasionally do mount cleanup campaigns upon it, but there are ugly and uneven stations along the line.

I'm satisfied with the operability and reliability of the sources posted here.  I am actively thinking about possible significant enhancements, briefly, more expressive relay/track circuit designations (allowing large, multi-right-of-way interlockings) and support for “topomorphic circuits” (see the [logic document](#logic)).  If I successfully implement these or other new features, they will not break extant interlockings—I won't “push” until I have solid code.

As of 21 February 2022, the application (both versions) includes, in source, the [**pugixml** portable XML library](http://pugixml.org) ©2006-2018 by Arseny Kapoulkine, which is MIT-license free to use.

<a id="linux"></a>
## When/why not Linux?

I do get regular requests for Linux support.  Right now, this is too difficult to be a short-term goal.  To support the Mac in 2014, rather than rewrite every bit of graphics or UI code in the system, I implemented the Windows GUI in Mac code, allowing me to *keep all the C++ code with little modification*, which I held a worthy goal. (That is why there are modules and functions with “windows” names in Mac-specific portion of the code). This path was also chosen in favor of using a multiplatform-portable graphics system (such as [Qt](https://www.qt.io)) and redoing every graphics call or model-assuming code-fragment in the system to call it.

Support on Linux, or some fourth system, without taking the onerous “write it all to use a portable GUI substrate” would mean total duplication of the work done for the Mac, which was huge.  Adaptation to a portable GUI system would be *even more huge*.  So, as with some other packages (e.g., [Hauptwerk](https://hauptwerk.com)), Windows and Mac for the foreseeable future.


<a id="logic"></a>
## New — Logic documentation


This Repository now contains an [extensive tutorial on Interlocking Logic (pdf)](https://github.com/BernardGreenberg/NXSYS/blob/master/Documentation/Interlocking%20Logic%20Design.pdf) and its [.odt source](https://github.com/BernardGreenberg/NXSYS/blob/master/Documentation/Interlocking%20Logic%20Design.odt) (Libre Office), as well as a new [document on the Relay Language](https://github.com/BernardGreenberg/NXSYS/blob/master/DocSource/RelayLanguage.md).  Both of these are intended for those interested in creating their own interlockings, and should be of great interest to any electrically-competent railfan.


## And finally ...

Relay-based wayside color-light block signalling was in use in the New York Subways before *Titanic* sailed, before anyone knew that silicon could be used for building anything except deserts and glass, and when “computer” meant an accountant. In the present century, newer, computer- and communications-based technologies have finally begun to make inroads in New York, and this stuff is ... *como se dice*, “a bit long in the tooth”, like its author, who grew up with it and learned to understand and admire it.  Nevertheless, the principles of safety design herein embodied survive even in the latest programmed-logic controller (PLC) plants, in New York and globally.  And NXSYS has already brought pleasure and learning to many, and even inspired imitators.

Go to town, or Coney Island Yard, with it, but keep my name and credit as you add your own.  It is my invention, my “baby”, my work, my gift, and, with my music, my legacy.

Thank you for interest ... Enjoy,

### Bernard Greenberg
### February 2022

===

## Related Resources

- [https://BernardGreenberg.com/Subway](https://BernardGreenberg.com/Subway) - my [personal site](https://BernardGreenberg.com) New York subway page, including downloads of the latest installables for Windows and macOS, as well as ...
- [A lengthy memoir](https://bernardgreenberg.com/Subway/bsg-subway.html) detailing my involvement with the New York City subway over many decades, including the story of NXSYS.
- [nycsubway.org](https://www.nycsubway.org/wiki/Main_Page), a massive site by David Pirmann on all aspects of the system, including thousands of current and historical photos and historical books, and, most relevantly, ...
- [A full primer/tutorial](https://www.nycsubway.org/wiki/Subway_Signals:_A_Complete_Guide) on "classic" New York City wayside color-light block signalling, written by me in the 1990s.  NXSYS was first posted there.
- Two recent video tutorials on NXSYS (YouTube): [Basic demo/introduction](https://www.youtube.com/watch?v=nAgy_TZ5Dcs) and [Second lesson](https://youtu.be/Bppq4wbgBxs).  If you visit, check out my musical work there, too.
- New York Metropolitan Transit Authority (MTA) [page on CBTC](https://new.mta.info/projects/cbtc) (Communications-Based Train Control), the modern technology replacing this entire superannuated colossus.


