#include "windows.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "compat32.h"
#include "nxgo.h"
#include "xtgtrack.h"
#include "typeid.h"
#include "brushpen.h"
#include "rlyapi.h"
#include "nxsysapp.h"

#define mIn(a,b) (((a) < (b)) ? (a) : (b))
#define mAx(a,b) (((a) > (b)) ? (a) : (b))

ExitLight::ExitLight (TrackSeg * seg, TSEX end_index, int xno) {

    XlkgNo = xno;
    Seg = seg;
    EndIndex = end_index;
    seg->GetEnd(end_index).ExLight = this;
    Lit = RedFlash = Blacking = FALSE;
    XPB = NULL;
    Reposition();
}


void ExitLight::Reposition () {
    WP_cord wpx1 = Seg->Ends[0].wpx;
    WP_cord wpy1 = Seg->Ends[0].wpy;
    WP_cord wpx2 = Seg->Ends[1].wpx;
    WP_cord wpy2 = Seg->Ends[1].wpy;
    int delx = (int)(wpx2-wpx1);
    int dely = (int)(wpy2-wpy1);
    double len = sqrt (delx*delx + dely*dely); /* yay Pythagoras! */
    double ulen = 2.0*Track_Seg_Len;
    if (ulen < len) {
	if (EndIndex == TSEX::E0) {
	    wpx2 = (int) (wpx1 + ulen*Seg->CosTheta);
	    wpy2 = (int) (wpy1 + ulen*Seg->SinTheta);
	}
	else {
	    wpx1 = (int) (wpx2 - ulen*Seg->CosTheta);
	    wpy1 = (int) (wpy2 - ulen*Seg->SinTheta);
	}
    }
    int del = 5;
    wp_x = mIn(wpx1, wpx2) - del;
    wp_y = mIn(wpy1, wpy2) - del;
    wp_limits.top = wp_limits.left = 0;
    wp_limits.right  = (int)(mAx (wpx1, wpx2) - wp_x + (long)del);
    wp_limits.bottom = (int)(mAx (wpy1, wpy2) - wp_y + (long)del);
    ComputeVisibleLast();
}



Virtual void ExitLight::Display (HDC hDC) {
    if (!Lit && !Selected && !RedFlash && !Blacking)
	return;
    if (hDC == NULL)
        DebugBreak();
    
    int scx1, scy1, scx2, scy2;
    Seg->GetGraphicsCoords(0, scx1, scy1);
    Seg->GetGraphicsCoords(1, scx2, scy2);
    int delx = scx2-scx1;
    int dely = scy2-scy1;
    double len = sqrt (delx*delx + dely*dely); /* yay Pythagoras! */
    double ulen = 2*Track_Seg_Len*NXGO_Scale;
    if (ulen < len) {
	if (EndIndex == TSEX::E0) {
	    scx2 = (int) (scx1 + ulen*Seg->CosTheta);
	    scy2 = (int) (scy1 + ulen*Seg->SinTheta);
	}
	else {
	    scx1 = (int) (scx2 - ulen*Seg->CosTheta);
	    scy1 = (int) (scy2 - ulen*Seg->SinTheta);
	}
    }
    HPEN pen;
#ifdef TLEDIT
    if (Selected)
	pen = SelectedExitLightPen;
#else
    if (RedFlash)
	pen = RedExitLightPen;
#endif
    else if (Blacking)
	pen = BlackExitLightPen;
    else
	pen = ExitLightPen;
    SelectObject (hDC, pen);
    MoveTo (hDC, scx1, scy1);
    LineTo (hDC, scx2, scy2);
}



#ifdef REALLY_NXSYS
void ExitLight::DrawBlack () {
    if (Visible) {
#ifndef NXSYSMac
	HDC hDC = GetDC (G_mainwindow);
	Blacking = TRUE;
	Display(hDC);
	Blacking = FALSE;
	ReleaseDC(G_mainwindow, hDC);
#endif
    }
}

void ExitLight::Off () {
    RedFlash = Lit = FALSE;
    DrawBlack();
    Seg->Invalidate();
}

void ExitLight::SetRedFlash (BOOL onoff) {
    if (RedFlash != onoff) {
	RedFlash = onoff;
	Invalidate();
	if (!onoff)
	    DrawBlack();
    }
}
#endif

void ExitLight::SetLit (BOOL onoff) {
    if (Lit != onoff) {
	Lit = onoff;
	Invalidate();
#ifdef REALLY_NXSYS
	if (!onoff)
	    DrawBlack();
#endif
    }
}

#ifdef REALLY_NXSYS
Virtual void ExitLight::Hit (int mb) {
    if (XPB != NULL) {			/* allow debug without XS */
	ReportToRelay (XPB, TRUE);
	ReportToRelay (XPB, FALSE);
    }
}
#endif

Virtual BOOL ExitLight::HitP (long x, long y) {
    return (Lit || Selected) && GraphicObject::HitP (x, y);
}

TypeId ExitLight::TypeID (){
    return TypeId::EXITLIGHT;
}

bool ExitLight::IsNomenclature(long id) {
    return id == XlkgNo;
}

void ExitLight::KReporter(BOOL state, void* v) {
    ((ExitLight *) v)->SetLit (state);
}

ExitLight::~ExitLight () {
    if (!NXGODeleteAll) {
	Seg->Invalidate();
	Seg->GetEnd(EndIndex).ExLight = NULL;
    }
}
