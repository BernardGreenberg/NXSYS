# NXSYS for Mac ... for MS Windows!

## Status 10 Feb 2022:
###I have acquired sufficient resources to upgrade and debug the Windows build, and am doing so. It is well under way, and I expect to to have a VS2022 version in C++14 operative in some days.   The OLE feature has been dropped.  Don't bother doing it yourself.

## Status report, 5 February 2022
About 30 years after NXSYS was born on 16-bit Windows 3.1.

Included here, in folder `MSWindows2016` in this XCode tree, is just that, i.e., the exact tree which built Windows NXSYS version 2 in 2016, whose output, with two MS redistributable dll's, is downloadable at [https://BernardGreenberg.com/Subway](https://BernardGreenberg.com/Subway).  This was done in Visual Studio 2015, and all the Visual Studio 2015 control files are here, although the pathnames in them, of course, need  to change if this is ever resurrected.  But all the non-system files are present in this subtree, including the NXOLE system having no analogue in the Mac build.

I have deleted all the "Release" and "Debug" object directories to save uploads and space.

Some words about the content of the three directories that sibl this .md file are in order.

- **Visual Studio 2015** includes a whole VS "Solution", that is, aggregation of related projects part of a bigger idea, including a file `WinNX2016.sln` defining it and its projects. `WinNX2016` and `TLEdit` correspond to `NXSYSMac` and `TLEdit` in this repo . `NXCTL` is a documented automation system not present in the Mac system, and I don't remember how `TextBuild` (in Python 2) is used, although it seems to address some line-end issue in resources.  The `vcxproj` files contain the preprocessor definitions to turn the requisite files from Mac to Windows sources.

- **bsglib** consists of three archaic personal C++ and C programs needed by the Windows build. A much better argument parser (`argparse`, modelled after its eponymous Python package) in modern C++ can be found in the `RelayIndex` folder of the main project. There is a better `fnmerge` in STL in the Mac project, although the `std::fs` package in C++17 can replace the whole thing trivially. `SyserrEx` is a trivial shim to a Win32 error-code interface, but necessary.

- **NXMac** is the interesting one.  It contains all the code other than that in `bsglib`—source, headers, and resource files, in itself and its subdirectories `v2`, `ole`, and `tled`. The vast majority of this content is is "portable", i.e., shared with the Mac version, with as much, or as little, preprocessor conditionalization as required.  There are some Windows-only modules.  ***What is super-important is that for all the allegedly portable modules, there exist more modern, capable, and correct versions in the `NXSYS` folder of the Mac project.*** (That is the origin of the `NXSYS` folder.) And those files are supposed to be ***still portable***, in C++11, although they have never used in a build on Windows.  Some of those files have been split into two or more files, but the components are guaranteed (or at least were so designed) to be portable across both systems.
