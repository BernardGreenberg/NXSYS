#Major Deficiencies and Futures

3 April 2024

NXSYS is quite stable right now, in both Windows and Mac versions. All of the known (to me) deficiencies and avenues for conceivable improvement are “major”, i.e., would require massive work (with resultant instability) to remedy.

Because relay-based fixed-block wayside color light signalling is basically obsolete in both function and technology, and targeted for eventual replacement with Communications-Based Train Control (CBTC) wherever it exists, interest in what NXSYS does decreases with every transportation failure in New York City, along with my incentive to address these issues.

Here are what I considered the most significant problems, in descending order of importance.

##Inadequate expressivity in IJAO naming

"IJAO" means "insulated joint associated object", meaning

+ Insulated joints themselves and their corresponding track circuits
+ Signals whose plate numbers derive from IJ identifiers (IJIDs)
+ Relays associated with any of those objects, sharing their names, including, but not limited to, track relays and repeaters, route lockers, and the line relays, timers, and stops of automatic signals.

There are three dimensions in which this is currently deficient, to wit, inability to express or handle any of the following:

1. Stationing numbers of 1000 or greater. For extant NYCT interlockings, this is only a problem on the former IND north of West 4th St, on all routes, but that's a lot.
2. Multiple "route letters" in a single scenario, typically where routes merge, e.g. A4-314 and F4-314 at DeKalb Avenue. This precludes implementing the most important large interlockings (e.g., East New York).
3. Exotic "track numbers", e.g., "Y1A" at E. 180 St or "F" at Stillwell Avenue, or multiple stationing schemas (IRT vs IND/BMT) in one interlocking (again, E. 180 St).  This includes all "yard tracks" at 240th St or any similar yard (and why the "train system" doesn't work there).

While \#1 is the easiest to fix, it is hardly trivial. NXSYS currently uses integers of 10000 or greater (i.e., ***not*** station number with track ID digit either at front or back depending on division) to represent IJ's without assigned identification, and such numbers occur in written-out scenarios. It would thus require an incompatible change to fix, and probably redesign of the management of these numbers in TLEdit.


\#2 and #3 require redesign of the Lisp-subset language in which interlockings are expressed, so that a sufficiently rich language of strings (including embedded hyphens) can be recognized as "relay symbols", as well as abbreviated notations (e.g., the current schemes) available up to the point of ambiguity.  Probably instantiation of "IJ Identifier" as a managed class of C++ objects is required.

Recognition of finite-state languages such as any proposed scheme for Relay Symbols can now be done with C++ regular expressions instead of the cumbersome go-to-laden state-machines in the extant code.

###Train system kinematics vs geometry

Closely related to the IJID problem is the train system's assumption that scaled distances on the right-of-way can be deduced from IJID's.  This would break down completely were multiple "route letters" and junctions of routes handled, but they are not now because of the just-described lacunae of IJID's.  But it already fails when "dishonest" IJID's are used, such as on the yard tracks at 240<sup>th</sup> St, where trains vanish off the map if routed to them.

Optimally, IJ descriptions should contain reasonable physical right-of-way coordinates (possibly including vertically), at least when they do *not* coincide with that implied by the IJID, but the layout description language is not now expressive enough to accommodate such information. Of course, TLEdit would need be changed to author and edit it.  Track circuits might need independent ID's, too.

## Relay network contact timing

The relay simulation algorithm, which is fully described in the section "Special Timing Considerations" in RelayLanguage.md, has "failure cases", one of which is described there, and mandates special care with the relative placement of certain relays in source files.  With that exception, all the supplied interlockings seemingly work perfectly, in spite of what is about to be described.

There are two departures of the relay simulator algorithm from what happens in real subway (IRS).  One is the use of "slow-release" relays of different kinds, which is totally ignored in NXSYS, where there is no such concept or capability.  The extant scenarios just "luck out".

The other is the discrepancy between the time that relays (say B, C D) dropped by the opening of another relay's (say A) contact do so, and the time at which the opposing contact on A closes, which is 0 on NXSYS today, but enough milliseconds to make a difference IRS. It is amazing that it works as well as it does with the current algorithm.  Simulation-like software used by NYCT today, however, does this correctly.

While attempting to develop circuitry for E. 180 St. (2013 dispensation) in spite of the IJID problems, I encountered a fairly simple sub-network of this maximal interlocking that I could not get to operate properly because of issues I traced (with Relay Trace) to this problem. I may post that incomplete scenario for this reason.

Fixing this comprises total redesign of the relay engine for three states (back contacts closed, front contacts closed, in transition = neither), let alone testing and possible repair of all extant scenarios.  The Relay Trace window content would look very different. But make no mistake: this is a serious and fundamental problem.

(Of course, by "timing" I mean "simulated timing" in NXSYS.)

##Topomorphic and other shared-wire circuits

**Topomorphic** is my name for a beautiful and widespread circuitry technique used not only in "all-relay" interlocking in NYCT and elsewhere, but in electromechanical and electropneumatic-mechanical machines of old (and on mainline railroads as well), to conserve wires and contacts while enforcing certain safeties by virtue of topology.  It has no analogue (as it were) in logic gates and arrays.

Current NXSYS relay circuits are all **well-formed logical trees (WFLTs)**. that is, AND and OR expressions of individual relay contacts and contained WFLTs (including the ability to include a front contact of the relay being defined ("stick contact")) as avatars of series and parallel circuits, respectively.  These expressions are identical to, in fact, *are*, Lisp S-expressions in AND and OR.

A simple description of the topomorphic model is the use of circuits that are more complex than WFLTs because of sharing of wiring between multiple relays, often in ingenious ways that exploit opposing traffic (and current flow) directions.   There is a better description and simple examples in the Logic Design PDF document.  NXSYS currently cannot express this at all.

The dividends of topomorphic circuit support would be:

+ Ability to represent actual NYCT circuits, including 733-33 typicals, contact for contact, drawing for drawing
+ Ability to design in and exploit that paradigm
+ Showing incredibly beautiful and impressive relay circuits.

It is to be noted that the interlocking and simulator function so gained **_is zero_**. Such circuits are (arguably) more difficult to debug, as well.

These are the challenges to topomorphic circuitry in NXSYS:

+ The current relay language is limited to WFLTs. Introducing into them labelled "points" that meet their name across circuits, while conceivable, seems not the right way to do this; at very least it begs the question of the subsequent meaning of the containing expression.  A full-featured graphical circuit editor is indicated, and that is a "full-year project", and the question of how it would integrate into the application is substantial. There certainly exist so-called "CAD tools" that can craft arbitrary circuit diagrams, although I do not own or know how to use any, and requiring others to is not reasonable.
+ Representations (in whatever paradigm) for logic in "relay tails", i.e., between the relay coil and supply return (which have no logical import outside of topomorphic circuits) would need be added.
+ A ***linearizer*** would be needed, i.e., functionality that traces all possible unique current paths in each topomorphic circuit **T**, discarding impossibilities, e.g., paths with both front and back contacts of a given relay, to produce a set of discrete evaluable WFLTs identical to today's circuits, one per each relay coil **R<sub>i</sub>** appearing in **T**. It could diagnose certain design errors as well.
+ The present Relay Draftsperson would need be replaced by one of the order of magnitude greater intelligence needed to draw such circuits at all, let alone in "topomorphic shape" (i.e., resembling the associated trackage, the origin of the name).  This would almost certainly be corollary to the aforementioned graphical editor, if that route is chosen, as it were. Showing the linearized circuits (identical to today's) could be a compromise or implementation scaffold, but that would sacrifice the principal dividend, i.e., the beautiful drawings/displays. (Note, however, that optionally viewing the linearized circuits might be necessary for debugging!)
+ Were the Relay Draftsperson to be so enhanced, a new debugging feature (the "test lamp") would be necessary, with which you could click on any *wire* in an active circuit, and see if it is at supply voltage or not, i.e., "on or off". How this would work with pre-linearized circuits is "less than perfectly clear".

This is, thus, of minimal priority.

##MSWindows: Better way to scroll

NXSYS and TLEdit can only be scrolled on Windows with the ancient-origin scroll bars.  It is cumbersome and error-prone and counterintuitive (move scroll-bar to the right, panel moves to the left).  The Mac scrolling via the trackpad is completely intuitive and easy: just grab the panel with two fingers and put it where you want.

Not all Windows machines have trackpads, and Windows is not trackpad oriented, but many Windows machines have touch-screens which would work as well, if the NXSYS windows system handled it, but it doesn't.  Having lived in the Mac world for the past 11 years, I detest Windows development, and don't have a touch-screen machine.

Maybe reversing the direction of the scroll-bar response is all that is needed (i.e., would make it most Mac-like). Somebody who is competent in modern Windows versions and apps probably could do this, or whatever is "right". easily.

##macOS: Migrate Objective-C++ to Swift

Swift was first released at the time I was already creating the Mac version of NXSYS, and it had no C++ interoperability.  Ten years later, Swift 5.9 has plenty, but it's pretty new, and nowhere as smooth as Objective-C++'s.

It would be wonderful to replace the Objective-C++ components of NXSYSMac with Swift code -- Swift is a much better language and free of many ugly historical macOS artifacts.

It's not at all clear how to make such a move, but I can't imagine that Apple will drop support for Objective-C++ any time soon.

##Apple Silicon support

This shouldn't be an issue, as Macs with Apple silicon can run Intel apps with Apple's simulation adapter (Rosetta), and, more significantly, NXSYSMac is built in XCode, which can certainly generate Apple silicon objects, but I don't have an Apple silicon Mac, so I cannot test this. It certainly *ought to* work without change.

##Linux port
"Fuhgeddaboudit" (as they say in Brooklyn).  Much too large and disruptive.

##Video-game graphics ("Cab view")
Forget that, too.  Way beyond the scope of the application.



