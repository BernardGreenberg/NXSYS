//
//  Created by Bernard Greenberg on 9/24/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//  Totalmente revidiert 3 aprilis anno iii della peste.

#import "GenericWindlgController.h"
#include "resource.h"
#include <unordered_map>
#include <map>

#define DEFINE_WINDLG_GENERIC(dummy_name, ...) \
static int dummy_name = DefineWindlgGeneric(__VA_ARGS__);

#define R(x)  {#x, x},

/* Global maps aren't used right now, but they conceivably might be.  The local
 maps are not strictly necessary -- the symbol values could be retrieved from a global
 map, but the local maps present the advantage that the dialog builder can run through the
 list and ensure that all the referenced resource ID's have, in fact been found in controls,
 and diagnose an absence.

 A global table could easily be automatically generated from the windows resource include
 file, though ...
 
 */

std::map<NSString*, int, CompareNSString>WindowsRIDNameToValue;
std::unordered_map<int, const char *>WindowsRIDValueToName;


/* This handles all dialog objects which don't need their own classes.  They need their
   own classes if and only if they have buttons that actually act in Mac code, not
   Windows code forwarded through TLWindowsSide,or simply being read by the IDOK handler.
   Right now, that's only TrackSeg with his view-circuit timer and the hirsute text string. */

/* This declares all the controls known to the Windows code by the ID's named, that is, all
 the controls that have to be mapped from Mac IB View artifacts into apparent Windows controls.
 
 This replaces an ugly and error-prone system based upon matching text strings and labels.  The
 watershed event that prompted this change is the introduction of text "identifier" fields in
 Cocoa controls, later than the 2014 initial conversion.  The "identifier" fields must be the
 exact strings below, i.e., the Windows resource symbols.  All controls without such ID's are left
 alone.

 */

DEFINE_WINDLG_GENERIC(d1, IDD_JOINT, @"JointProperties", RIDVector{
    R(IDC_JOINT_STATION_ID)
    R(IDC_JOINT_WPX)
    R(IDC_JOINT_WPY)
    R(IDC_JOINT_INSULATED)
})

DEFINE_WINDLG_GENERIC(d2, IDD_TRAFFICLEVER, @"TrafficLeverProperties", RIDVector{
    R(IDC_TRAFFICLEVER_LEVER)
    R(IDC_TRAFFICLEVER_WPX)
    R(IDC_TRAFFICLEVER_WPY)
    R(IDC_TRAFFICLEVER_LEFT)
    R(IDC_TRAFFICLEVER_RIGHT)
})

DEFINE_WINDLG_GENERIC(d3, IDD_SWKEY, @"SwitchKeyProperties", RIDVector{
    R(IDC_SWKEY_LEVER)
    R(IDC_SWKEY_WPX)
    R(IDC_SWKEY_WPY)
})

DEFINE_WINDLG_GENERIC(d4, IDD_PANELSWITCH, @"PanelSwitchProperties", RIDVector{
    R(IDC_PANELSWITCH_LEVER)
    R(IDC_PANELSWITCH_NOMENCLATURE)
    R(IDC_PANELSWITCH_STRING)
    R(IDC_PANELSWITCH_WPX)
    R(IDC_PANELSWITCH_WPY)
})

DEFINE_WINDLG_GENERIC(d5, IDD_PANELLIGHT, @"PanelLightProperties", RIDVector{
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

DEFINE_WINDLG_GENERIC(d6, IDD_EDIT_EXLIGHT, @"ExitLightProperties", RIDVector{
    R(IDC_EDIT_XLIGHT_XLNO)
    R(IDC_EDIT_XLIGHT_IJNO)
    R(IDC_EDIT_XLIGHT_ORIENT)
})

DEFINE_WINDLG_GENERIC(d7, IDD_SWITCH_ATTR, @"SwitchProperties", RIDVector{
    R(IDC_SWITCH_EDIT)  /* poorly-named lever number */

    R(IDC_SWITCH_WARN)

    R(IDC_SWITCH_IS_A)
    R(IDC_SWITCH_IS_B)
    R(IDC_SWITCH_IS_SINGLETON)
    /* These buttons have been routed at IB time to GenericWindlgController::activeButton;
     their presence here is used by that method to route them to the Windows DLGPROC when
     that method is invoked (presumably by them). */
    R(IDC_SWITCH_HEURISTICATE)
    R(IDC_SWITCH_SWAP_NORMAL)
    R(IDC_SWITCH_HILITE_NORMAL)

    R(IDC_SWITCH_EDIT_JOINT_ATTRIBUTES)
})

DEFINE_WINDLG_GENERIC(d8, IDD_EDIT_SIGNAL, @"SignalProperties", RIDVector{
    R(IDC_EDIT_SIG_LEVER)
    R(IDC_EDIT_SIG_TRACK_ID)
    R(IDC_EDIT_SIG_STATION_NO)
    R(IDC_EDIT_SIG_HEADS)

    R(IDC_EDIT_SIG_ORIENTATION)
    R(IDC_EDIT_SIG_IJID)

    R(IDC_EDIT_SIG_STOP)
    R(IDC_EDIT_SIGNAL_JOINT)
})
