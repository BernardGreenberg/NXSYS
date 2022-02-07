#  Duckburg Tower A Interlocking 
### Interlocking implementation tutorial
### 15-21 January 1998
### Copyright ©1998, 2022 Bernard S. Greenberg

This folder contains a 1998 tutorial, seven successive stages of implementation of a good-sized yard/"railroad"-type (imaginary) interlocking ("Duckburg Tower A") in NXSYS Version 2 (still works in 2.5.1) from switches alone operative to a completely functioning interlocking to spec.  Each successive file `db1-db7.trk ` adds additional features.  Each file is heavily commented, especially marked as to what changed from the previous stage, and intended to be studied and learned from (although comments more than 2 or 3 versions old which are either very lengthy or
obsoleted by later comments get removed).

Each of these interlocking implementations actually responds with
from minimal to full function.  They all operate on exactly the same
physical layout (defined in `dburg.trk` and included from that), You
need `stdmacs.trk`, too.

Although this interlocking is colorful and spacious, it is nowhere as
complicated as, say, Myrtle Avenue (nowhere as dense, no grade-level
crossings, fewer conditional crosslocks, no ST, etc).  On the other
hand, it does contain a wide variety of sub-layouts (singleton,
diamond crossover, yard ladder, long stretch of automatic-signalled
pseudo-traffic-controlled loop track), and a lot can be learned by
studying them.

- `db1` will set up routes around switches, but not clear signals, or
  set up end to end routes.  It has nothing more than non-vital switch
  selection circuitry, which is the heart of NX/UR and the most important
  thing to learn and the logic responsible for the most characteristic
  part of the NX/UR behavior.  You can see routes on entrance/exit lights.

- `db2` adds the substantial complexity required to couple sub-interlockings
  via "end-to-end" or "through" routes.  Some route lights are lit by
  temporary wiring, and entrance/exit lights remain on.]

- `db3` adds the necessary logic to deal with the implications of overlap
  on switch position, including approach signals.  The approach signals
  become functional.

- `db4` adds route locking (including stop route-locking), and "stubs" for
  approach-locking relays just to get the route-locking operative, and
  the result is a quite convincing approximation of the real thing.
  (although you cannot run trains through it because there is no slotting).
  Entrance/exit lights and white "lines-o-lite" are implemented properly.

- `db5` is even closer -- the LS relays that lock switches are added, the
  "conditional crosslock" logic that pushes live control-lengths around,
  and the "CLK" ("click") logic that flashes switch and signal icons
  to say why a route can't be lined.  The result is extremely convincing
  and powerful, even though trains can't be run on it.

- `db6` is nearly complete -- signal slotting, auto-cancelling, and
  conditional crosslock/facing point overlap have been added to
  allow all that is needed for NXSYS trains to run safely -- real
  trains could not run safely because approach lock limits are not yet 
  implemented (awaits version 7).

- `db7` is the completely functional interlocking, with call-on's, 
  correct approach locking limits (including "Switch AS" overlap approach
  locking), and all the above.

The idea of this is to be a learning tool: to be able to see each
successive level of logic added and provide a model for implementing
interlockings incrementally in such a way that you get some "action"
(both for fun, reward, and confirmation of proper design) even a
short way along the path

Each of these levels uses NXSYS "relay macros" to define "kludge"
(temporary, inelegant exigency) versions of relays that must be there
in some form (such as PBS) to get the logic working, but all of whose
details will be left for a later level.

 	

~~~~
  1/19/98   8:44          12,538  db1.trk
  1/19/98   8:34          28,672  db2.trk
  1/19/98   8:28          37,546  db3.trk
  1/20/98  20:47          57,537  db4.trk
  1/21/98  17:11          71,421  db5.trk
  1/21/98  20:27          76,137  db6.trk
  1/21/98  20:29          80,654  db7.trk
  1/20/98  20:53           7,200  dburg.trk Layout file, identical for all
~~~~

This was once private, but now (2022) is public.  Enjoy. 

(Yes, I'm a big fan of the late Carl Barks's anatoid *oeuvre.* )