# NXSYS MS Windows Build(s)
## 26 February 2022

This repository now has enough content to build Windows 10 executables (64-bit, 32 not guaranteed) of the three executables of the system, in Visual Studio (VS) 2022, C++ Language Level C++17, platform toolset VS2022 level 143, Windows SDK “latest installed version“ 10.0, my Windows 10 at current updates 12 February 2022.

A usable up-to-date product can be built (if not yet downloaded as executables).  It builds and works as advertised, but I am still cleaning up loose ends, so it is not stable. I will release it when it is.

**You can still download a fully operative 2016 Windows NXSYS from [the NXSYS page on my site](https://BernardGreenberg.com/NXSYS).** If you are stuck in an ancient, de-supported version of Windows not 64 bits, use the 2016 posting, which is for 32-bit Windows.

Everything I know about works. `Debug` and `Release` builds of 4 projects (`NXSYS`, `TLEdit`, `Relay Index`, and an MSI installer project), platform `x64`. Right now you have to move the Documentation and Interlockings trees yourself if you want your test builds to access them; the Installer build packages them properly (and Debug builds move them with `robocopy`, which is a horror).

### Features removed

- The OLE Automation server and control aren’t there.  I haven’t used NXSYS OLE in the 21st century, and I don’t suppose anyone has, so it likely won't be revived.  *¡Olé!, como se dice en España*.
- Printing (never enabled on the Mac) claims to work, queues a file, but it doesn't print.  It was never very useful.  Large temporary printouts waste expensive toner. I may excise it.
- Cab view has never been in Version 2, and won’t start now.  Video game design is “above my pay grade”.

### Layout of Source

In this repository, the top-level directory `NXSYS` contains everything that is shared between the Mac and Windows builds of the app (and nothing else).  The second-level directory `TLEdit/tled`, quite unsymmetrically, contains everything shared by both builds of TLEdit (perhaps I will reorganize this) "and hopefully little else".  TLEdit depends upon the `NXSYS` directory, too, for sources and headers.

The top-level directory `NXSYSWindows` contains everything specific to Windows, and *nothing* used on the Mac.  All Windows-specific code and resources (about 25 files) for both apps are in it, directly.  The solution file `NXSYSWindows.sln` is there, as well as subdirectories for the four (current) VS projects contained therein, which *do not contain code*, but only VS artifacts such as the `.vcxproj` project file and its artifacts.


### Build it yourself

You will need the [MS Visual Studio Installer Project Extension](https://marketplace.visualstudio.com/items?itemName=VisualStudioClient.MicrosoftVisualStudio2022InstallerProjects), which is freely available, if you wish to build the MSI/ installer.

All of the pathnames in the project files are solution-relative; there are no references outside the repository tree, in fact, no references other than to the solution directory (`NXSYSWindows`) or its peers `NXSYS` and `TLEdit` (although there are build-time references to the `Documentation` directory, as `$(SolutionDir)..\Documentation`).

All you have to do is download this repository, assure you have a VS 2022 at least as up-to-date as mine (described above, free Community Edition used) open the solution `NXSYSWindows.sln` its eponymous directory, select and build both projects (`NXSYS` and `TLEdit`) and go to town. 

Do read [this document here](https://github.com/BernardGreenberg/NXSYS/blob/master/NXSYSWindows/WindowsStatus.md) about compiler (preprocessor) flags.

### Oh yes, those DLLs

All Windows programs built with Visual Studio require a passel of DLLs (Dynamic Link Libraries).  If you are the developer who has Visual Studio, you have those DLL's “and there is no problem” — *for you*. But if you build an installer and a user who does not have Visual Studio (as most don’t) tries it, your app will fail to run, noting missing DLLs.  You have two choices: you can pack all the “redistributable” DLLs in your installer, or somehow convey an out-of-band message to your users to download DLLs from Microsoft themselves, which is not a popular activity. You need a second machine or willing friend to quality-assure this.

Microsoft currently prefers the second method, because it allows them to fix bugs in the DLLs; cached DLLs do not get fixed.  The NXSYS Installer thumbs its nose at Microsoft and packs a dozen DLLs (less than 1 megabyte) on-board, that go into the NXSYS Installation directory. The Installer *builder* expects them to be in
~~~
C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.31.31103\x64\Microsoft.VC143.CRT\*.dll
~~~
which is where they will be if you install the [Microsoft Visual C++ Redistributables download](https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170) from that link. Isn't that what *redistributable* means?

If you do modify and redistribute this product, please substitute your own name as the company name in the installer and elsewhere.  GPL3 license, though—my original authorship cannot be effaced.

