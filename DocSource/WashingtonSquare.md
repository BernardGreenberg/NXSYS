# Washington Square problem

##North of 14th St, IND stationing numbers acquire 4 digits, and cannot be represented

##Proposed Solution

Insert new sub-form `(Version (2 7 0 1013) :GENSYM-BASE 1000000)` in `LAYOUT`. Will fail older NXSYS.

Better diagnose attempts to use ID's at this level. 10^5  is conceivable, 10-1582 (track 10, station 1582). 

Diagnose ID's 10000 ≥ x, 1000000 < x:   
  `Can't use ID numbers in this range.  Please click 'Help' and read "Stationing number range"`
  
'Irreversible act': upgrade station numbers (actually 'upgrade temporary numbers', turns on a switch automatically enabled by appropriate Version form.

Cross-platform get\_application\_version => vector<int>.

Clear partition and different calls for assign/relinquish lever number (signals, switches, and traffic) (how relinquish 15A?) vs gensyms. Use std::unordered\_set for former, (ordered) std::set (or map to GOptr?) for latter, manage reuse and assignment/withdrawal from roving ptr.

Fix IJ-number decoders (e.g., signal plate) to use GensymBase or something like it

Copy version resource from NXSYS; use it in About dialog. Code in NXSYSWindows/dialogs.cpp.  Make new common WinNXSYS/WinTLEdit module to return some cross-platform STL-based info structure: App name, build date as CTime, version vector (including mac build).  Use it in about dialogs and title bar.

Idea: exploit 0 ID's; assign ID's associatively at save time.  Don't assign for IJ's.  Not needed; only switches or track-ends need ID's with current dumper algorithm.