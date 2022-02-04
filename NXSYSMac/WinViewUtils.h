//
//  WinViewUtils.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/24/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#ifndef NXSYSMac_WinViewUtils_h
#define NXSYSMac_WinViewUtils_h


#include "WinMouseDefs.h"

typedef struct __DC_ *HDC;
HDC GetDC_();
void ReleaseDC_(void*, HDC);
struct RECT {int left; int right; int top; int bottom;};
void DisplayVisibleObjectsRect(struct __DC_ *, struct RECT&);
void GDISetMainWindow(NSWindow* window, NSView* view);
void NXGO_Rodentate(unsigned, unsigned, unsigned);
void NXGOMouseUp();
void SetTextColor(__DC_*, int);
void SetBkMode(__DC_*, int);
void InvalidateRect(void*, RECT*, int);
void InvalidateRelayDrafter();
void SelectObject(__DC_*, void*);
void* GetStockObject(void*);
void LineTo(HDC, int, int);
void MoveTo(HDC, int, int);

#define TRANSPARENT 32768
#define BLACK_PEN (void*)1004



#endif
