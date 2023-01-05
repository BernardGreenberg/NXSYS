#include "windows.h"
#include <string.h>
#include <stdio.h>
#include "stdlib.h"
#include "compat32.h"
#include "nxgo.h"
#include "xtgtrack.h"
#include "typeid.h"
#include "brushpen.h"
#include "math.h"
#include "PolyKludge.h"

#include "signal.h"


#define FLEETING_TRIANGLE_BASE_FRAC .75
#define FLEETING_TRIANGLE_HEIGHT_FRAC .75

#define mIn(a,b) (((a) < (b)) ? (a) : (b))
#define mAx(a,b) (((a) > (b)) ? (a) : (b))

PanelSignal::PanelSignal
   (TrackSeg * ts, TSEX end_index, Signal * s, char * text) {
    Seg = ts;
    Sig = s;
    s->PSignal = this;
    EndIndex = end_index;
    Label = NULL;
    Radius = 8;
    TRelX = 20;
    TRelY = 15;

    Reposition();
    SetXlkgNo (s->XlkgNo, FALSE);
    MakeSelfVisible();
    if (s && s->TStop)
	s->TStop->Reposition();
}

PanelSignal::~PanelSignal() {
    delete Sig;
    if (!NXGODeleteAll)
	delete Label;
}

#ifndef TLEDIT
void PanelSignal::EditContextMenu (HMENU m) {
    if (Sig)
	Sig->EditContextMenu(m);
}
#endif

void PanelSignal::GetAngles (float &costheta, float&sintheta) {
    
    //TrackSegEnd * ep = &Seg->Ends[EndIndex];
    if (EndIndex == TSEX::E0) {
	costheta = Seg->CosTheta;
	sintheta = Seg->SinTheta;
    }
    else {
	costheta = -Seg->CosTheta;
	sintheta = -Seg->SinTheta;
    }
}


void PanelSignal:: Reposition() {
    float costheta, sintheta;
    GetAngles(costheta, sintheta);
    TrackSegEnd&E = Seg->GetEnd(EndIndex);
    double fxc = E.wpx, fyc = E.wpy;
    fxc += costheta * TRelX;
    fyc += sintheta * TRelX;		/* point on tk corresp to sig head */
    float downcostheta = -sintheta;
    float downsintheta = costheta;
    fxc += downcostheta*TRelY;
    fyc += downsintheta*TRelY;		/* signal head */
    MoveWP ((int)fxc, (int)fyc);
    PositionLabel(FALSE);
    if (Sig && Sig->TStop)
	Sig->TStop->Reposition();
}

TypeId PanelSignal::TypeID() {
    return TypeId::SIGNAL;
}

bool PanelSignal::IsNomenclature (long id) {
/* +++++ gonna need a lot of work with symbolic object names */
    return (Sig
	    &&
	    ((Sig->XlkgNo == id)
	     || (Sig->StationNo && (Sig->StationNo == id))));
	    
}


void PanelSignal::GetDispPoints (WP_cord &x1, WP_cord &y1,
				 WP_cord &x2, WP_cord &y2,
				 WP_cord &x3, WP_cord &y3) {
    float costheta, sintheta;
    GetAngles(costheta, sintheta);
    double fxc = wp_x;
    double fyc = wp_y;

    int bu = (int)(.75*Radius);

    x2 = (int)(fxc -costheta * (Radius+1.5*bu));
    y2 = (int)(fyc -sintheta * (Radius+1.5*bu));
    x1 = (int) (x2 + bu*sintheta);
    y1 = (int) (y2 - bu*costheta);
    x3 = (int) (x2 - bu*sintheta);
    y3 = (int) (y2 + bu*costheta);
}
extern bool REDISPLAYING;
void PanelSignal::Display (HDC dc) {

    RECT r;
    //int bu = (int)(.75*Radius);
    int ra = (int)(Radius*NXGO_Scale);
    r.left = sc_x - ra;
    r.right = sc_x + ra;
    r.top = sc_y - ra;
    r.bottom = sc_y + ra;

#ifndef TLEDIT
    if (!Sig->Fleeted)
	DrawFleeting(dc, FALSE);
#endif
    SelectObject (dc, GetStockObject(NULL_PEN));
    SelectObject (dc, Sig->GetGKBrush());
#ifdef NXSYSMac
    if (!REDISPLAYING)
        DebugBreak();
#endif
    
    Ellipse (dc, r.left, r.top, r.right, r.bottom);

    float costheta, sintheta;
    GetAngles(costheta, sintheta);
    double fxc = wp_x;
    double fyc = wp_y;
    WP_cord x0, y0, x1, y1, x2, y2, x3, y3;
    GetDispPoints (x1, y1, x2, y2, x3, y3);
    x0 = (int)(fxc -costheta * Radius);
    y0 = (int)(fyc -sintheta * Radius);
    SelectObject (dc, SigPen);
    MoveTo (dc, WPXtoSC(x0), WPYtoSC(y0));
    LineTo (dc, WPXtoSC(x2), WPYtoSC(y2));
    MoveTo (dc, WPXtoSC(x1), WPYtoSC(y1));
    LineTo (dc, WPXtoSC(x3), WPYtoSC(y3));
#ifndef TLEDIT
    if (Sig->Fleeted)
	DrawFleeting(dc, TRUE);
#endif
}

#ifndef TLEDIT
void PanelSignal::DrawFleeting (HDC dc, BOOL enabled) {
    //int bu = (int)(.75*Radius);
    float costheta, sintheta;
    GetAngles(costheta, sintheta);
    POINT polly[3];
    double fx0 = wp_x - costheta * Radius;
    double fy0 = wp_y - sintheta * Radius;
    polly[0].x = WPXtoSC((int)fx0);
    polly[0].y = WPYtoSC((int)fy0);
    double dx4 = fx0 - costheta * Radius*FLEETING_TRIANGLE_HEIGHT_FRAC;
    double dy4 = fy0 - sintheta * Radius*FLEETING_TRIANGLE_HEIGHT_FRAC;
    double fb = Radius*FLEETING_TRIANGLE_BASE_FRAC/2.0;
    polly[1].x = WPXtoSC ((int)(dx4 +  sintheta*fb));
    polly[1].y = WPYtoSC ((int)(dy4 -  costheta*fb));
    polly[2].x = WPXtoSC ((int)(dx4 -  sintheta*fb));
    polly[2].y = WPYtoSC ((int)(dy4 +  costheta*fb));
    SelectObject (dc, GetStockObject (NULL_PEN));
    SelectObject (dc, enabled? GKGreenBrush : GetStockObject(BLACK_BRUSH));
    Polygon (dc, polly, 3);
}

#endif
Virtual void PanelSignal::ComputeWPRect () {
    wp_limits.right = wp_limits.left = wp_limits.top = wp_limits.bottom = 0;
    LimWPCoord (wp_x - Radius, wp_y - Radius);
    LimWPCoord (wp_x - Radius, wp_y + Radius);
    LimWPCoord (wp_x + Radius, wp_y + Radius);
    LimWPCoord (wp_x + Radius, wp_y - Radius);
    WP_cord x1, y1, x2, y2, x3, y3;
    GetDispPoints (x1, y1, x2, y2, x3, y3);
    LimWPCoord (x1, y1);
    LimWPCoord (x2, y2);
    LimWPCoord (x3, y3);
}

void PanelSignal::LimWPCoord (WP_cord xw, WP_cord yw) {
    long x = xw - wp_x;
    long y = yw - wp_y;
    if (x - 1 < wp_limits.left)
	wp_limits.left = (int)(x-1);
    if (x + 1 > wp_limits.right)
	wp_limits.right = (int)(x+1);
    if (y - 1 < wp_limits.top)
	wp_limits.top = (int)(y-1);
    if (y + 1 > wp_limits.bottom)
	wp_limits.bottom = (int)(y+1);
}

void PanelSignal::SetXlkgNo (int xlkgno, BOOL compvisible){
    if (Sig)
	Sig->XlkgNo = xlkgno;
    if (xlkgno == 0) {
	delete Label;
	Label = NULL;
    }
    else {
        std::string lab;
        if (xlkgno >= 7000) {
            lab += 'A' + ((xlkgno/1000) - 7);
            lab += std::to_string(xlkgno % 1000);
        }
	else
            lab = std::to_string(xlkgno);
	if (Label == NULL)
	    Label = new NXGOLabel (this, wp_x, wp_y, lab.c_str());
	else
            Label->SetText (lab.c_str());
	PositionLabel(compvisible);
    }
}

void PanelSignal::PositionLabel (BOOL compvisible) {

    if (Label == NULL)
	return;

    Label->Invalidate();
    float costheta, sintheta;
    GetAngles(costheta, sintheta);
    double fullrad = Label->Radius()+ Radius + 1.5*((int)(.75*Radius));
    Label->PositionCenter ((int)(wp_x - costheta * fullrad),
			   (int)(wp_y - sintheta * fullrad));
    if (compvisible) {
	//BOOL wasv = Label->Visible;
	Label->ComputeVisibleLast();
	if (Label->Visible)
	    ComputeVisibleObjectsLast();
    }
    else 
	Label->ComputeVisibleLast();
    Label->Invalidate();
}

#ifndef TLEDIT
Virtual void PanelSignal::Hit (int mb) {
    if (Sig)
	Sig->Hit(mb);
}

void PanelSignal::ProcessLoadComplete() {
    Sig->ProcessLoadComplete();
}

Virtual void PanelSignal::UnHit () {
    if (Sig)
	Sig->UnHit();
}
#endif
