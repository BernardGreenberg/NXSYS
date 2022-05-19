# Undo TBDs

Mod status in titlebar (*)
  
Change wildfire to accumulate list first, iterate it second.  
Dialog to display count.

Cut segment should stash and restore circuit.  
Strategy for cutting tagged end-joint  
  Current theory is (SEG)(JOINT)(JOINT) in recreate_form.

Joint merge (very difficult)  
- Prohibit when either is insulated or movee has station #.  
- Disallow setting station # on uninsulated joints.
- Prohibit circuit-setting when end is not insulated.  
- Write document on track circuit management

Undo stack before save (document how to use).  Save-point size() stack.
- may need tick-counter.

Windows implementation  
- Menu management function 
- Add undo doc to ...something.
