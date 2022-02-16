# Relay Indexer and Source Locator

### 16 February 2022

These two features are new to the Windows build, but have been available in the Mac builds for a while.  They are intended to assist those designing NXSYS interlockings in pseudo-Lisp relay circuitry (which is the only way).

These two functions exploit a command-line program, the *Relay Indexer*, which, unsurprisingly enough, is called `RelayIndex(.exe)`, the `.exe` suffix on Windows, not Mac, and in either case supplied self-standing in both distribution zip files.

`RelayIndex` operates on an entire interlocking, from source.  You direct it to the top-level `.trk` file, the same one you open in the application (NXSYS).  It produces two files, a relay cross-reference, and a `TAGS` file, in the same folder as the source to which it was directed.  They are both text files.  You can place the output somewhere else with the `-o (--output)` control argument.

We will discuss them independently.

## Relay cross-reference

The relay cross reference is a listing, in relay order (smaller object (lever, track circuit, etc.) numbers first, alphabetical within the same object) for each relay, of all the relays (including this one, if it has a “stick contact”) who include a contact of this relay in their circuits.  The ”referrers” are sorted in the same way.

Here is an excerpt from the cross-reference of Myrtle Avenue: 
<span style="font-size:50%">
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
</span>
Lines read across, not column-down. The header line for each relay lists the name of the relay, the line number, and the character position in the file where its definition appear.  Of course, those will change if you edit.

Note that 155NL is listed as “undefined".  That is not an error; 155NL is the tower operator's “normal lever control” for switch 155, and 155NL is thus a so-called *Quisling relay;* you don't supply it—the simulator does.

Note also that the file-name is repeated.  This is not an error or a weakness; even medium-sized interlockings are distributed over several files.

The output file will be called the same as the input file, but with `.trk` replaced by `.xref`.

## Source locator / Emacs interoperation

This feature allows you to locate the pseudo-Lisp source for any relay in the interlocking by clicking right on its coil or contact in the Relay Draftsperson drawings. If set up properly, an external editor or other program of your choice will navigate to the relay definition in the source file so you can edit it, and then type control- (Cmd-) R to NXSYS to reload the fixed version.

The NXSYS feature relies on being told a key piece of information, viz., exactly what external program or BAT/bash script to execute to tell about the relay you want to edit. This script, which you supply, is called the *SourceLocatorScript*.

On both systems, you edit the source of a relay by clicking right on its coil or contact in the Relay Draftsperson, whereupon you will get a small “context menu” allowing you to close or hide the window ... and if the Source Locator Script has been set up properly, you will also see a new option, `Edit Source 158HY` (or whatever the relay name is). Select that and your editor will be fed the string `158HY` in a way that you prescribe.  I will tell you how to do it for Emacs (including Mac Aquamacs).

#### Writing the Source Locator Script

The Source Locator Script, which you supply, specifies what is to happen on your system when NXSYS wants to locate the source for a relay.

The content of the script depends entirely on the editor or scheme you are using.  I am going to assume you are using Emacs (including [Aquamacs](https://aquamacs.org/)) with its [comprehensive ”tags” facility](https://www.emacswiki.org/emacs/EmacsTags) for locating defined entities in multiple computer languages.

For Emacs/Aquamacs, using the native TAGS facility, the content should look like this:
~~~
/usr/local/bin/emacsclient -e "(find-tag \"$1\")"         (Mac)

c:\"Program Files"\Emacs\x86_64\bin\emacsclient -e "(find-tag """%1""")"   (Windows)
~~~

`emacsclient` is a command-line application supplied with Emacs that can send messages to a running instance of Emacs (or Aquamacs). The above formula assumes that is installed in the default place where Emacs installation puts it, and should be changed appropriately if you installed it somewhere else.

You can call this script anything at all you want, perhaps `EmacsSourceLocator.sh` or `EmacsSourceLocator.bat` (Mac, Windows).  On the Mac, you must, as with all shell scripts, `chmod +x ...` to allow it to be executed.

If you're not using Emacs or Aquamacs, but some scheme of your own, you presumably know what you should put in your Source Locator Script.  It will be called as a command with one argument, the name of the relay whose source is sought, which will not contain spaces, quotes, parentheses, or other shell-problematic characters.

#### Identifying the Source Locator Script to NXSYS

This has now (NXSYS 2.6.0) become easy.

There is a File Open Dialog in both implementations that allows you to tell NXSYS about your Source Locator script.  On the Mac, it is called up by a new button, `Set Lctr Path`, on the right-hand top of the Relay Draftsperson window (you can expose the latter via the `View` menu). On Windows, it is the identically-named option on the `Control` menu at its top left.  NXSYS will update its per-user data (info-plist or registry key) accordingly.

Both commands assume you will use a `.sh` (Mac) or `.BAT` (Windows) file, and only offer such files in the file dialog.  If you actually have an executable program that you have written for this function, feel free to edit the `HKEY_CURRENT_USER` registry for `B.Greenberg\NXSYS\Settings` (Windows) or info-plist `~/Library/Preferences/BernardGreenberg.NXSYSMac.plist` (Mac) yourself with appropriate tools, in which you are surely proficient if you have actually written such a program.

### Use under Emacs/Aquamacs

This has been tested under Emacs (Windows build) 27, and Aquamacs 3.5 (Mac only).

In order for Emacs or Aquamacs to “hear” the messages sent to it by `emacsclient`, you have to “start the Emacs server“ in Emacs (or Aquamacs).  This is done with the command `M-x server-start`.  If you are an Emacs adept, you can do that in your startup (i.e., add `(server-start)`), if you want.  But fail and the commands sent from NXSYS will not be heard. Here is [additional info on the Emacs server](https://wikemacs.org/wiki/Emacs_server).

There are two customizations of Emacs you should make if you want to edit `.trk` files in Emacs’ powerful [Lisp mode](https://www.emacswiki.org/emacs/EmacsLispMode).  You definitely want to tell it that all `.trk` files are to be edited in Lisp mode, and to modify the syntax of exclamation point to not be a valid ”word” character. Unless you do this, if you click, in Emacs, on a back-contact term such as `!153RWK`, Emacs will fail to find the source for putative relay `!153RWK`, not knowing that the ! should not be included in the name.


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

(add-hook 'after-change-major-mode-hook 'my-after-change-major-mode)
~~~

You may or may not have an Emacs startup.  If you are using regular Emacs (e.g., on Windows), you probably don’t, and if you're using Aquamacs, you probably do. On Windows, it is a file called `.emacs` in your home directory, `c:\Users\kathy` or whatever (Microsoft is working to make it hard to access).  If you are using Aquamacs, it is `~/Library/Preferences/Aquamacs Emacs/Preferences.el` (`el` stands for “Emacs Lisp”, although `.emacs` in your home directory is read, too, which covers the “regular Emacs” case).

### Editing interlockings with Lisp Mode

If you have done all this, and created a TAGS files for the interlocking you want to edit (yes, it is still useful after editing and changing line numbers), and have started the emacs server, editing interlockings is now trivial (assuming you know how to design and debug them).

Read in as many interlocking `.trk` files as you want, and use all Emacs tools.  `m-X visit-tags-table` to the `TAGS` file you have created. Now, you can simply point to any term in the logic of any relay, and type `m-.`, that is, “meta dot”, and *presto*, the definition of that relay (whose contact you clicked on) will come up!  `C-u m-.` (control U, Meta .) and you can type in the name of a relay, and it will be found!  Alphabetic case ignored!

From NXSYS, simply click-right on a relay contact or coil, and if Source Editing is offered, select it, and Emacs / Aquamacs will find that relay and focus it (although it will not bring itself to the top of the screen; you have to switch it or keep it visible).

#### By the way

The use of Lisp to extend Emacs on non-Lisp machines is [my 1978 innovation](https://multicians.org/mepap.html).  GNU Emacs’ choice of Lisp, years later, was informed by my work.


