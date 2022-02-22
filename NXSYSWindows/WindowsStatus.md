# NXSYS MS Windows Status
## 22 February 2022

This repository now has enough content to build Windows 10 executables (64-bit, 32 not guaranteed) of the three executables of the system, in Visual Studio (VS) 2022, C++ Language Level C++17, platform toolset VS2022 level 143, Windows SDK “latest installed version“ 10.0, my Windows 10 at current updates 12 February 2022.

A usable up-to-date product can be built (if not yet downloaded as executables).  It builds and works as advertised, but I am still cleaning up loose ends, so it is not stable. I will release it when it is.

**You can still download a fully operative 2016 Windows NXSYS from [the NXSYS page on my site](https://BernardGreenberg.com/NXSYS).** If you are stuck in an ancient, de-supported version of Windows not 64 bits, use the 2016 posting, which is for 32-bit Windows.

Everything I know about works, except what’s listed below that may not ever work. `Debug` and `Release` builds of 4 projects (`NXSYS`, `TLEdit`, `Relay Index`, and an MSI installer project), platform `x64`. Right now you have to move the Documentation and Interlockings trees yourself if you want to use application access to them.

### Now, what doesn’t work

- The OLE Automation server and control aren’t there.  I haven’t used NXSYS OLE in the 21st century, and I don’t suppose anyone has, so it likely won't be revived.  *¡Olé!, como se dice en España*.
- Printing (never enabled on the Mac) claims to work, queues a file, but it doesn't print.  It was never very useful.  Large temporary printouts waste expensive toner. I may excise it.
- Cab view has never been in Version 2, and won’t start now.  Video game design is “above my pay grade”.

##### System-wise

I have not yet determined which, if any, DLL's need to be redistributed, but if you can build it, you can run it. The Microsoft MSI installer (a pleonasm) generator packs DLL's; I can't know if it's enough or too many until someone without Visual Studio tests the installation.
 
 I know it doesn’t build without `version.dll`, and `comctl32.dll`, but the Installer  generator told me to remove them from the Installation, where it itself had put it.
  
I have not checked executables into the Repository. I may.

### Layout of Source

In this repository, the top-level directory `NXSYS` contains everything that is shared between the Mac and Windows builds of the app (and nothing else).  The second-level directory `TLEdit/tled`, quite unsymmetrically, contains everything shared by both builds of TLEdit (perhaps I will reorganize this) "and hopefully little else".  TLEdit depends upon the `NXSYS` directory, too, for sources and headers.

The top-level directory `NXSYSWindows` contains everything specific to Windows, and *nothing* used on the Mac.  All Windows-specific code and resources (about 25 files) for both apps are in it, directly.  The solution file `NXSYSWindows.sln` is there, as well as subdirectories for the four (current) VS projects contained therein, which *do not contain code*, but only VS artifacts such as the `.vcxproj` project file and its artifacts.


### Build it yourself

You will need the [MS Visual Studio Installer Project Extension](https://marketplace.visualstudio.com/items?itemName=VisualStudioClient.MicrosoftVisualStudio2022InstallerProjects), which is freely available, if you wish to build the MSI/ installer.

All of the pathnames in the project files are solution-relative; there are no references outside the repository tree, in fact, no references other than to the solution directory (`NXSYSWindows`) or its peers `NXSYS` and `TLEdit` (although there are build-time references to the `Documentation` directory, as `$(SolutionDir)..\Documentation`).

All you have to do is download this repository, assure you have a VS 2022 at least as up-to-date as mine (described above, free Community Edition used) open the solution `NXSYSWindows.sln` its eponymous directory, select and build both projects (`NXSYS` and `TLEdit`) and go to town. 

Note that my standard for code correctness is the Mac compiler, not the windows one.  The latter produces many warnings that the Mac does not, and should be ignored. Maybe I will clean them up some day.  Some are from disappointing industry failures, such as the `size_t` returned by both `strlen` and STL `size()` aggrieving ancient Microsoft API’s defined to want `int`s.

