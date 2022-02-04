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

#if ! NXSYSMac
#include <new.h>
#endif

/* 14 January 1998 */
/* Seriously improved for STL/C++11 9/2019 */

class TextFontData {
    public:
	LOGFONT LFGiven;
	HFONT   HFont;

	TextFontData(LOGFONT * lfp) {
            LFGiven = *lfp;
	    HFont = CreateFontIndirect (lfp);
	}

	int operator == (LOGFONT * lfp) {
	    return !memcmp (&LFGiven, lfp, sizeof(LOGFONT));
	}
};

/* I suspect this creates relocation problems, mewbuf or not  9/3/2014 */
/* solved 9/2019 -- we save indices, not pointers */

static std::vector<LOGFONT> LFTemplates;
static std::vector<TextFontData> Fonts;


/* These should last as long as the NXSYS, no need to delete them
   for new interlocking */

static int GetCanonicalLogfontIndex (LOGFONT* lfp) {
    for (auto& lfs : LFTemplates) {
	if (!memcmp (&lfs, lfp, sizeof(LOGFONT)))
            return (int)(&lfs - LFTemplates.data());
    }
    LFTemplates.push_back(*lfp);
    return (int)(LFTemplates.size()-1);
}



static HFONT GetCanonicalTextFont (LOGFONT* lfp) {
    for (auto& fp : Fonts)
	if (fp == lfp)
            return fp.HFont;
    Fonts.emplace_back(lfp);
    return Fonts.back().HFont;
}

LOGFONT& TextString::RedeemLogfont() {
    return LFTemplates[OriginalLogfontIndex];
}

void TextString::SetNewLogfont (LOGFONT* lf) {
    OriginalLogfontIndex = GetCanonicalLogfontIndex (lf);
    AssumedScale = NXGO_Scale;
    ScaleSelf();
    Invalidate();
}

TextString::TextString (const char * string, LOGFONT * lf,
			RW_cord x, long y, COLORREF color, BOOL colorgiven) :
   Color(color), ColorGiven(colorgiven), String(string) {

/* version 1 nonsense */

#if NXV2
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
    LOGFONT lf (RedeemLogfont());  // make copy
    

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
    int height = DrawText (hDC, String.c_str(), String.size(), &r,
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
#if TLEDIT
    if (Selected)
	SetTextColor (hdc, RGB(0,255,0));
    else
#endif
	SetTextColor (hdc, ColorGiven ? Color : TrackDftCol);
    /* whoops have to scale the damned thing for screen scaling*/
    SelectObject (hdc, HFont);
    DrawText (hdc, String.c_str(), String.size(), &r,
	      DT_VCENTER | DT_CENTER |DT_SINGLELINE | DT_NOCLIP);
    SetTextColor (hdc, tc);
    SelectObject (hdc, Fnt);
}

#if TLEDIT
bool TextString::MouseSensitive() {return true;}
#else
bool TextString::MouseSensitive() {return false;}
#endif

int TextString::TypeID () {return ID_TEXT;}

BOOL TextString::HitP(long x, long y) {
    if (MouseSensitive())
        return GraphicObject::HitP(x, y);
    else
        return FALSE;
}

/* how's this get deleted?   -- (answer 9/2019: General GraphicObject deletion) */

void  LayoutTextString (const char * string, LOGFONT * lf, long x, long y,
		       COLORREF color, BOOL color_given){
    new TextString (string, lf, (RW_cord) x, y, color, color_given);
}

void TextFontCleanup() {
    for (auto& f : Fonts)
	DeleteObject(f.HFont);
}


