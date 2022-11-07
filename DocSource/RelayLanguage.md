# NXSYS scenario and relay language
##### Status 4 April 2022


This file documents the language used with NXSYS to define track scenarios and interlockings. Using this language, and looking at the supplied examples, you should be able to define your own layouts.

The term *the simulator* is used below to denote the NXSYS application in such contexts where there is a need to draw a clear distinction between logic and actions effected by that application, and that effected by the relays whose definitions you supply to it.  The simulator operates some of your relays, and is operated by others of them, as described below.

Making the signals and switches actually work is not so easy: You must learn signalling logic design.   Consume everything in the main helpfile, then go on to the “(Advanced) Signalling Logic Tutorial" in the application help menu (or the `Documentation` folder in [this GitHub repository](https://github.com/BernardGreenberg/NXSYS)).  Also, study the 1998 tutorial [Duckburg Tower A](https://github.com/BernardGreenberg/NXSYS/tree/master/Interlockings/Duckburg)!

Of course, you are free and encouraged to study and mimic the circuitry provided in the examples; you can start with what we supply and modify until it becomes something else.  With NXSYS, you can actually *watch* the relays in operation and experiment with modifying their circuits, privileges I never had when learning this material from paper in the mid-1960's.  (However, I warn and declare unequivocally that my designs are for entertainment and educational purposes only and include certain simplifications and possible errors, and are not fit for use for controlling real railroads or other life-critical systems, and I assume no risk or responsibility should any of this information, or designs correctly or incorrectly derived from it, be faulty and lead to harm, damage, loss, or injury. Use it at your own risk.)

The [NXSYS web page](https://BernardGreenberg.com/Subway) on my [personal site](https://BernardGreenberg.com) is presently the official location for executables. Critical updates will be posted there.  [This GitHub repository](https://github.com/BernardGreenberg/NXSYS) is the authoritative source.

### NXSYS pseudo-Lisp

The relay language is a modified subset of [Lisp](https://en.wikipedia.org/wiki/Lisp_\(programming_language\)). In effect, this means little more than that parentheses define *forms* recursively and extant Lisp editing tools (e.g., Emacs) may be used to advantage.  All whitespace is equivalent, case is insignificant except in quoted strings, and semicolons start a rest-of-line comment.

A **form** is either an **atom** or a **list**.  A **list** starts and ends with an open parenthesis and a close parenthesis, and contains zero or more forms as its **elements**. Here are the known types of atoms in NXSYS Lisp, with examples


|Example | Description|
---------|-----------
|2345       | Integer, (possibly) signed, decimal|
|foobar     | Symbol (string of alphanumerics starting               with alphabetic). Used to cause references to the same thing.  CASE is INSigGNiFICANt! Colon (:) and hyphen (-) are legal "alphabetics", e.g. `:ALL-GROUNDHOGS` is a legal symbol name.
|17NWK      |Relay Symbol.  Starts with digits, ends with alphabetics and/or digits. CASE IS INSIGNIFICANT! (NB: I hope to expand this syntax).
|"Title"    |Quoted string.  Used where text must appear.  Backslash escapes backslash or quote.|
|#\A        |Single character object—used for subway line designations. (See below for handling BB & MM.)|
|!24HY      |Back contact Relay Symbol. Used for back (closed when dropped) contacts.|
|782.3      |Floating-point number|
|355/113    |Rational number|

Top-level (not contained in other forms) forms define the track layout, its signals, and its relays.

### Interlocking definitions
 
A `ROUTE` form must appear first in the first file.  Unfortunately, it bears a couple of Version 1 compatibility artifacts presently devoid of meaning, but extant interlockings have them.

		(route "Progman St. Interlocking  --  of 12/8/94" #\A  00       north ...
		(ROUTE  "Title string for window              " Line-ID Origin  direction
        
*Line-ID* is the single letter used on signal plates for the whole layout, e.g., `#\F` in **F4-334** (i.e., Fourth Avenue BMT). Note that in 2.5 and later, `:EXTENDED-ROUTE-LETTER` can override this with a string of more than one character.  Yes, multiple subway lines joining don't work well (planned).
   
*Origin* is a Version 1 artifact that is ignored in Version 2 NXSYS. It must be a number, and 0 will do well. *Direction* is another Version 1 artifact meaningless in Version 2 NXSYS, but anything other than `NORTH` or `SOUTH` will err. Sorry.

After *direction*, zero or more **property pairs** can appear, each being a **Symbol** and a value. Unknown or misspelled properties are ***guaranteed to be ignored***.  This opens the door to downward-compatible, conditionally-used features. These are most definitely still relevant.

The following are available to parameterize the "train" system:

		:CRUISING-FEET-PER-SECOND number
		:TRAIN-LENGTH-FEET number
		:YELLOW-FEET-PER-SECOND number

*Cruising* refers to the target speed for distant (green) indications. *Yellow* refers to the home indication (prepare to stop at next signal).  Train kinematics are inferred from these parameters and distances inferred from signal/IJ stationing in hundreds of feet.

The pair `:IRT T` causes IRT-style signal numbering to be displayed on the plates, and track section circuit ID numbers to be interpreted by the IRT convention, i.e., 

			10*stationing_point + track_number

The pair `:TORONTO T` causes Toronto-style signal numbering to be displayed on the signal plates.

		:HELP-TEXT ("Menu string" "Help-string" ["@help.html"])

creates interlocking-specific documentation offered on the "Help" menu with the string used as *Menu string* (which also is used as the help window title, so avoid ampersands (i.e., to designate menu accelerators).


`Help-string` will be taken as help text to be displayed as text, unless it starts with `@`, in which case will be taken as a pathname (relative to the `.trk`-containing directory) of a text file containing the documentation text.

The third argument is optional, and only recognized by 2.6 and newer (it will be ignored by earlier versions).  If supplied, it must be a string starting with `@` and ending with `.html`.  The application will web-browse that file in the containing directory, in the HTML help viewer on the Mac and the system browser on Windows (If you want it to work on older versions, you must supply a real text file, too).

These next two property pairs are only recognized in NXSYS 2.5.0 and newer, and, by careful contract, are *ignored* by earlier versions:

		:TRAFFIC-LEVERS-TRISTATE T
		
Enables three-state, center-neutral traffic-control knobs. If elected, the interlocking logic must work with or without them. See the Release Notes.

		:EXTENDED-ROUTE-LETTER "BB"
		
(or "MM" or whatever), a *string*, which NXSYS 2.5 and later will use in signal plates instead of the `LINE-ID` letter.

### Defining track, signals, and switches

**Use TLEdit!**

You create track and panel layout with TLEdit, including all the traffic levers, random keys/controls, etc.  The `LAYOUT` form used is not intended for user editing, but described, for completeness only, [below](#layout).  You should never have to look at it, and must not edit it by hand lest you create an unloadable, uneditable file.  You must include the TLEdit-generated track layout with an `include` form, e.g.,

			(include "GrandCentralLayout.trk")
     
and never edit it, other than by using TLEdit.

### Defining relays

Finally, one defines the relays.  I will give brief descriptions of the important known relay nomenclatures below, although the technique of interlocking design will not be explained here, although much may be inferred from my prototypes.

Relays are defined by `RELAY` forms, which look like this:

    (RELAY 4PBS expression1 expression2 expression3 .... )
    
i.e.,

		RELAY name  expressions.

*name* must be a Relay Symbol for the relay being defined.  The *expressions* are zero or more terms to be *and*'ed, i.e., wired in series.  Valid expressions are:

|     Type       |          Example|
|-------|------|
|Front contact|`13RWC`|
|Back contact|`!22XS`|
|Terms in parallel|`(OR 22PBS 22XR .... )`|
|Terms in series|`(AND 15NWC 17NWC !10AS)`|
|Label definition|`(LABEL L4HY01` *terms to be wired in series* `)`|
|Reference to label|`L4HY01`     ;symbol/atom, not relay symbol!!!|
|Macro call|`(SWIF2 7 10AS 12AS)`|

Labels are used to define common subexpressions that would implemented in real relays by shared wires. `(LABEL name expr1 expr2 ....)` defines the AND (series wiring) of all the *expr*'s in it as *name*, and allows that name to be used as a synonym in subsequent relay definitions.

(2022: `LABEL`s were a weak attempt to “shared wiring” between relays, both to reduce error-prone code duplication and strive towards what I call “topomorphic circuits”, that is, circuitry whose topology mirrors that of the track layout.  This is an ingenious and venerable technique used ubiquitously by the real circuitry. See the logic document for a good example. Neither the NXSYS Lisp language nor the current simulator (let alone the relay graphics system!) is adequate to represent such circuits.)

**Macros** are used to define standardized identical relay definitions, identical in shape but not in relay numbers.  For example.

    (defrmacro SWIFNC 3
               (AND (OR 1NWC (arg 2)) 
                    (OR 1RWC (arg 3))))   ;8 Dec 1994
 
In general,

    (DEFRMACRO  name  n template)

*Name* must be a symbol, and *n* is the number of arguments expected. The actual arguments may be numbers or relay names:
  
    (SWIFINC 23 10AS 12AS)
             1   2    3

In the *template*, `(arg 2)` substitutes all of the second argument to the macro call, and references to relay symbols cause the switch or signal numbers of the corresponding argument to be substituted, i.e., `(OR 1NWC...` becomes `(OR 23NWC` in the above.

Note that macros may be used both as "top-level forms" in a file, or as subforms of other relay definitions, even in other macros.

TIMER forms are just like other relay forms, except that a time in seconds appears after the relay name:

    (timer 4U 7 (OR 4AS......
 
This defines a timer relay that closes its named contacts after 7 continuous seconds of its defining terms being true.

The following relays are known to the simulator.  Some are read by it to update the panel, and others are operated by it to reflect interaction. If you don't have them, the corresponding feature will not be implemented for the object in question.  If you have them, they be used as expected.

### Relays read by the simulator

These relays are read (i.e., logically observed) by the simulator to operate the panel, and which the (your) relay logic must drive to make the panel "work".  They are called *reporting relays* in the code, and trigger nomenclature-specific code execution when they change state.

Note that numeric part of a relay name is the lever number for those relays that have it, and track section (IJ) otherwise.

|Relay|Function|
|---- | -------|
|**For signal**||
|H|Home - when picked, signal is clear|
|D|Distant. When signal is clear, picked if signal is green.     (If HV and/or DV are found, they will be used instead of          H and/or D, although H will be used for the GK light).|
|FL|Fleeting relay.  Created and read by simulator.|
|DR|For home signal, picked when cleared to diverging route.|
|CO|For home signal, call-on when picked.|
|SK|White `S` light (with yellow) in advance of GT signals.|
|DK|White `D` light (with yellow) in advance of GT signals. Signal must be clear (HV, if exists) or H to display.|
|STR|ST20 light, only displayed if signal is red.|
|LH|Lunar White light. Don't turn off until high signal clears            (e.g., during stop drive).|
|V|Stop (trip).  When picked, stop is held clear.|
|K|Exit light at signal.|
|PBS|Push button stick.  Fundamental initiation relay. Only read by the simulator to light GK light, and enable dropping by mouse click (see below).|
|LS|Switch lock (stick) - switch free when picked, locked (and red|lock light lit) when dropped. Gates RLP/NLP to RWZ/NWZ.|
|**For track**||
|T|Track.  When picked, track section is vacant.|
|K|Route.  When picked, white route line-of-light is shown.|
|**For switch**|(A and B operated together)|
|NWZ|Normal switch control - picked when switch wants to be normal (don't we all?)
|RWZ|Reverse switch control - picked when switch wants to be reverse.|
|**For traffic lever**|
|NFK|Normal direction white indicator|
|NFKR|Normal direction red indicator|
|RFK|Reverse direction white indicator|
|RFKR|Reverse direction red indicator|

### Relays operated by the simulator

The following are the relays operated by the simulator in response to the panel, providing input to rest of the relay system from the user interface. They are called *Quisling relays* in the code, in dishonor of Vidkun Quisling (1887-1945), the Norwegian traitor who operated his government on orders from the Nazi regime.

The "PB"'s are pulsed by the simulator.

|Relay|Explanation|
|-----|-----------|
|0CPB|is a huge simulated relay which is pulsed to drop every signal in the simulator, so a back contact appears in all PBS's. As of 19 Feb 97, this is no longer necessary; the simulator drops all PBS's known to its signals when required.  0CPB is still pulsed, though, and can be used elsewhere.|
|0RAS|is similar, and pulsed to release all approach locking on menu command (and must be used in all AS relays).
|0BRGP|"Bobble red signal repeaters".  This relay is pulsed by the `Interlocking > Bobble red signal repeaters` menu command, and simulates temporary track power failures. `RGP` circuits must include its contact.|
|**For signal**||
|PB|Push button (initiate or exit) at signal on control panel.|
|COPB|Trackside call-on push-button on home signals.|
|FL|Fleeting - turned on or off by menu command on signal,|
|XPB|Exit push button - pulsed by click on lit exit light.|
|PBS|Push button stick.  Picked when successful initiation at              signal. Only operated by the simulator to drop when GK clicked              on.|
|**For switch**||
|NL|Normal lever - pulsed to request switch normal (right click or aux key)|
|RL|Reverse lever - pulsed to request switch reverse (ditto)|
|NWP|Normal Switch repeater - switch actually in that position.|
|RWP|Reverse Switch repeater|
|**for track**||
|T|Track occupation. Picked when track section vacant. Can be generated by mouse, the "train" system, etc.|
|**For stop**|"V" for "valve" (electropneumatic)|
|NVP|The train stop has come fully to the normal (tripping) position.|
|RVP|The train stop has come fully to the clear position.|
|**For traffic lever**|This has differing semantics in 2.5 and later if the interlocking **elects tristate traffic control in that case**. See below. Call that case **3ST**, and the earlier (compatible case) **2ST**|
|NL (2ST)|Traffic lever knob in normal position.(Picked at NXSYS load time).|
|RL (2ST)|Traffic lever knob in reverse position. Note that these merely reflect the KNOB state, not the logical traffic lever state (which is implemented by relays)-- the knob can be used to control anything.  No flashing is currently available.|
|NL (3ST)|The operator has turned (and will immediately release) the knob to the normal position.|
|RL (3ST)|The operator has turned (and will immediately release) the knob to the reverse position.|
||In 2ST, one or the other of NL or RL is picked at all times. In 3ST, neither is picked unless the operator is actually operating the control.|

The tristate traffic control situation is complex because interlockings exploiting it, or, rather, *able to* exploit it, must be carefully coded to work compatibly in 2.5 and earlier, without the feature, and, of course, older interlockings must work flawlessly in 2.5 and later. Please read [the helpfile on this feature](https://github.com/BernardGreenberg/NXSYS/blob/master/Documentation/TrafficLevers2_5.html).

**The simulator initializes all relays with nomenclatures RGP, AS, D, DV at interlocking load time by computing their state by evaluating their expressions** - this is a very inelegant solution to a fairly massive initial-state problem not yet solved in general.

### Other relays of honorable mention

Here are some other relays, not known to the simulator proper. See the main NXSYS helpfile for explanation of the Vital/Non-vital distinction, and the logic document for even more.

|Relay|Semantics|
|-----|----------|
|**For signal**||
|XS|Exit stick - picked when exit at this signal selected.|
|R|Route relay/ Picked when complete route at signal established.
|HY|Slotting (control section) - home signal - picked when the control section is clear.|
|COS|Call on stick - picked when call on is selected by tower operator."
|AS|Approach stick - picked when approach locking is clear.  Most general test for signal considered safely to be at stop.|
|U|Approach locking timer (also called U for track grade timer, though).|
|VS|Stop stick.  Forces train stop down in reverse direction.|
|XL|Exit lockout.  Sometimes used. When dropped, prohibits exit there.|
|XR|End-to-end routing: forward probe from one subnetwork to next.|
|ZS|End-to-end routing: successful exit selection returning in reverse direction.|
|**For switch**| (all "when picked")|
|RWZ, NWZ|Switch control -- switch legitimately wants to be that position.|
|RWC, NWC|Switch in correspondence in that position.|
|RLP, NLP|Switch called for in that position, even if locked otherwise.|
|RWK, NWK|Switch in correspondence, **and** locked **or** called for in that position.
|ANN, BNN, ANS, BNS, RN, RS|NX route selection magic relays.    Example: 17ANS = "contemplating a move over 17A, Normal position, South direction."  When a corresponding pair (e.g., 17BNN/17BNS, 17RN/17RS) is picked, NLP or RLP is called to make the switch move.  This is the heart of NX/UR.|
|**for track**||
|NS, SS|North/South stick.  Implements "route locking" (see main helpfile). Normally picked.  Dropped in linked chains over a route to lock switches, prevent opposing movement, and put white lines of light on board. Retained by either occupation, the previous section's identical relay, or signal AS.|
|U|GT/ST signal timing.|

**Relays operated by “random button” controls, and operating "random panel lights" and the like are usually named by you in TLEdit.**  If you care about the superterrific “Train ID button system” and its lights and buttons, study [myrautop.trk at Myrtle Av.](https://github.com/BernardGreenberg/NXSYS/blob/master/Interlockings/Myrtle/myrautop.trk) and maybe the couldn't-be-cooler [dynmenu.cpp](https://github.com/BernardGreenberg/NXSYS/blob/master/NXSYS/v2/dynmenu.cpp) which implements its model.

You can define any other relays you want, freely. Right now their names must be valid “relay symbols”, i.e., digits followed by alphabetics.  The number will associate it with a track section or lever, and it will show up on relay list dialogs for that object.

There is a new class of "optional" relays, currently including


|  Relay | Semantics|
|----|---|
|CLK (switch) |If picked, which should only happen when the switch track section is neither occupied nor routed, the points of the switch will flash in white in whatever position they are in.|
|CLK (signal)|If picked, the GK light for the signal, if lit, will flash. If this relay exists for the signal, the builtin call-on processing that flashes the GK light will not be performed - the CLK relay assumes this responsibility and must be programmed for it.|
|BPBS (signal)|If the simulator can't find PBS for a signal, but can find BPBS, which is used with ST signals, it will use it instead for GK light indication, fleeting, and cancelling.|
|LK (switch)|The simulator will look for this, and if not found, use LS. It will use the back contact of the one found to drive the switch lock light, lock out the switch auxiliary key and switch control motion.  LK must be used (as in 1957, but not 1994, 733-33 standard) if there are conditional crosslocks in NWZ/RWZ that do not appear in LS.|

These optional relays need not be coded, but if they are, they will be utilized as  indicated above.

### “Special timing consideration”

With one exception, relay definitions may appear in the source file in any order.  If the circuit is correctly designed and race-free, the order of relays will not matter.  There is, however, one case in the standard interlocking design where relative timing is critical, and thus the detail of the simulator's algorithm, and hence, source order, become considerations.

The Relay evaluation algorithm is as follows: when a relay is picked or dropped, AND this is really a change from its previous state, i.e., a real transition, the identities of all relays that depend on that relay are put in a queue in the order they appear in the source file. Then all of these relays are visited, their states recomputed, and if their states change, their respective "dependents" are added to the end of the "needs work" queue.  This procedure is continued until the queue is empty.

This algorithm works pretty well most of the time, far better than simply recursing over relays.  Computation of new state is fast.  The only time this algorithm is not transparent is when relay B repeats relay A, and a third relay, C, has a stick circuit broken over `(OR !B A)`, such as home signal PBS.  The real circuit drops A, which drops B: in the time interval between A's breaking and B's picking up, C drops.  This will not work in the simulator by the algorithm described above, and one must ensure that C is defined before B in the source file, to make sure that C sees the change of A before B does.  This is the case with HY (and COS) and PBS: The **PBS for a home signal must be defined before its HY or COS**. Dwarf signals, having no HY, require TP and TPP to repeat T to make the same circuit work, for the same reason.

-------

#### Additional special forms (19 February 1997)

The Common Lisp special forms `COMMENT`, `EVAL-WHEN`, and `INCLUDE` are supported by NXSYS and the compiler.


`COMMENT`   all subforms are read, but ignored when it appears at top level. Example

    (COMMENT (RELAY 240R 240PBS 125RWK 127NWK 212XS)) ;;old 240R
    (RELAY 240R          ;;real definition.

`(EVAL-WHEN  tags form1 form2 form3 ....)`
 *tags* is a list, either (), `(EVAL), (LOAD), (LOAD )` etc.
             if EVAL appears among the keys, the forms will be processed when the interpreted file is loaded.  If LOAD appears among the keys, the forms will be processed by the compiler as though  they had appeared at top level, but (unless EVAL appears) ignored when the file is loaded interpreted. **[2022: Don't use it or try to understand it.]**
             
`INCLUDE`    e.g., `(INCLUDE "stdmacs.trk")`.  The single argument is a pathname relative to the file which contains the INCLUDE.  The forms and definitions in the file will be treated by NXSYS and by the compiler as though they appeared in the file at the point of the INCLUDE form.

A special relay called `0LogicHalt` has been defined.  If you provide a definition for this relay, and it ever picks, at that time a message box will pop up stating this: if you press "Yes", the relay simulator engine will halt and allow you to examine its state in detail with the Draftsperson and other tools.  You must (currently) reload the interlocking after such a breakpoint.

<a id="layout"></a>
#### Track Layout Language

The Lisp-embedded language used in track layout files generated by TLEdit is, contrary to prior (sarcastic) claims, not at all obvious.  You should never have to deal with it or code in it, unless bugs in either TLEdit or NXSYS proper require immediate fix and I am not available.  That should not happen, but in NXSYS, as with real rapid transit, errors do happen.

The layout is a graph (in the mathematical sense), of “nodes” and “rays” (arcs). Nodes are either 
+ Insulated joints, from which emanate either 1 or 2 rays, 1 in the case of a track termination/end, and 2 otherwise.
+ “Kinks”, i.e., uninsulated track bends, also 1 or 2 rays, but track terminations are forcibly insulated at “save” time. Of course, real tracks do not join or bend at angles, but panel track representations typically do.
+ Switches, from which invariably emanate 3 rays (*tres rayos*, no *Tres Reyes*).

“Rays” are track segments, which always connect two nodes. Every node thus has an *order*, the number of rays emanating from it, of 1, 2, or 3: 1 is a track termination, 2 a non-terminal insulated joint or kink, and 3 a switch.

The `LAYOUT` form consists of
+ A  `VIEW-ORIGIN` subform defining the initial screen position on the layout
+ Any number of `PATH` subforms, described below, describing the graph
+ Any number of “random object” subforms, defining signals, exit lights, traffic levers, and other panel artifacts.

Each `PATH` describes a journey from one track termination or switch to another by the nodes it visits. Paths with neither (i.e., closed loops with no switches) cannot be represented and are diagnosed by TLEdit at “save” time.  Kinks are designated by the absence of an explicit node type.  Track circuit designation subforms (`TC`) are dumped between the nodes of segments having them.

The three rays emanating from a switch are called its *stem*, *normal*, and *reverse* branches.  The dumper notes which it traverses when it does so. Rather than starting a new `PATH` when traversing a switch not before encountered, the dumper incorporates it into the `PATH` being recorded, of necessity either entering or leaving via the *stem*.  The dumper meticulously verifies the validity of its graph-theoretic assumptions, as well as the well-formedness of the graph, to avoid creating unloadable layouts.

Again, hopefully you will never have to deal with any of this. If you really care, study `save.cpp`.



=======================================

(END)
