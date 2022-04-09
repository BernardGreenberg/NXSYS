//
//  Created by Bernard Greenberg on 9/24/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//  Totalmente revidiert 3-9 aprilis anno iii della peste.

#import "GenericWindlgController.h"
#include "resource.h"

/* See GenericWindlgController.h for how that unique variable name is generated!
   Initializing top-level static variables is the only way to get load-time
   executed code in C++. All that is left for some formerly extraordinarily
   complicated closure-generating macros. */

#define DEFDLG(...) \
static int UNIQUE_VARIABLE = DefineWindlgGeneric(__VA_ARGS__);

#define R(x)  {#x, x},

/*
 This handles all object dialogs that don't need their own classes.  They need their
 own classes if and only if they have buttons that actually act in Mac code, not just
 Windows code forwarded through TLWindowsSide (the latter including OK/Cancel).
 Right now, the hard cases is only TrackSeg with his view-circuit timer and the hirsute
 text string dialog, which have their own source files.

 This declares all the controls known to the Windows code by the ID's named, that is, all
 the controls that have to be mapped from Mac IB View artifacts into apparent Windows controls.
 
 This replaces an ugly and error-prone system based upon matching text strings and labels.  The
 watershed event that prompted this change is the introduction of text "identifier" fields in
 Cocoa controls, later than the 2014 initial conversion.  The "identifier" fields must be the
 exact strings below, i.e., the Windows resource symbols.  All controls without such ID's are left
 alone.

 These dialog-specific lists are not strictly necessary -- the symbol values could be retrieved
 from a global easily built from the Windows resource file, but these local lists present the
 advantage that the dialog builder can run through them and ensure that all the referenced
 resource ID's have, in fact been found in controls, and diagnose absences and duplication.
 */

DEFDLG(IDD_JOINT, @"JointProperties", RIDVector{
    R(IDC_JOINT_STATION_ID)
    R(IDC_JOINT_WPX)
    R(IDC_JOINT_WPY)
    R(IDC_JOINT_INSULATED)
})

DEFDLG(IDD_TRAFFICLEVER, @"TrafficLeverProperties", RIDVector{
    R(IDC_TRAFFICLEVER_LEVER)
    R(IDC_TRAFFICLEVER_WPX)
    R(IDC_TRAFFICLEVER_WPY)
    R(IDC_TRAFFICLEVER_LEFT)
    R(IDC_TRAFFICLEVER_RIGHT)
})

DEFDLG(IDD_SWKEY, @"SwitchKeyProperties", RIDVector{
    R(IDC_SWKEY_LEVER)
    R(IDC_SWKEY_WPX)
    R(IDC_SWKEY_WPY)
})

DEFDLG(IDD_PANELSWITCH, @"PanelSwitchProperties", RIDVector{
    R(IDC_PANELSWITCH_LEVER)
    R(IDC_PANELSWITCH_NOMENCLATURE)
    R(IDC_PANELSWITCH_STRING)
    R(IDC_PANELSWITCH_WPX)
    R(IDC_PANELSWITCH_WPY)
})

DEFDLG(IDD_PANELLIGHT, @"PanelLightProperties", RIDVector{
    R(IDC_PANELLIGHT_LEVER)
    R(IDC_PANELLIGHT_GREEN)
    R(IDC_PANELLIGHT_YELLOW)
    R(IDC_PANELLIGHT_RED)
    R(IDC_PANELLIGHT_WHITE)
    
    R(IDC_PANELLIGHT_WPX)
    R(IDC_PANELLIGHT_WPY)
    
    R(IDC_PANELLIGHT_RADIUS)
    R(IDC_PANELLIGHT_STRING)
})

DEFDLG(IDD_EDIT_EXLIGHT, @"ExitLightProperties", RIDVector{
    R(IDC_EDIT_XLIGHT_XLNO)
    R(IDC_EDIT_XLIGHT_IJNO)
    R(IDC_EDIT_XLIGHT_ORIENT)
})

DEFDLG(IDD_SWITCH_ATTR, @"SwitchProperties", RIDVector{
    R(IDC_SWITCH_EDIT)  /* poorly-named lever number */

    R(IDC_SWITCH_WARN)

    R(IDC_SWITCH_IS_A)
    R(IDC_SWITCH_IS_B)
    R(IDC_SWITCH_IS_SINGLETON)
    /* Active buttons not requiring custom Mac code. */
    R(IDC_SWITCH_HEURISTICATE)
    R(IDC_SWITCH_SWAP_NORMAL)
    R(IDC_SWITCH_HILITE_NORMAL)

    R(IDC_SWITCH_EDIT_JOINT_ATTRIBUTES)
})

DEFDLG(IDD_EDIT_SIGNAL, @"SignalProperties", RIDVector{
    R(IDC_EDIT_SIG_LEVER)
    R(IDC_EDIT_SIG_TRACK_ID)
    R(IDC_EDIT_SIG_STATION_NO)
    R(IDC_EDIT_SIG_HEADS)

    R(IDC_EDIT_SIG_ORIENTATION)
    R(IDC_EDIT_SIG_IJID)

    R(IDC_EDIT_SIG_STOP)
    R(IDC_EDIT_SIGNAL_JOINT)
})
