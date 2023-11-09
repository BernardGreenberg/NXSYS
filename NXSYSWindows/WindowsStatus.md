# NXSYS MS Windows Build(s)
## 9 November 2023

The 32-bit build has been abandoned.  The very few conditionalizations and installer script have not been removed, but the distributed 32-bit will remain at Version 2.6.

“You've had them all along; just click your magic slippers.” --- the DLL problem discussed below had an easy solution.  All that had to be changed is the installation instructions.  the **setup.exe** built by the installer script validates and loads (from the web) the correct set of DLL's prior to running the MSI.  The on-board DLL's have been removed from the Windows installer package, and a 64-bit installation (**setup.exe**, an MSI, and a brief text file explaining what to do) of 64-bit Windows 2.7.9, built by Ellis Shore (this time) have been posted to the distribution site.

## 27 February 2022

This repository can build the three fully functional executables of the system, 64 and 32-bit, in Visual Studio (VS) 2022, C++ Language Level C++20, platform toolset VS2022 level 143, Windows SDK “latest installed version“ 10.0, my Windows 10 at current updates 12 February 2022.  There are no built executables in this repository; use [my web site page](https://BernardGreenberg.com/Subway) for that.

The 64-bit build is the “official” one (and the Mac build more “official” than that); if you are on any version of Windows 10 (or later), use it. The Visual Studio solution can build 64- and 32-bit applications and installers (separate installers for 64 and 32, on purpose).

To run NXSYS or TLEdit, nothing is needed but the two executables (although see [this note below](#dlls)). But the delivered products incorporate Interlocking Library and Documentation trees in their product directory, along their xml manifests, which the new `Interlocking Library` command and new help menu require to function.  The Debug builds do this with `robocopy`; the Release builds do not.  The Installer projects, which expect the Release builds, freight all this on board, and install them alongside the executables.

### Former Windows features removed

- The OLE Automation server and control are gone.  I haven’t used NXSYS OLE in the 21st century, and I don’t suppose anyone has, so it likely won't be revived.  *¡Olé!, como se dice en España*.
- Printing (never enabled on the Mac) claims to work, queues a file, but it doesn't print.  It was never very useful.  Large temporary printouts waste expensive toner. I may excise it.
- Cab view has never been in Version 2, and won’t start now.  Video game design is “above my pay grade”.

### Layout of Source

In this repository, the top-level directory `NXSYS` contains everything that is shared between the Mac and Windows builds of the app (and nothing else).  The second-level directory `TLEdit/tled`, quite unsymmetrically, contains everything shared by both builds of TLEdit (perhaps I will reorganize this) "and hopefully little else".  TLEdit depends upon the `NXSYS` directory, too, for sources and headers.

The top-level directory `NXSYSWindows` contains everything specific to Windows, and *nothing* used on the Mac.  All Windows-specific code and resources (about 25 files) for both apps are in it, directly.  The solution file `NXSYSWindows.sln` is there, as well as subdirectories for the four (current) VS projects contained therein, which *do not contain code*, but only VS artifacts such as the `.vcxproj` project file and its artifacts.


### Build the apps yourself


All of the pathnames in the project files are solution-relative; there are no references outside the repository tree. In fact, no references other than to the solution directory (`NXSYSWindows`) or its peers `NXSYS`, `TLEdit` and `Relay Index`.

All you have to do is download and unzip, or clone, this repository, assure you have a VS 2022 at least as up-to-date as mine (described above, free Community Edition used) open the solution `NXSYSWindows.sln` its eponymous directory, select and build the three projects (`NXSYS`, `TLEdit` and `Relay Index`) and go to town.

Do read [this document here](https://github.com/BernardGreenberg/NXSYS/blob/master/NXSYSWindows/CompilerFlags.md) about compiler (preprocessor) flags.

### Build the installer MSI(s)

There are two installer projects, `Installer`, the 64-bit version, and `Setup32`, the 32-bit version (both installers are 32-bit programs, but they install different versions of the application.  While one installer might have been "cleaner", it would either preclude installing the 32-bit version on a 64-bit system, or require custom UI in the Installer).  **The 32-bit version is officially obsolete as of 9 November 2023, but can be built.**

You will need the [MS Visual Studio Installer Project Extension](https://marketplace.visualstudio.com/items?itemName=VisualStudioClient.MicrosoftVisualStudio2022InstallerProjects), which is freely available. Don’t be surprised if you have to fiddle with the installer projects to make them work on your system (especially if you try to modify them), requiring (sadly) knowledge of the MSI world.  The 64-bit output goes to `NXSYSWindows\x64\Release\NXSYS.msi`, and the 32-bit output to `NXSYSWindows\Release\NXSYS32.msi`.

I have experienced difficulty making it accept shortcuts referencing “Primary Project Output”s as it seems to want. As a result, I package the output executables as ordinary .exe's, and point the desktop and Start menu shortcuts to them.

#### Oh yes, those DLLs
<a id="dlls"></a>
(Decimated and upgraded 9 November 2023)

Projects built with Visual Studio require the runtime subroutines required by its languages, which exist in executable "Dynamic Link Libraries" (DLL's') called the "Visual C++ 14 Runtime Libraries", that must be present on your machine to execute those built projects.  The developer (e.g., me) has these DLL's by virtue of having Visual Studio, but that does not supply them to the end-user; but solving that problem is now easy.

This Microsoft page explains it.  The NXSYS installer project (**Installer**) now does this (thanks to Ellis Shore, who figured out the right strategy).  The installing user executing **setup.exe** causes that program to install these prerequisites on the user's machine.

https://learn.microsoft.com/en-us/visualstudio/ide/reference/prerequisites-dialog-box?view=vs-2022

The 32-bit legacy build does not do this, but carries its own DLL's, which it does not install, but keeps in its own directory, a poor strategy if every app did that.

If you do modify and redistribute this product, please substitute your own name as the company name in the installer and elsewhere.  GPL3 license, though—my original authorship cannot be effaced.

