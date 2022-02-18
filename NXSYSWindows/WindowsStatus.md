# NXSYS MS Windows Status
## 18 February 2022

This repository now has enough content to build Windows 10 executables (64- and 32-bit) of the three executables of the system, in Visual Studio (VS) 2022, C++ Language Level C++17, platform toolset VS2022 level 143, Windows SDK “latest installed version“ 10.0, my Windows 10 at current updates 12 February 2022.

The re-adaptation work is fully done.  A usable up-to-date product can be built (if not yet downloaded as executables).  It builds and works as advertised, but I am still cleaning up loose ends, so it is not stable. I will release it when it is.

**You can still download a fully operative 2016 Windows NXSYS from [the NXSYS page on my site](https://BernardGreenberg.com/NXSYS).**  I probably will not distribute new 32-bit versions. If you are stuck in an ancient, de-supported version of Windows not 64 bits, use the 2016 posting.

### What ***does*** work in the new build:

Everything I know about works, except what's listed below that may not ever work. `Debug` and `Release` builds of 3 projects (`NXSYS`, `TLEdit`, `Relay Index`), platforms `x86` and `x64`. Right now you have to move the help files and the image tree into the executable directory yourself.

### Now, what doesn’t work

- The OLE Automation server and control aren’t there.  I haven’t used NXSYS OLE in the 21st century, and I don’t suppose anyone has, so it likely won't be revived.  *¡Olé!, como se dice en España*).
- Printing (never enabled on the Mac) claims to work, queues a file, but it doesn't print.  It was never very useful.  Large temporary printouts waste expensive toner. I may excise it.
- Cab view has never been in Version 2, and won’t start now.  Video game design is “above my pay grade”.

##### System-wise,

- I have not yet determined which, if any, DLL's need to be redistributed, but if you can build it, you can run it. The issue is users not having Visual Studio.


 The 2016 Windows build required redistributing `concrt140.dll`, `vccorlib140.dll`, `msvcp140.dll` and `vcruntime140.dll`. It now seems Microsoft [makes it easy to get “VC DLL’s”](https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170).  
 
 I know it doesn’t build without `version.dll` and `comctl32.dll`. I have to tell it that, but I can’t imagine the absence of either on any system.
- I have not checked executables into the Repository. I may.

### Layout of Source

In this repository, the top-level directory `NXSYS` contains everything that is shared between the Mac and Windows builds of the app (and nothing else).  The second-level directory `TLEdit/tled`, quite unsymmetrically, contains everything shared by both builds of TLEdit (perhaps I will reorganize this) "and hopefully little else".  TLEdit depends upon the `NXSYS` directory, too, for sources and headers.

The top-level directory `NXSYSWindows` contains everything specific to Windows, and *nothing* used on the Mac.  All Windows-specific code and resources (about 25 files) for both apps are in it, directly.  The solution file `NXSYSWindows.sln` is there, as well as subdirectories for the three (current) VS projects contained therein, which *do not contain code*, but only VS artifacts such as the `.vcxproj` project file and its artifacts.

### Build it yourself

All of the pathnames in the project files have been relativized.  There should be no references outside the repository tree, in fact, no references other than to the solution directory (`NXSYSWindows`) or its peers `NXSYS` and `TLEdit` (although there might (TBD) be build-time references to the `Documentation` directory when this is complete).

All you have to do is download this repository, assure you have a VS 2022 at least as up-to-date as mine (described above, free Community Edition used) open the solution `NXSYSWindows.sln` its eponymous directory, select and build both projects (`NXSYS` and `TLEdit`) and go to town. 

Note that my standard for code correctness is the Mac compiler, not the windows one.  The latter produces many warnings that the Mac does not, and should be ignored. Maybe I will clean them up some day.  Some are from disappointing industry failures, such as the `size_t` returned by both `strlen` and STL `size()` aggrieving ancient Microsoft API’s defined to want `int`s.

