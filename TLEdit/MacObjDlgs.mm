//
//  Created by Bernard Greenberg on 9/24/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.

#import "GenericWindlgController.h"
#include "resource.h"

/* This handles all dialog objects which don't need their own classes.  They need their
   own classes if and only if they have buttons that actually act in Mac code, not
   Windows code forwarded through TLWindowsSide,or simply being read by the IDOK handler.
   Right now, that's only TrackSeg with his view-circuit timer and the hirsute text string. */

/* The format here is King of Simplicity -- Resource ID followed by a string to be found
   in the text of the control, unless it has a slash in it, then the first part is the
   name of a box containing the control matching the second part, to disambiguate boxes
   with identically-texted controls.  Whether this actually designates the control
   that matches, or the NEXT control in keySequence is complex -- see the method jtbp.
   This is used to exploit labels, visible and hidden, as "labels" for text input ctls.

   If not linked in that way, the controls can be labels or buttons or checkboxes or just
   about any damned thing, although the "assert"s in GenericWindlgController need be told.
 */

//static struct TextKey jointDefs[] = {
static DefVector jointDefs = {
    {IDC_JOINT_STATION_ID, "Station"},
    {IDC_JOINT_WPX, "X pos"},
    {IDC_JOINT_WPY, "Y pos"},

    {IDC_JOINT_INSULATED, "nsulated"},
};

REGISTER_DIALOG_4(d1,IDD_JOINT,@"JointProperties",jointDefs)

static DefVector TLDefs = {
    {IDC_TRAFFICLEVER_LEVER, "Lever"},
    {IDC_TRAFFICLEVER_WPX,"X pos"},
    {IDC_TRAFFICLEVER_WPY,"Y pos"},

    {IDC_TRAFFICLEVER_LEFT, "Left"},
    {IDC_TRAFFICLEVER_RIGHT, "Right"},
};

REGISTER_DIALOG_4(d2,IDD_TRAFFICLEVER, @"TrafficLeverProperties", TLDefs)

static DefVector swkeyInputs = {
    {IDC_SWKEY_LEVER, "lever"},
    {IDC_SWKEY_WPX, "X position"},
    {IDC_SWKEY_WPY, "Y position"},
};

REGISTER_DIALOG_4(d3,IDD_SWKEY, @"SwitchKeyProperties", swkeyInputs)

static DefVector PSinputs = {
    {IDC_PANELSWITCH_LEVER, "nterlock"},
    {IDC_PANELSWITCH_NOMENCLATURE, "omenclature"},
    {IDC_PANELSWITCH_STRING, "Key"},
    
    {IDC_PANELSWITCH_WPX, "X po"},
    {IDC_PANELSWITCH_WPY, "Y po"},
};

REGISTER_DIALOG_4(d4,IDD_PANELSWITCH, @"PanelSwitchProperties", PSinputs)

static DefVector PLinputs = {
    {IDC_PANELLIGHT_LEVER, "nterlock"},
    {IDC_PANELLIGHT_GREEN, "Green"},
    {IDC_PANELLIGHT_YELLOW, "Yellow"},
    {IDC_PANELLIGHT_RED, "Red"},
    {IDC_PANELLIGHT_WHITE, "White"},
    
    {IDC_PANELLIGHT_WPX, "X po"},
    {IDC_PANELLIGHT_WPY, "Y po"},
    
    {IDC_PANELLIGHT_RADIUS, "Radius"},
    {IDC_PANELLIGHT_STRING, "Label"},
};

REGISTER_DIALOG_4(d5,IDD_PANELLIGHT, @"PanelLightProperties", PLinputs)

static DefVector XLDefs = {
    {IDC_EDIT_XLIGHT_XLNO, "lever"},

    {IDC_EDIT_XLIGHT_IJNO, "IJ"},
    {IDC_EDIT_XLIGHT_ORIENT, "Orient"},
};

REGISTER_DIALOG_4(d6,IDD_EDIT_EXLIGHT, @"ExitLightProperties", XLDefs)

static DefVector SWdefs = {
    {IDC_SWITCH_EDIT, "Switch Number"},

    {IDC_SWITCH_WARN, "Error:"},

    {IDC_SWITCH_IS_A, "A points"},
    {IDC_SWITCH_IS_B, "B points"},
    {IDC_SWITCH_IS_SINGLETON, "Singleton"},
    /* These buttons have been routed at IB time to GenericWindlgController::activeButton;
     their presence here is used by that method to route them to the Windows DLGPROC when
     that method is invoked (presumably by them). */
    {IDC_SWITCH_HEURISTICATE, "Heur"},
    {IDC_SWITCH_SWAP_NORMAL, "Swap"},
    {IDC_SWITCH_HILITE_NORMAL, "High"},
    /* Just plain "edit" not good enough!  Text boxes have that by default! -- 1 day lost 3/23/2022 */
    {IDC_SWITCH_EDIT_JOINT_ATTRIBUTES, "Edit Joint"},
};

REGISTER_DIALOG_4(d7,IDD_SWITCH_ATTR, @"SwitchProperties", SWdefs)

static DefVector Sigdefs = {
    {IDC_EDIT_SIG_LEVER, "Lever"},
    {IDC_EDIT_SIG_TRACK_ID, "Track"},
    {IDC_EDIT_SIG_STATION_NO, "Station"},
    {IDC_EDIT_SIG_HEADS, "Heads"},

    {IDC_EDIT_SIG_ORIENTATION, "Orientation"},
    {IDC_EDIT_SIG_IJID, "Identification"},

    {IDC_EDIT_SIG_STOP, "top"},
    {IDC_EDIT_SIGNAL_JOINT, "Edi"},
};

REGISTER_DIALOG_4(d8,IDD_EDIT_SIGNAL, @"SignalProperties",Sigdefs)
