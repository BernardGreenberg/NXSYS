#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef NXV2
#include "xtgtrack.h"
#else
#include "track.h"
#endif
#include "plight.h"
#include "objid.h"
#include "brushpen.h"
#include "compat32.h"
#include "rlyapi.h"

#include "nxsysapp.h"

#define NUM_DT_OPTS DT_CENTER |DT_SINGLELINE |DT_NOCLIP

#define MAX_RANDOM_BRUSHES 25

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

#ifndef TLEDIT
static struct {
    COLORREF color;
    HBRUSH Brush;
} RandomBrushes[MAX_RANDOM_BRUSHES];
static int NRandomBrushes = 0;

static struct {
    const char * Name;
    HBRUSH * ptr;

} Designators []
   = {
    {"r", &GKRedBrush},
    {"g", &GKGreenBrush},
    {"y", &GKYellowBrush},
    {"w", &GKWhiteBrush}
};

#define N_COLOR_DESIGNATORS (sizeof(Designators)/sizeof(Designators[0]))
#endif


#ifndef TLEDIT
void PanelLightAspect::Reporter (BOOL state, void * v) {
    ((PanelLightAspect *)v)->Report(state);
}

void PanelLightAspect::Report (BOOL state) {
    long mask = 1 << (this - &PLight->Aspects[0]);
    if (state)
	PLight->BitStates |= mask;
    else
	PLight->BitStates &= ~mask;
    PLight->Invalidate();
}
#endif

void PanelLight::SetRadius (int r) {
    Radius = r;
    wp_limits.left   = -Radius-1;
    wp_limits.right  = +Radius+1;
    wp_limits.top =    -Radius-1;
    wp_limits.bottom = +Radius+1;
}

PanelLight::PanelLight (int xno, int radius, WP_cord p_wpx, WP_cord p_wpy,
			   const char* ) :
   BitStates (0L),
   NAspects(0)
{
    wp_x = p_wpx;
    wp_y = p_wpy;
    SetRadius (radius);
    SetXlkgNo (xno);
}

Virtual void PanelLight::Display (HDC hdc) {
    int rad = (int)(Radius * NXGO_Scale + .8);
    long v = BitStates;
    SelectObject (hdc, GetStockObject (NULL_PEN));
    for (int index = 0; v; index++, v >>= 1) {
	if (v & 1) {
	    SelectObject (hdc, Aspects[index].Brush);
	    Ellipse(hdc, sc_x - rad, sc_y - rad, sc_x + rad, sc_y + rad);
	    return;
	}
    }
#ifdef TLEDIT
    if (Selected)
	SelectObject(hdc, GKGreenBrush);
    else
#endif
	SelectObject (hdc, GKOffBrush);
    Ellipse(hdc, sc_x - rad, sc_y - rad, sc_x + rad, sc_y + rad);
    return;
}


Virtual int PanelLight::TypeID() {
    return ID_PANELLIGHT;
}

Virtual int PanelLight::ObjIDp(long id) {
    return XlkgNo == id;
}


void PanelLight::SetXlkgNo (int xno) {
    XlkgNo = xno;
    sprintf (NumStr, "%d", xno);
    NumStrLen =(int) strlen(NumStr);
}

BOOL PanelLight::AddAspectI(COLORREF color, const char * designator, const char * name) {
    if (NAspects >= MAX_PL_ASPECTS) {
	/* should complain, not tledit */	
	return FALSE;
    }
    PanelLightAspect * ap = &Aspects[NAspects];
    ap->PLight = this;
#ifdef TLEDIT
    ap->Color = color;
    ap->Name = _strdup(name);
    ap->Colorstring = designator? _strdup(designator) : NULL;
    ap->Brush = NULL;
#else
    if (designator) {
	for (int i = 0; i < N_COLOR_DESIGNATORS; i++)
	    if (!_stricmp (designator, Designators[i].Name)) {
		ap->Brush = *Designators[i].ptr;
		break;
	    }
	    else if (i == N_COLOR_DESIGNATORS-1)
		ap->Brush = GKOffBrush;
    }
    else {
	for (int i = 0; i < NRandomBrushes; i++)
	    if (color == RandomBrushes[i].color) {
		ap->Brush = RandomBrushes[i].Brush;
		break;
	    }
	    else
             if (i == NRandomBrushes-1) {
		    if (NRandomBrushes >= MAX_RANDOM_BRUSHES)
			return FALSE;
		    else {
			RandomBrushes[NRandomBrushes].color = color;
			RandomBrushes[NRandomBrushes++].Brush = CreateSBrush(color);
		    }
             }
    }
    CreateAndSetReporter (XlkgNo, name, PanelLightAspect::Reporter, ap);
#endif
    NAspects++;
    return TRUE;
}

BOOL PanelLight::AddAspect (COLORREF color, const char *name) {
    return AddAspectI (color, NULL, name);
}
BOOL PanelLight::AddAspect (const char* designator, const char * name) {
    return AddAspectI ((COLORREF) 0, designator, name);
}

void InitPanelLightData () {

}
