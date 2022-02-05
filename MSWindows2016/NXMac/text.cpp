#include "windows.h"
#include <string.h>
#include <vector>
#ifndef NXV2
#include "track.h"
#endif
#include "text.h"
#include "objid.h"
#include "brushpen.h"
#include "nxsysapp.h"
#ifdef NXSYSMac

#else
#include <new.h>
#endif

/* 14 January 1998 */

class TextFontData {
    public:
	LOGFONT LFGiven;
	HFONT   HFont;

	void *operator new (size_t);
	TextFontData(LOGFONT * lfp) {
	    memcpy (&LFGiven, lfp, sizeof(LOGFONT));
	    HFont = CreateFontIndirect (lfp);
	}
        TextFontData() {
        }

	int operator == (LOGFONT * lfp) {
	    return !memcmp (&LFGiven, lfp, sizeof(LOGFONT));
	}
};

/* I suspect this creates relocation problems, mewbuf or not  9/3/2014 */

static std::vector<LOGFONT> LFTemplates(25);
static std::vector<TextFontData> Fonts(25);


/* These should last as long as the NXSYS, no need to delete them
   for new interlocking */

static LOGFONT * GetCanonicalLogfontStructure (LOGFONT* lfp) {
    for (std::vector<LOGFONT>::iterator I = LFTemplates.begin();
         I != LFTemplates.end();
         I++) {
	if (!memcmp ((LOGFONT*)&(*I), lfp, sizeof(LOGFONT)))
	    return &(*I);
    }
    size_t index = LFTemplates.size();
    LFTemplates.push_back(*lfp);
    return &(LFTemplates[index]);
}
    

static HFONT GetCanonicalTextFont (LOGFONT* lfp) {
    for (std::vector<TextFontData>::iterator I = Fonts.begin();
         I != Fonts.end(); I++) {
	if (*I == lfp)
	    return I->HFont;
    }
    TextFontData tfd(lfp);
    Fonts.push_back(tfd);
    return tfd.HFont;
}


void TextString::SetNewLogfont (LOGFONT* lf) {
    pOriginalLogfont = GetCanonicalLogfontStructure (lf);
    AssumedScale = NXGO_Scale;
    ScaleSelf();
    Invalidate();
}

TextString::TextString (const char * string, LOGFONT * lf,
			RW_cord x, long y, COLORREF color, BOOL colorgiven) :
   Color(color), ColorGiven(colorgiven), String(_strdup(string)) {

    StringLength = strlen(String);
/* version 1 shit */

#ifdef NXV2
    wp_x = x;
    wp_y = y;
#else
    TrackDef * td = TrackDefs[0];	/* better be one */
    wp_x = RWx_to_WPx (td, rw_x = x);
    wp_y = RWyhun_to_WPy (y);
#endif
    SetNewLogfont (lf);
}
   

void TextString::ScaleSelf () {
    LOGFONT lf;
    memcpy (&lf, pOriginalLogfont, sizeof(lf));

    if (lf.lfHeight == 0)
	lf.lfHeight = NXSYS_DEFAULT_TEXT_HEIGHT;
    if (lf.lfWeight == 0)
	lf.lfWeight = NXSYS_DEFAULT_TEXT_WEIGHT;

    lf.lfHeight = (int) (NXGO_Scale*lf.lfHeight + .5);
    lf.lfWidth = (int) (NXGO_Scale*lf.lfWidth + .5);
    
    HFont = GetCanonicalTextFont (&lf);
    HDC hDC = GetDC (G_mainwindow);
    RECT r;
    memset (&r, 0, sizeof(r));
    SelectObject (hDC, HFont);
    int height = DrawText (hDC, String, StringLength, &r,
			   DT_SINGLELINE | DT_NOCLIP |  DT_CALCRECT);
    ReleaseDC (G_mainwindow, hDC);
    wp_limits.left = - r.right/2 - 1;
    wp_limits.right = r.right/2 + 1;
    wp_limits.top = -height/2 - 1;
    wp_limits.bottom = height/2 + 1;
    AssumedScale = NXGO_Scale;
}

void TextString::Display (HDC hdc) {
    if (NXGO_Scale != AssumedScale)
	ScaleSelf();

    RECT r;
    r.left = WPXtoSC (wp_x + wp_limits.left);
    r.right = WPXtoSC (wp_x + wp_limits.right);
    r.top = WPYtoSC (wp_y + wp_limits.top);
    r.bottom = WPYtoSC (wp_y + wp_limits.bottom);
    SelectObject (hdc, HFont);
    COLORREF tc = GetTextColor (hdc);
#ifdef TLEDIT
    if (Selected)
	SetTextColor (hdc, RGB(0,255,0));
    else
#endif
	SetTextColor (hdc, ColorGiven ? Color : TrackDftCol);
    /* whoops have to scale the damned thing for screen scaling*/
    SelectObject (hdc, HFont);
    DrawText (hdc, String, StringLength, &r,
	      DT_VCENTER | DT_CENTER |DT_SINGLELINE | DT_NOCLIP);
    SetTextColor (hdc, tc);
    SelectObject (hdc, Fnt);
}

int TextString::TypeID () {return ID_TEXT;}
int TextString::ObjIDp (long) {return 0;}

BOOL TextString::HitP(long x, long y) {
#ifdef TLEDIT
    return GraphicObject::HitP(x, y);
#else

#ifndef NXSYSMac
    x; y;
#endif
    return FALSE;
#endif

}
   

/* how's this get deleted? */

void  LayoutTextString (char * string, LOGFONT * lf, long x, long y,
		       COLORREF color, BOOL color_given){
    new TextString (string, lf, (RW_cord) x, y, color, color_given);
}

void TextFontCleanup() {
    for (size_t i = 0; i < Fonts.size(); i++)
	DeleteObject(Fonts[i].HFont);
}


