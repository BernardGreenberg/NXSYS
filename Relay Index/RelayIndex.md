# Relay Indexer and Source Locator

### 15 February 2022

These two features are new to the Windows build, but have been available in the Mac builds for a while.  They are intended to assist those designing NXSYS interlockings in pseudo-Lisp relay circuitry (which is the only way).

These two functions exploit a command-line program, the *Relay Indexer*, which, unsurprisingly enough, is called `RelayIndex(.exe)`, the `.exe` suffix on Windows, not Mac, and in either case supplied self-standing in both distribution zip files.

`RelayIndex` operates on an entire interlocking, from source.  You direct it to the top-level `.trk` file, the same one you open in the application (NXSYS).  It produces two files, a relay cross-reference, and a `TAGS` file, in the same folder as the source to which it was directed.  They are both text files.  You can place the output somewhere else with the `-o (--output)` control argument.

We will discuss them independently.

## Relay cross-reference

The relay cross reference is a listing, in relay order (smaller object (lever, track circuit, etc.) numbers first, alphabetical within the same object) for each relay, of all the relays (including this one, if it has a “stick contact”) who include a contact of this relay in their circuits.  The ”referrers” are sorted in the same way.

Here is an excerpt from the cross-reference of Myrtle Avenue (make your window wider if the lines wrap).

~~~

155L 244 7104 myrtle.trk
 152CLK    154ZPJ3   155ANN    155ANS    155BNN    155BNS    155CLK    155LS     
 155RN     155RS     160PBS    162PBSS   184KXL    188KXL    194APBS   

155LS 251 7315 myrtle.trk
 154CO     154H      155NWK    155NWZ    155RWK    155RWZ    158CO     158H      
 164CO     164H      170CO     170H      184CO     184STR    200CO     200STR    

155NL UNDEFINED
 155NLP    
~~~

Lines read across, not column-down. The header line for each relay lists the name of the relay, the line number, and the character position in the file where its definition appear.  Of course, those will change if you edit.

Note that 159 NL is listed as “undefined".  That is not an error; 155NL is the tower operator's “normal lever control” for switch 155, and 155NL is thus a so-called *Quisling relay;* you don't supply it—the simulator does.

Note also that the file-name is repeated.  This is not an error or a weakness; even medium-sized interlockings are distributed over several files.

The output file will be called the same as the input file, but with `.trk` replaced by `.xref`.

## Source locator / Emacs interoperation

This feature allows you to locate the pseudo-Lisp source for any relay in the interlocking by clicking right on its coil or contact in the Relay Draftsperson drawings. If set up properly, an external editor or other program of your choice will navigate to the relay definition in the source file so you can edit it (and then type control- (Cmd-) R to NXSYS to reload the fixed version.

The NXSYS feature relies on being told a key piece of information, viz., exactly what external program or BAT/bash script to execute to tell about the relay you want to edit.  Although the purpose and action are identical on both systems (Windows and Mac), the way and place you put the information differs.  This script, which you supply, is called the *SourceLocatorScript* (It needn't be a script, it could be an executable program, but we will assume script for now, BASH on Mac and BAT on Windows).

On both systems, you edit the source of a relay by clicking right on its coil or contact in the Relay Draftsperson.  Now, on either system, you will get a small context menu allowing you to close or hide the window, but, if the Source Locator Script has been set up properly, you will also see a new option, `Edit Source 158HY` (or whatever the relay name is). Select that and your editor will be fed the string `158HY` in a way that you prescribe.  I will tell you how to do it for Emacs (including Mac Aquamacs).

#### Writing the Source Locator Script

The content of the Source Locator Script depends entirely on the editor or scheme you are using.  I am going to assume Emacs (including [Aquamacs](https://aquamacs.org/)) with its [comprehensive ”tags” facility](https://www.emacswiki.org/emacs/EmacsTags) for locating defined entities in multiple computer languages.

For Emacs/Aquamacs, using the native TAGS facility, the content should look like this:
~~~
/usr/local/bin/emacsclient -e "(find-tag \"$1\")"         (Mac)

c:\"Program Files"\Emacs\x86_64\bin\emacsclient -e "(find-tag """%1""")"   (Windows)
~~~

Note that these assume that Emacs and its "emacsclient" are installed in the default place; they should be changed appropriately if not.

You can call this script anything at all you want, `EmacsSourceLocator.sh` or `EmacsSourceLocator.bat` (Mac, Windows).  On the Mac, you must, as with all shell scripts, `chmod +x ...` to allow it to be executed.

If you're not using Emacs or Aquamacs, but some scheme of your own, you presumably know what you should put in your Source Locator Script.  It will be called as a command with one argument, the name of the relay whose source is sought, which cannot contain spaces, quotes, parentheses, or other problematic characters.

#### Identifying the Source Locator Script to NXSYS

This is the tricky part.  In the Olde Days, I'd tell you to add a line to the INIT file, but these days a graduate degree in computer science is needed.  On Windows, we use the Registry.  If you are not competent to edit the registry, get help.  You need Admin access, too, to set this up. On the Mac, you have to use an obscure program, `PListBuddy`, or, if you are an XCode adept, that is even simpler.

##### Windows

Basically, you have to set the pathname of your Source Locator Script as the value of the Registry variable called, oddly enough, `SourceLocatorScript` in NXSYS' `HKEY_LOCAL_MACHINE` Registry key.  The Registry pathname of NXSYS' Registry key is (or should be), logically enough:

`HKEY_LOCAL_MACHINE\Software\B.Greenberg\NXSYS

NXSYS does not create registry keys for you, because that is a privileged operation and would not work if you are not an administrator.  Normally, registry keys get created by the installer, which can only be run as an administrator.  But NXSYS doesn't have an installer.   I know quite a bit about writing Microsoft Installers, and I don't want to do that right now (it is a ton of work and detail).  So you have to log in as admin, and create "B.Greenberg" and "NXSYS" under me in your Software Registry "Hive".

When you, or your administrator, has created that key, create a new “string value” called `SourceLocatorScript`, and give it a value of the full pathname, including drive, of your source locator script, e.g., `c:\Users\joe\EmacsSourceLocatorScript.bat`.  If this is not done, the Edit Source menu item will not appear on the Relay Draftsperson right-click menu.

NXSYS inspects this value when it starts up. If you set it while NXSYS is running, you will have to restart it for the setting to be noticed.

(NB for heavy-duty Windows adepts (changed from "hackers"): NXSYS, even though *currently* a 32-bit app, uses [KEY\_WOW64\_64KEY](https://docs.microsoft.com/en-us/windows/win32/winprog64/example-of-registry-reflection-and-redirection-on-wow64) to access the “64-bit registry”, which is to say, the one you get when you use `regedit` and don’t know how Windows 10 redirects the registry requests of 32-bit apps unless this is done).

##### On the Mac

You don't need any special privilege here, but you do have to know how to use `PListBuddy` or XCode to edit “plists”.  `PListBuddy` should be in `/usr/libexec/PListBuddy` on a healthy Mac.  There are several good articles on how to use it. [Here's one](https://www.marcosantadev.com/manage-plist-files-plistbuddy/). Or use XCode, if you are skilled in its use and have it (it is very large and complex -- if you do not already use Xcode, don’t attempt to for this).

You have to find NXSYS’s `plist`.  It should be at
~~~
~/Library/Preferences/BernardGreenberg.NXSYSMac.plist
~~~
(it will always be my name, as the author/vendor, not yours!).  Once you have skill in simple `plist` editing, do the same as on Window, i.e., add a string value called `SourceLocatorScript` with a value of the full pathname from `/` of your source locator script.  Again, skip this or do it wrongly, and NXSYS will not offer `Source Edit`.

### Use under Emacs/Aquamacs

This has been tested under Emacs (Windows build) 27, and Aquamacs 3.5 (Mac only).

In order for Emacs or Aquamacs to “hear” the messages sent to it by `emacsclient`, you have to “start the Emacs server“ in Emacs (or Aquamacs).  This is done with the command `M-x server-start`.  If you are an Emacs adept, you can do that in your startup (i.e., add `(server-start)`), if you want.  But fail and the commands sent from NXSYS will not be heard. Here is [additional info on the Emacs server](https://wikemacs.org/wiki/Emacs_server).

There are two more changes to Emacs you should make if you want to edit `.trk` files in Emacs’ powerful [Lisp mode](https://www.emacswiki.org/emacs/EmacsLispMode).  You definitely want to tell it that all `.trk` files are to be edited in Lisp mode, and have to modify the syntax of exclamation point to not be a valid ”word” character. Unless you do this, if you click, in Emacs, on a back-contact term such as `!153RWK`, Emacs will fail to find the source for putative relay `!153RWK`, not knowing that the ! should not be included in the name.


To effect these changes, add this Lisp code verbatim, copy-paste, to your Emacs / Aquamacs startup (or make one):
~~~
(add-to-list 'auto-mode-alist '("\\.trk\\'" . lisp-mode)) 

(defun my-trk-mode-setup ()
  (modify-syntax-entry ?! "."))

(defun my-after-change-major-mode ()
  "Custom `after-change-major-mode-hook' behaviors."
  (when (derived-mode-p 'lisp-mode)
    (let ((fname (buffer-file-name)))
      (when (and fname (string-suffix-p ".trk" fname))
	      (my-trk-mode-setup)))))

(add-hook 'after-change-major-mode-hook 'bsg-after-change-major-mode)
~~~

You may or may not have an Emacs startup.  If you are using regular Emacs (e.g., on Windows), you probably don’t, and if you're using Aquamacs, you probably do. On Windows, it is a file called `.emacs` in your home directory, `c:\Users\kathy` or whatever (Microsoft is working to make it hard to access).  If you are using Aquamacs, it is `~/Library/Preferences/Aquamacs Emacs/Preferences.el` (`el` stands for “Emacs Lisp”, although `.emacs` in your home directory is read, too, which covers the “regular Emacs” case).

### Editing interlockings with Lisp Mode

If you have done all this, and created a TAGS files for the interlocking you want to edit (yes, it is still useful after editing and changing line numbers), and have started the emacs server, editing interlocking is now trivial (assuming you know how to design and debug them).

Read in as many interlocking `.trk` files as you want, and use all Emacs tools.  `m-X visit-tags-table` to the `TAGS` file you have created. Now, you can simply point to any term in the logic of any relay, and type `m-.`, that is, “meta dot”, and *presto*, the definition of that relay (whose contact you clicked on) will come up!  `C-u m-.` (control U, Meta .) and you can type in the name of a relay, and it will be found!  Alphabetic case ignored!

From NXSYS, simply click-right on a relay contact or coil, and if Source Editing is offered, select it, and Emacs / Aquamacs will find that relay and focus it (although it will not bring itself to the top of the screen; you have to switch it or keep it visible).

#### By the way

The use of Lisp to extend Emacs on non-Lisp machines is [my 1978 innovation](https://multicians.org/mepap.html).  GNU Emacs’ choice of Lisp, years later, was informed by my work.


