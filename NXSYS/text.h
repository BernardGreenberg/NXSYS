#ifndef _NXSYS_TEXT_STRING_H__
#define _NXSYS_TEXT_STRING_H__

/* 14 January 1998 */

#include "nxgo.h"
#ifdef TLEDIT
#include <stdio.h>
#endif


#define NXSYS_DEFAULT_TEXT_HEIGHT 20
#define NXSYS_DEFAULT_TEXT_WEIGHT FW_BOLD

#include<string>

class TextString : public GraphicObject {

    public:   
	TextString (const char * string,  LOGFONT * lftemplate, RW_cord x, long y,
		   COLORREF color, BOOL colorgiven);

	void SetNewLogfont (LOGFONT *);
        LOGFONT& RedeemLogfont();
	double AssumedScale;
        std::string String;
        int  OriginalLogfontIndex;
	HFONT HFont;
	COLORREF Color;
	BOOL ColorGiven;

	virtual void    Display(HDC dc) ;
	virtual int     TypeID();
	virtual BOOL    HitP (long x, long y);
        virtual bool    MouseSensitive();
#ifdef TLEDIT
	virtual void EditClick(int x, int y);
	virtual BOOL_DLG_PROC_QUAL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
	virtual ~TextString();
	virtual int Dump (FILE * f);

#endif
    private:
	void ScaleSelf();
};


void  LayoutTextString (const char * string, 	LOGFONT * lftemplate,
			long x, long y, COLORREF color, BOOL colorgiven);


#endif
