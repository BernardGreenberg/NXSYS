//
//  DeviceContext.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 10/29/14. From Winapi.mm
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>


#define MACDONT 1  // stops windows.h from defining contentious names (e.g., BOOL, Polygon)
#include "windows.h"
#include <vector>
#include <string>
#include "WinMacCalls.h"

static std::vector<LOGFONT> fonts;

#import "WinDialogProtocol.h"

/* This seems necessary to make Text graphics "look right".  No comprendo. */
double WinMacFontRatio = .80;

#define MIN_FONT_SIZE 10.0

BOOL RetrieveGDIWrapperInfo(HGDIOBJ gh, bool& isPen, COLORREF& cr, int& param1, int& param2);

NSRect RectToMac (RECT*);
HFONT getHWNDFont(HWND hWnd);

struct __DC_ {
    HGDIOBJ curPen;
    HGDIOBJ curBrush;
    COLORREF brushColor;
    COLORREF penColor;
    COLORREF bkColor;
    COLORREF textColor;
    HFONT curFont;
    int bkMode;
    BOOL havePen;
    BOOL haveBrush;
    int penMode;
    int penWidth;
    int movedX;
    int movedY;
    BOOL originKnown;
    
    
    __DC_ ();
    __DC_ (HWND hWnd);
    void SelectObject(HGDIOBJ obj);
    void MoveTo(int x, int y);
    void LineTo(int x, int y);
    void PolygonInt(POINT* points, int n);
};

HFONT CreateFontIndirect(LOGFONT* pLF){
    fonts.push_back(*pLF);
    return (HFONT)(fonts.size() + UFONT_BASE -1);
}

void MacReleaseGDIOBJs() {
    fonts.clear();
   // uidgen++;
}



HDC GetDC_() {
    return new __DC_;
}

HDC GetDC(HWND hWnd) {
    return new __DC_(hWnd);
}

__DC_::__DC_ () {
    curBrush = NULL;
    curPen = NULL;
    curFont = NULL;
    originKnown = FALSE;
    havePen = FALSE;
    haveBrush = FALSE;
}

__DC_::__DC_(HWND hWnd) { // C++11 whee!
    __DC_();
    curFont = getHWNDFont(hWnd);
}

/* called from way outside, because rgb is more useful than Mac fractions or NS api */
NSColor *colorFromRGB(unsigned char r, unsigned char g, unsigned char b)
{
    return [NSColor colorWithCalibratedRed:(r/255.0f) green:(g/255.0f) blue:(b/255.0f) alpha:1.0];
}
/* and COLORREF- see text/font dialog in TLEdit */
NSColor *colorFromCOLORREF(COLORREF c) {
    char unsigned b = c & 0xFF;
    c >>= 8;
    char unsigned g = c & 0xFF;
    c >>= 8;
    char unsigned r = c & 0xFF;
    return colorFromRGB(r, g, b);
}
/* for completeness */
COLORREF COLORREFFromNSColor(NSColor * nsc) {
    int redint = 255*nsc.redComponent;
    int grnint = 255*nsc.greenComponent;
    int bluint = 255*nsc.blueComponent;
    return  (((redint << 8) | grnint) << 8) | bluint;
}

void SelectObject(HDC hDC, HGDIOBJ obj) {
    assert(hDC != NULL);
    hDC->SelectObject(obj);
}

void __DC_::SelectObject(HGDIOBJ obj) {
    
    if (obj == NULL)
        return;  /* null fonts when we have not fonts, etc. */
    assert(obj != NULL);
#if 0
    if (hDC == NULL_PEN) {
        /* tbd */
    }
#endif
    if ((long)obj >= UFONT_BASE && (long)obj < UFONT_END) {
        curFont = (HFONT)obj;
        return;
    }

    bool isPen;
    COLORREF cr;
    int param1;
    int param2;
    
    if (!RetrieveGDIWrapperInfo(obj, isPen, cr, param1, param2)) {
        return;
    }
    if (isPen) {
        havePen = TRUE;
        curPen = obj;
        penColor = cr;
        penMode = param1;
        penWidth = param2;
    } else {
        haveBrush = TRUE;
        curBrush = obj;
        brushColor = cr;
    }
}

void LineTo(HDC hDC, int x, int y){
    assert(hDC != NULL);
    hDC->LineTo(x, y);
}

void __DC_::LineTo(int x, int y) {
    assert(havePen);
    NSBezierPath * bezier = [NSBezierPath bezierPath];
    
    [bezier setLineWidth:penWidth];
    
    [bezier moveToPoint:NSMakePoint(movedX, movedY)];
    [bezier lineToPoint:NSMakePoint(x, y)];
    
    NSColor * c = colorFromCOLORREF(penColor);
    [c set];
    
    [bezier stroke];
    movedX = x;
    movedY = y;
    originKnown = TRUE;
}

void MoveTo(HDC hDC, int x, int y){
    assert(hDC != NULL);
    hDC->MoveTo(x, y);
}

void __DC_::MoveTo(int x, int y) {
    movedX = x;
    movedY = y;
    originKnown = TRUE;
}

void MoveToEx(HDC hDC, int x, int y, void* cpp) {
    assert(cpp == NULL); // don't support storing back yet (easy enough, though)
    MoveTo(hDC, x, y);
}

void Ellipse(__DC_* hDC,  int left,  int top,  int right, int bottom) {
    assert(hDC != NULL);
    assert(hDC->haveBrush);
    NSRect rect;
    rect.origin.x = left;
    rect.origin.y = top;
    rect.size.width = right-left;
    rect.size.height = bottom-top;
    NSBezierPath* thePath = [NSBezierPath bezierPath];
    [thePath appendBezierPathWithOvalInRect:rect];
    NSColor * c= colorFromCOLORREF(hDC->brushColor);
    [c set];
    [thePath fill];
}

void FillRect(HDC, RECT* rp, HBRUSH hBrush) {
    bool isPen;
    COLORREF cr;
    int param1;
    int param2;
    
   // assert(hDC != NULL); // don't need one at all in this Mac world . . .
    assert(hBrush != NULL);
    if (!RetrieveGDIWrapperInfo(hBrush, isPen, cr, param1, param2)) {
        assert (!"GDI wrapper retrieval fails in FillRect");
    }
    assert(!isPen);
    NSBezierPath* thePath = [NSBezierPath bezierPath];
    [thePath appendBezierPathWithRect:RectToMac(rp)];
    [colorFromCOLORREF(cr) set];
    [thePath fill];
}

void Rectangle(HDC hDC, int left, int top, int right, int bottom){
    assert(hDC != NULL);
    assert(hDC->haveBrush);
    NSRect rect;
    rect.origin.x = left;
    rect.origin.y = top;
    rect.size.width = right - left;
    rect.size.height = bottom - top;
    NSBezierPath* thePath = [NSBezierPath bezierPath];
    [thePath appendBezierPathWithRect:rect];
    NSColor * c= colorFromCOLORREF(hDC->brushColor);
    [c set];
    [thePath fill];
}


HDC ReleaseDC_(HWND, HDC hDC) {
    
    assert(hDC != NULL);
    delete hDC;
    
    return NULL;
}

HDC ReleaseDC(HWND hWnd, HDC hDC) {
    return ReleaseDC_(hWnd, hDC);
}

static NSRect getTextBr(NSString *theString, NSDictionary *attributes) {
    NSSize size = NSMakeSize(300,300);
    NSRect r = [theString boundingRectWithSize:size
                                       options:0
                                    attributes:attributes];
    return r;
}

int DrawText(__DC_* hDC, char const*s, unsigned long limit, RECT* pr, int flags) {
    std::string tempbuf;
    size_t ll = strlen(s);  /* not really safe */
    if (limit < ll) {
        tempbuf.append(s, limit);
        s = tempbuf.c_str();
    }
    double fh = 10.0;
    const char * fname = "Helvetica";
    bool isBold = false;
    if (hDC->curFont == NULL) {
        fh = 10.0;
    } else {
        long fx = (long)(hDC->curFont) - UFONT_BASE;
        assert(fx >= 0 && fx < fonts.size());
        LOGFONT * pLF = &fonts[fx];
        if (pLF->lfFaceName[0] != '\0')
            fname = pLF->lfFaceName;
        fh = pLF->lfHeight; //These are POINTS in Macintosh
        isBold = (pLF->lfWeight == FW_BOLD);
    }
    fh *= WinMacFontRatio;  /* not understood, either */
    if (fh < MIN_FONT_SIZE) {
        fh = MIN_FONT_SIZE;
    }
    
    NSString * nfname = [[NSString alloc] initWithUTF8String:fname];
    NSColor * fgColor = colorFromCOLORREF(hDC->textColor);
    
    NSFontManager * fontManager = [NSFontManager sharedFontManager];
    NSFont* font = [fontManager fontWithFamily:nfname
                                        traits:(isBold ? NSBoldFontMask : 0)
                                        weight:0
                                          size:fh];
    NSMutableDictionary *attributes  = [NSMutableDictionary
                                        dictionaryWithObjectsAndKeys:
                                        font, NSFontAttributeName,
                                        fgColor, NSForegroundColorAttributeName, nil];
    
    if (hDC->bkMode != TRANSPARENT) {
        NSColor * bkColor = colorFromCOLORREF(hDC->bkColor);
        [attributes setObject:bkColor forKey:NSBackgroundColorAttributeName];
    }
    
    /* http://stackoverflow.com/questions/6730052/how-to-convert-iso-8859-1encoded-string-into-utf-8-in-objective-c */
    NSString * theString = [[NSString alloc]
                            initWithCString: s encoding:NSISOLatin1StringEncoding];
    //[NSString stringWithUTF8String:s]; // no Indian railroads with Bengali nomenclature
    
    NSAttributedString * currentText=[[NSAttributedString alloc] initWithString:theString attributes: attributes];
    
    if (flags & DT_CALCRECT) {
        pr->top = 0;
        pr->left = 0;
        NSRect br = getTextBr(theString, attributes);
        pr->right = br.size.width;
        pr->bottom = br.size.height;
        return pr->bottom;
    }
    
    double x = pr->left;
    double y = pr->top;
    
    if (flags & DT_VCENTER) {
        double boxh = pr->bottom - pr->top;
        y += boxh/2 - fh/2; //Eureka!
    }
    if (flags & DT_CENTER) {
        double boxw = pr->right - pr->left;
        
        NSRect br = getTextBr(theString, attributes);
        x += boxw/2 - br.size.width/2;
        
    }
    
    [currentText drawAtPoint:NSMakePoint(x, y)];
    return 0;
}

void NXM_Polygon(HDC hDC, POINT* points, int n) {
    assert(hDC != NULL);
    hDC->PolygonInt(points, n);
}

void __DC_::PolygonInt(POINT *points, int xn) {

    NSBezierPath *bezier = [NSBezierPath bezierPath];
    [bezier moveToPoint:NSMakePoint(points[0].x, points[0].y)];
    for (int i = 1; i < xn; i++) {
        [bezier lineToPoint:NSMakePoint(points[i].x, points[i].y)];
    }
    bool strokin = false;
    if (curPen == NULL || curPen == GetStockObject(NULL_PEN)) {
        [bezier setLineWidth:0.0];
    } else {
        [bezier setLineWidth:(double)(penWidth)];
        strokin = true;
    }
    
    [bezier closePath];
    
    NSColor * c= colorFromCOLORREF(brushColor);
    [c set];
    [bezier fill];
    if (strokin) {
        NSColor * c = colorFromCOLORREF(penColor);
        [c set];
        [bezier stroke];
    }
}

void SetBkColor(HDC hDC, COLORREF color) {
    hDC->bkColor = color;
}

void SetTextColor(HDC hDC, COLORREF color) {
    hDC->textColor = color;
}

COLORREF GetTextColor(HDC hDC) {
    return hDC->textColor;
}

void SetBkMode(HDC hDC, int mode) {
    hDC->bkMode = mode;
}

int  GetBkMode(HDC hDC) {
    return hDC->bkMode;
}





