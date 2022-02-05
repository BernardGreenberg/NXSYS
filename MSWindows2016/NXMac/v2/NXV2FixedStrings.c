
char * V2NXHelpText =

 "
Introduction to Version 2 NXSYS (The Macintosh port, NXSYS / Mac, is of Version 2).

PLEASE BE SURE TO SCROLL DOWN AND READ THIS ENTIRE HELP TEXT!


Version 2 NXSYS supports track layouts of arbitrary extent or geometry as well as 
    several other new features.  This help text describes only the differences from 
    Version 1 NXSYS --- it is assumed that the reader has the Version 1 Helpfile
    accessible(via a the Help | Usage/Help menu item).  Please do not fail to read 
    the specific help texts supplied with, and added to the Help menu by, each 
    interlocking loaded.

Version 2\'s graphic track-layout editor, which is the only way to create layouts, and
    with it the ability to define your own layouts, is not (perhaps yet) being offered 
    with this distribution.

Version 2 NXSYS is used in the exact same way as the latest versions of Version 1 NXSYS,
    with the following changes:
 
 o The sole, 32-bit Windows executable is currently nxv2.exe

 o There is no 16-bit Windows Version 2 NXSYS.  Version 2 NXSYS and the track layout
   editor are 32-bit only, and exploit 32-bit features.

 o There is now (2014) a Mac/OSX version of Version 2, NXSYS / Mac. Please see
   the corresponding item under \"Help\" for features and limitations specific to
   NXSYS / Mac over and above those described here.

 o 3-D graphics (Cab View) do not now exist, and will be a separate product/distribution
   should they come to exist. This omission is due to the fact that Version 2 geometry
   is powerful enough to represent three-dimensional layouts(i.e., fly-over and duck-under
   tracks, such as at Rockaway Blvd.), and the internal description language and 
   track-layout editor are not expressive enough to model that).

 o There is no more concept of \"a track\": layouts are composed of track \"segments\",
   each of which are straight lines, which are composed into both track circuits
   (electrically connected segments with one track relay) and connected to each
   other at their ends and at switches.  There are no more track numbers. There
   are no more standard directions for tracks.

 o The \"train\" system changes somewhat; the concept of \"northbound/southbound\" is
   completely gone, as is \"normal direction\" on a track.  When you create a train,
   you must click on the \"loose end\" of a track to start a train there, in the only
   possible  direction.  You can send trains head-to-head after each other on the
   same track and the signalling is all that prevents a collision. \"Reverse\"
   continues to work, but the only indication you have as to in what direction the
   train is headed is the \"next signal\" display on the train panel.  The position 
   indicator, (\"F4-213+58\") no longer means 58 feet past F4-213, 42 feet from
   F4-214, but 58 feet in whatever direction the train is travelling since passing
   F4-213: it might well be \"against\" the stationing.

 o Insulated joints are now \"first class objects\" and have numbers, as in real life.
   Signal plate numbers are taken from the IJ numbers; when these do not correspond,
   as on bidirectionally-signalled tracks, expect a \"wrong\" (J4 instead of J3)
   plate number, although the track layout editor can hand-tune.  Also, there is now
   no way to express more than one \"letter\" of stationing (M1 instead of J1, or BB
   instead of B).

 o As in real life, but not Version 1 NXSYS, distance/geometry on the interlocking
   panel is not isomorphic to distance/geometry on the property. Distance for timing
   train motion is computed by noting hundreds of feet between insulated joints,
   and scaling the multiple electrically continuous segments between the two points
   to their apparent relative lengths on the interlocking panel;  this provides a
   credible result in almost all cases (there is additional cleverness beyond that).
   This technique, however, is unsuitable for accurate 3-D representations, and in
   some cases (e.g., yards) where nonstandard insulated joint paradigms are employed,
   creates \"black holes\"  where train-system \"trains\" cannot go lest they stall or vanish.

 o Fleeted signals show a little green triangle on their stems.

 o In the demo system, (TRAIN n CREATE t ...), t is not a track number but an IJ id
   of an end of a track.   Also, be sure to identify approach signals who are members
   of strings of such as 7162, etc., in \"(SIGNAL\" forms.

 o Although we do not now supply tools or information for creating your own NXSYS V2
   interlockings, the relay language is identical to that (now) used in NXSYS V1, a
   new, incompatible format of track-layout specification is employed.  Binary object
   format per se has not changed, and either version will diagnose the other\'s files,
   although Version 1 layouts are not currently loadable by version 2 NXSYS.

 o For interlockings that offer such a feature (via defining/referencing the relay
   0AZ), NXSYS offers \"Enable Automatic Control\" in the \"Interlocking\" menu.  This
   can be checked on or off (not reset at reload time), and is grayed if the loaded
   interlocking does not offer it.  This is similar to the automatic features in some
   NYCTA and other interlockings, driven by \"trackside Train ID boxes\" (pushbutton
   menus) operated by train operators.

 o Version 2 supports signals with lever numbers prefixed with A, B, C, etc. Currently, 
   lever numbers greater than 7000 get the 1000\'s converted into A, B, C, D, etc, and
   the remainder becomes the lever number.  This should only be interesting if you are
   creating your own or looking at the relays, and this may well change in the future
   as it is highly inadequate (a more flexible namespace of relay nomenclature is
   sorely needed).

 o Station platform graphics (other than appropriated track line segments) are not 
   presently available.
     
======================================================================================
CREDIT AND DISCLAIMER

NXSYS, NXSYS Version 2, and NXSYS / Mac Copyright (c) 1994-2001, 2014 Bernard S. Greenberg.

The author of this work is not affiliated with or supported or endorsed by any transit
    provider or signalling concern.  This system is intended for and licensed exclusively
    for personal educational and recreational use.  The author assumes no liability for any
    use, specifically assuming no liability for harm, damage, or injury resulting from
    attempted use in controlling real vehicles or other real systems, or for inaccuracies
    in application or representation of signalling standards or representations of railroad
    properties and equipment.  The author assumes no liability for damage or harm to computer
    systems or files caused by bugs, program errors, or corruption.  No warranty, assurance
    of merchantability nor suitability for any purpose is given or implied. No license for
    sale, resale, redistribution, or derivative works is granted or implied.

The author may be contacted at nycsubway.org.  You\'ll find me on the signal pages.
Reference/tutorial information on NYC Subway signalling (by the author) may be found at
     http://www.nycsubway.org  - look for \"behind the scenes\" on the navigation bar,
     then \"Signals.\"  A URL for the NXSYS web site may be found there.

";
