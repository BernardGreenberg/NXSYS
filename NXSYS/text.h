#ifndef _NXSYS_TEXT_STRING_H__
#define _NXSYS_TEXT_STRING_H__

/* 14 January 1998 */
#include "typeid.h"
#include "nxgo.h"
#ifdef TLEDIT
#include <stdio.h>
#endif


#define NXSYS_DEFAULT_TEXT_HEIGHT 20
#define NXSYS_DEFAULT_TEXT_WEIGHT FW_BOLD

#include <string>
#include "PropCell.h"

GraphicObject*  LayoutTextString (const char * string,     LOGFONT * lftemplate,
                                  long x, long y, COLORREF color, BOOL colorgiven);

class TextString : public GraphicObject, public PropEditor<TextString>{

public:
    TextString (const char * string,  LOGFONT * lftemplate, RW_cord x, long y,
                COLORREF color, BOOL colorgiven);

    /* funciones */
    void SetNewLogfont (LOGFONT *);
    LOGFONT& RedeemLogfont();

    /* variables de estato */
    double AssumedScale;
    std::string String;
    int  OriginalLogfontIndex;
    HFONT HFont;
    COLORREF Color;
    BOOL ColorGiven;

    /* funciones virtuales de GraphicObject */
    virtual void    Display(HDC dc) ;
    virtual TypeId  TypeID();
    virtual BOOL    HitP (long x, long y);
    virtual bool    MouseSensitive();

#ifdef TLEDIT
    class PropCell : public PropCellPCRTP<PropCell, TextString> {
        double AssumedScale;
        std::string String;
        int OriginalLogfontIndex;
        HFONT HFont;
        COLORREF Color;
        BOOL ColorGiven;
    public:
        void Snapshot_ (TextString* t) {
            AssumedScale = t->AssumedScale;
            String = t->String;
            OriginalLogfontIndex = t->OriginalLogfontIndex;
            HFont = t->HFont;
            Color = t->Color;
            ColorGiven = t->ColorGiven;
        }
        void Restore_ (TextString* t) {
            t->AssumedScale = AssumedScale;
            t->String = String;
            t->OriginalLogfontIndex = OriginalLogfontIndex;
            t->HFont = HFont;
            t->Color = Color;
            t->ColorGiven = ColorGiven;
        }
    };
    
    virtual void EditClick(int x, int y);
    virtual BOOL_DLG_PROC_QUAL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
    virtual ~TextString();
    virtual int Dump (ObjectWriter& f);

#endif
private:
    void ScaleSelf();
};


#endif
