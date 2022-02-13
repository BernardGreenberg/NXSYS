# NXSYS MS Windows Status
## 12 February 2022

This repository contains (in addition to working Mac builds of NXSYS and TLEdit), enough content to build Debug Windows 10 executables (32-bit Win32) of both, in Visual Studio (VS) 2022, C++ Language Level C++17, platform toolset VS2022 level 143, Windows SDK “latest installed version“ 10.0, my Windows 10 at current updates 12 February 2022.

This re-adaptation work, however, is still in progress: a few things don't work, but it is pretty much done.  A usable up-to-date product can be built (if not yet downloaded executable).

**You can still download a fully operative 2016 Windows NXSYS from [the NXSYS page on my site](https://BernardGreenberg.com/NXSYS).**

### What ***does*** work (today) in the new build:

- NXSYS (the main app) can open, display, and operate the latest interlockings, now including [Duckburg](https://github.com/BernardGreenberg/NXSYS/tree/master/Interlockings/Duckburg). The command menu works.  Signals, switches, and the whole relay logic engine, including the trace window, all seem to work as designed and as they do on the Mac. Even “trains” work, and the Relay Draftsperson and Relay Query dialogs!

- TLEdit, the track layout editor, seems to work, too, with its toolbar (a little different from the Mac’s beautiful one) and rodentation all in order, as well as object detail dialogs.

### Now, what doesn’t work *yet* —

(Of course I’ll update this as more is made to work).

- The Interlocking Status Report dialog/display, which has never before been in the Windows version, but is in intended to be, isn’t there yet.
- The OLE Automation server and control aren’t there.  I haven’t used it in the 21st century, and I don’t suppose anyone has, so it likely won't be revived.  *¡Olé!, como se dice en España*).
- The cool 2.5 Mac feature that allows you to click right in the Relay Draftsperson to send a relay name to another application (e.g., Emacs) isn't there. Maybe it should look for the command line in the Registry (it's in the `info-plist` file on the Mac).
- I have not checked executables into the Repository. I may.

##### System-wise,
- There is no release configuration or build yet.
- I'd like to build this for 64-bit (The Mac version is 64-bit).  The advantage is that the services and DLL's of the Windows 32-bit compatibility subsystem in Windows 10 (which latter is 64-bit) would not be needed.
- I have not yet determined which DLL's need to be redistributed, but if you can build it, you can run it. The 2016 Windows build required redistributing `concrt140.dll`, `vccorlib140.dll`, `msvcp140.dll` and `vcruntime140.dll`. It now seems Microsoft [makes it easy to get “VC DLL’s”](https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170).

The text-based help dialogs are gone.  All help is HTML with illustrations, via your browser (from local files).  New, definitive help files (including a new tutorial on logic design).

### Layout of Source

In this repository, the top-level directory `NXSYS` contains everything that is shared between the Mac and Windows builds of the app (and nothing else).  The second-level directory `TLEdit/tled`, quite unsymmetrically, contains everything shared by both builds of TLEdit (perhaps I will reorganize this) "and hopefully little else".  TLEdit depends upon the `NXSYS` directory, too, for sources and headers.

The top-level directory `NXSYSWindows` contains everything specific to Windows, and *nothing* used on the Mac.  All Windows-specific code and resources (about 20 files) for both apps are in it, directly.  The solution file `NXSYSWindows.sln` is there, as well as subdirectories for the two (current) VS projects contained therein, which *do not contain code*, but only VS artifacts such as project, object, and precompiled header cache files.

### Build it yourself

All of the pathnames in the project files have been relativized.  There should be no references outside the repository tree, in fact, no references other than to the solution directory (`NXSYSWindows`) or its peers `NXSYS` and `TLEdit` (although there might (TBD) be build-time references to the `Documentation` directory when this is complete).

All you have to do is download this repository, assure you have a VS 2022 at least as up-to-date as mine (described above, free Community Edition used) open the solution `NXSYSWindows.sln` its eponymous directory, select and build both projects (`NXSYS` and `TLEdit`) and go to town, via the Local track for now ...



 

