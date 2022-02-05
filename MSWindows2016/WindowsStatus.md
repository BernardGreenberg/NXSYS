# NXSYS for Mac ... for MS Windows!
### Status report, 5 February 2022
About 30 years after NXSYS was born on 16-bit Windows 3.1.

Included here, in folder `MSWindows2016` in this XCode tree, is just that, the exact tree which built Windows NXSYS version 2 in 2016, whose output, with two MS redistributable dll's, is downloadable at [https://BernardGreenberg.com/Subway](https://BernardGreenberg.com/Subway).  This was done in Visual Studio 2015, and all the Visual Studio 2015 control files are here, although the pathnames in them, of course, need  to change if this is ever resurrected.  But all the non-system files are present in this subtree, including the NXOLE system having no analogue in the Mac build.

I have deleted all the "Release" and "Debug" object directories to save uploads and space.

Some words about the content of the three directories that sibl this .md file are in order.

- **Visual Studio 2015** includes a whole VS "Solution", that is, aggregation of related projects part of a bigger idea, including a file `WinNX2016.sln` defining it and its projects. `WinNX2016` and `TLEdit` correspond to `NXSYSMac` and `TLEdit` in this repo . `NXCTL` is a documented automation system not present in the Mac system, and I don't remember how `TextBuild` (in Python 2) is used, although it seems to address some line-end issue in resources.  The `vcxproj` files contain the preprocessor definitions to turn the requisite files from Mac to Windows sources.

- **bsglib** consists of three archaic personal C++ and C programs needed by the Windows build. A much better argument parser (`argparse`, modelled after its eponymous Python package) in modern C++ can be found in the `RelayIndex` folder of the main project. There is a better `fnmerge` in STL in the Mac project, although the `std::fs` package in C++17 can replace the whole thing trivially, and `SyserrEx` is a variadic error message facility of which there are already several better ones in `readsexp.cpp`.

- **NXMac** is the interesting one.  It contains all the code other than that in `bsglib`—source, headers, and resource files, in itself and its subdirectories `v2`, `ole`, and `tled`. The vast majority of this content is is "portable", i.e., shared with the Mac version, with as much, or as little, preprocessor conditionalization as required.  There are some Windows-only modules.  ***What is super-important is that for all the allegedly portable modules, there exist more modern, capable, and correct versions in the `NXSYS` folder of the Mac project.*** (That is the origin of the `NXSYS` folder.) And those files are supposed to be ***still portable***, in C++11, although they have never used in a build on Windows.  Some of those files have been split into two or more files, but the components are guaranteed (or at least were so designed) to be portable across both systems.

You do not need a Mac to clone this repository and engage with this code.

To build a new Windows NXSYS, the "solution" and its projects have to be opened and upgraded by a current Visual Studio environment, the embedded pathnames changed, and such problems as crop up in integrating newer code solved.  Anyone attempting this project for amusement should try building the entire 2016 image with a newer Visual Studio before attempting to integrate the newer code.

The reasons I'm not eager to do this myself any time soon are:
- I love the Mac and dislike MS-Windows. I know that the MS System is much more popular, and ubiqitous. I don't particularly like debugging on Windows, but these reasons are irrational.

- I don't have a modern machine running modern Windows, and it's not worth my while to buy one just to upgrade NXSYS.  That opinion may change,

- Even were I to be gifted or "inherit" such a machine, Visual Studio isn't the freebee it used to be, and I don't know if freeware Visual Studios have enough C++/STL support to handle NXSYS’s needs, or if other "little (essential) things" have been omitted. I do not want to pay Microsoft tribute for the privilege of compiling C++ programs for their platform.

- I'm not eager to re-migrate to MinGW or other non-Microsoft Win32 solutions.

The eccentric stragegy I chose for creating the Mac Build leaves NXSYS at its core a Windows program with Mac-simulated user-interface elements, so the Windows version is ***considerably simpler than the Mac version*** for the exact same functionality, as the operating system supports its windows (hence, ...) and dialogs natively.  Debugging the Windows build is considerably simpler than debugging in the hirsute Objective C++ ARC (Automatic Reference Counting) regime.  So let that be an inspiration if anyone wants to try.

Note also that `NXSYSV1.html` is now called `NXSYS.html`, in the Documentation folder (it's named in the Windows code/project files.)
