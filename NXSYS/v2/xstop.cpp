#include "windows.h"
#include <string.h>
#include <stdio.h>
#include "stdlib.h"
#include "compat32.h"
#include "nxgo.h"
#include "xtgtrack.h"
#include "objid.h"
#include "brushpen.h"
#include "math.h"
#include "ssdlg.h"
#include "PolyKludge.h"



#include "signal.h"


enum StopShowState {STOPSTATE_DARK, STOPSTATE_TRIPPING, STOPSTATE_CLEAR};

#ifdef TLEDIT
int ShowStopPolicy = (int) STOPSTATE_TRIPPING;
BOOL StopsChanging = FALSE;
#else
extern int ShowStopPolicy;
extern int StopsChanging;
#endif   


Stop::Stop (Signal * s) {
    Tripping = 1;
    s->TStop = this;
    coding = 0;
    Sig = s;
    Reposition();
    MakeSelfVisible();
}

/*   virtual */ void Stop::Display (HDC dc) {
     if (ShowStopPolicy == SHOW_STOPS_NEVER && !StopsChanging)
	return;

    HBRUSH brush = Tripping ? GKRedBrush : GKYellowBrush;

    if (ShowStopPolicy == SHOW_STOPS_NEVER)
	brush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    else if (coding)
	if (code_clicks & 1)
	    brush = (HBRUSH)GetStockObject(BLACK_BRUSH);
	else;
    else
	if (ShowStopPolicy == SHOW_STOPS_RED && !Tripping)
	    brush = (HBRUSH)GetStockObject(BLACK_BRUSH);

    SelectObject (dc, NullPen);
    SelectObject (dc, brush);

    POINT point[3]{};

    point[0].x = WPXtoSC (x1);
    point[1].x = WPXtoSC (x2);
    point[2].x = WPXtoSC (x3);

    point[0].y = WPYtoSC (y1);
    point[1].y = WPYtoSC (y2);
    point[2].y = WPYtoSC (y3);

    Polygon (dc, point, 3);
}


/*   virtual */ ObjId Stop::TypeID() {
    return ObjId::STOP;
}

/*   virtual */ int Stop::IsNomenclature(long) {
    return 0;
}

/* stops should not get hit, or they make parts of exit lights invisible */

BOOL Stop::HitP (long, long) {
    return FALSE;
}

void Stop::Reposition () {

    float side = 10.0f;
    float disp = 6.0f;
    float alt = side*0.866f;
    float coff = alt/3;
    float rad = 2*coff;

    PanelSignal * ps = Sig->PSignal;
    if (ps ==NULL)
	return;
    float costheta, sintheta;
    ps->GetAngles (costheta, sintheta);
    TrackSegEnd * ep = &ps->Seg->GetEnd(ps->EndIndex);
    
    float fx = ep->wpx + disp*costheta;
    float fy = ep->wpy + disp*sintheta;

    fx  -= Track_Normal_Halfwidth*sintheta;
    fy  += Track_Normal_Halfwidth*costheta;

    x1 = (int) fx;
    y1 = (int) fy;
    
    float fx2 = fx + side*costheta;
    float fy2 = fy + side*sintheta;
    x2 = (int) fx2;
    y2 = (int) fy2;
    
    float fx3 = (fx + fx2)/2;
    float fy3 = (fy + fy2)/2;

    x3 = (int) (fx3 - alt*sintheta);
    y3 = (int) (fy3 + alt*costheta);

    wp_x = (int)(fx3 - coff*sintheta);
    wp_y = (int)(fy3 + coff*costheta);

    wp_limits.left  = (int)((WP_cord) - rad);
    wp_limits.right = (int)((WP_cord) + rad);
    wp_limits.top   = (int)((WP_cord) - rad);
    wp_limits.bottom= (int)((WP_cord) + rad);
    /* +++ this is really poor and needs a better theory to optimize loading/creation */
    ComputeVisibleLast();
}

