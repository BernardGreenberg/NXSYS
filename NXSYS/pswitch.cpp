#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef NXV2
#include "xtgtrack.h"
#else
#include "track.h"
#endif
#include "pswitch.h"
#include "objid.h"
#include "brushpen.h"
#include "compat32.h"
#include "rlyapi.h"

#include "nxsysapp.h"
#include "PolyKludge.h"

#define NUM_DT_OPTS DT_CENTER |DT_SINGLELINE |DT_NOCLIP

#ifndef Virtual
#define Virtual
#endif

#ifndef NXV2
#ifndef TLEDIT
#ifndef NXV1
#define NXV1
#endif
#endif
#endif

static HPEN Pen = NULL;

#define PSW_MID_WIDTH 12
#define PSW_TOP_WIDTH 8
#define PSW_HALF_HEIGHT 20

PanelSwitch::PanelSwitch (int xno, WP_cord p_wpx, WP_cord p_wpy, const char* relay_nomenclature)
{
    wp_x = p_wpx;
    wp_y = p_wpy;

#if TLEDIT
    RelayNomenclature = relay_nomenclature;
#else
    Rly = CreateQuislingRelay ((long)xno, relay_nomenclature);
#endif
    wp_limits.left = - PSW_MID_WIDTH-1;
    wp_limits.right =  PSW_MID_WIDTH+1;
    wp_limits.top =  -PSW_HALF_HEIGHT-2;
    wp_limits.bottom = + PSW_HALF_HEIGHT+2;
    State = FALSE;
    SetXlkgNo (xno);
}

Virtual void PanelSwitch::Display (HDC hdc) {
    int midwidth = (int)(PSW_MID_WIDTH * NXGO_Scale + .8);
    int topwidth = (int)(PSW_TOP_WIDTH * NXGO_Scale + .8);
    int height = (int)(PSW_HALF_HEIGHT * NXGO_Scale + .8);

    if (Pen == NULL)
	Pen = CreateSPen(1, RGB(192,192,192));
    POINT P[4];
    P[0].x  = sc_x -midwidth;
    P[0].y = P[3].y = sc_y;
    P[1].x = sc_x - topwidth;
    P[2].x = sc_x + topwidth;
    P[3].x = sc_x + midwidth;

    SelectObject (hdc, Pen);
    for (int s = 0; s < 2; s++) {
	if (s == 0)			/* off, lower */
	    P[1].y = P[2].y = sc_y + height; /* screen down is more ## */
	else
	    P[1].y = P[2].y = sc_y - height;
	BOOL fill = ((s !=0) == State);
	if (fill)
#ifdef TLEDIT
	    if (Selected)
		SelectObject(hdc, GKGreenBrush);
	    else
#endif
		SelectObject (hdc, GKOffBrush);
	else
	    SelectObject (hdc, GetStockObject (BLACK_BRUSH));
	Polygon (hdc, P, 4);
    }
    return;
}

Virtual int PanelSwitch::TypeID() {
    return ID_PANELSWITCH;
}

Virtual int PanelSwitch::ObjIDp(long id) {
    return XlkgNo == id;
}


void PanelSwitch::SetXlkgNo (int xno) {
    XlkgNo = xno;
    sprintf (NumStr, "%d", xno);
    NumStrLen = (int)strlen(NumStr);
}

#ifndef TLEDIT
Virtual void PanelSwitch::Hit (int mb) {

    BOOL UpperHalf = (NXGOHitY <= sc_y);
    if (UpperHalf)
	if (State)
	    State = FALSE;
	else;
    else
	if (State);
	else State = TRUE;
	
    ReportToRelay (Rly, State);
    Invalidate();
}
#endif


void InitPanelSwitchData () {
    

}
