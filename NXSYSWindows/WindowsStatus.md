# NXSYS MS Windows Build(s)
## 26 February 2022

This repository can build the three fully functional executables of the system, 64 and 32-bit, in Visual Studio (VS) 2022, C++ Language Level C++20, platform toolset VS2022 level 143, Windows SDK “latest installed version“ 10.0, my Windows 10 at current updates 12 February 2022.  There are no built executables in this repository; use [my web site page](https://BernardGreenberg.com/Subway) for that.

The 64-bit build is the “official” one (and the Mac build more “official” than that); if you are on any version of Windows 10 (or later), use it.  The 32-bit applications can be built and run without error, but there is (presently) no installer for them (TBD). You can still download a fully operative 2016 Windows NXSYS (32-bit) from [the NXSYS page on my site](https://BernardGreenberg.com/Subway) (but it’s  a zip file, not an installer).

To run NXSYS or TLEdit, nothing is needed but the two executables (although see [this note below](#dlls)). But the delivered products incorporate Interlocking Library and Documentation trees in their product directory, along their xml manifests, which the new `Interlocking Library` command and new help menu require to function.  The Debug builds do this with `robocopy`; the Release builds do not.  The Installer projects, which expect the Release builds, freight all this on board, and install them alongside the executables.

### Features removed

- The OLE Automation server and control aren’t there.  I haven’t used NXSYS OLE in the 21st century, and I don’t suppose anyone has, so it likely won't be revived.  *¡Olé!, como se dice en España*.
- Printing (never enabled on the Mac) claims to work, queues a file, but it doesn't print.  It was never very useful.  Large temporary printouts waste expensive toner. I may excise it.
- Cab view has never been in Version 2, and won’t start now.  Video game design is “above my pay grade”.

### Layout of Source

In this repository, the top-level directory `NXSYS` contains everything that is shared between the Mac and Windows builds of the app (and nothing else).  The second-level directory `TLEdit/tled`, quite unsymmetrically, contains everything shared by both builds of TLEdit (perhaps I will reorganize this) "and hopefully little else".  TLEdit depends upon the `NXSYS` directory, too, for sources and headers.

The top-level directory `NXSYSWindows` contains everything specific to Windows, and *nothing* used on the Mac.  All Windows-specific code and resources (about 25 files) for both apps are in it, directly.  The solution file `NXSYSWindows.sln` is there, as well as subdirectories for the four (current) VS projects contained therein, which *do not contain code*, but only VS artifacts such as the `.vcxproj` project file and its artifacts.


### Build the apps yourself


All of the pathnames in the project files are solution-relative; there are no references outside the repository tree. In fact, no references other than to the solution directory (`NXSYSWindows`) or its peers `NXSYS` and `TLEdit`.

All you have to do is download and unzip, or clone, this repository, assure you have a VS 2022 at least as up-to-date as mine (described above, free Community Edition used) open the solution `NXSYSWindows.sln` its eponymous directory, select and build both projects (`NXSYS` and `TLEdit`) and go to town.

Do read [this document here](https://github.com/BernardGreenberg/NXSYS/blob/master/NXSYSWindows/CompilerFlags.md) about compiler (preprocessor) flags.

### Build the installer MSI(s)

Building the installer MSIs is not quite as easy.  There are two projects, `Installer`, the 64-bit version, and `Setup32`, the 32-bit version (both installers are 32-bit programs, but they install different versions of the application.  While one installer might have been "cleaner", it would either preclude installing the 32-bit version on a 64-bit system, or require custom UI in the Installer).

You will need the [MS Visual Studio Installer Project Extension](https://marketplace.visualstudio.com/items?itemName=VisualStudioClient.MicrosoftVisualStudio2022I nstallerProjects), which is freely available. Don’t be surprised if you have to fiddle with the installer projects to make them work on your system, requiring (more sadly) knowledge of the MSI world.  The 64-bit output goes to `NXSYSWindows\x64\Release\NXSYS.msi`, and the 32-bit output to `NXSYSWindows\Release\NXSYS32.msi`.

I have experienced difficulty making it accept shortcuts referencing “Primary Project Output”s as it seems to want, and less trouble simply freighting the output executables as ordinary .exe's, and pointing the shortcuts at them.

#### Oh yes, those DLLs
<a id="dlls"></a>

All Windows programs built with Visual Studio require a passel of DLLs (Dynamic Link Libraries).  If you are the developer who has Visual Studio, you have those DLL's “and there is no problem” — *for you*. But if you build an installer and a user who does not have Visual Studio (as most don’t) tries it, your app will fail to run, noting missing DLLs.  You have two choices: you can pack all the “redistributable” DLLs in your installer, or somehow convey an out-of-band message to your users to download DLLs from Microsoft themselves, which is not a popular activity. You need a second machine or willing friend to quality-assure this.

Microsoft currently advocates the second method, because it allows them to fix bugs in the DLLs; cached DLLs do not get fixed.  The NXSYS Installer thumbs its nose at Microsoft and packs a dozen DLLs (less than 1 megabyte) on-board, that go into the NXSYS Installation directory. The Installer *builder* expects them to be in
~~~
C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.31.31103\x64\Microsoft.VC143.CRT\*.dll
~~~
which is where they will be (for 64 bits) and
~~~
C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.31.31103\x86\Microsoft.VC143.CRT\*.dll
~~~
for 32, if you install the [Microsoft Visual C++ Redistributables download](https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170) from that link. Isn't that what *redistributable* means?

If you do modify and redistribute this product, please substitute your own name as the company name in the installer and elsewhere.  GPL3 license, though—my original authorship cannot be effaced.

