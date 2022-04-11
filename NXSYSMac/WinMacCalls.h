//
//  WinMacCalls.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/16/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#ifndef NXSYSMac_WinMacCalls_h
#define NXSYSMac_WinMacCalls_h


/* Generic Windows-emulating stuff */

typedef void* HWND;
@class NSString;

void DeleteHwndObject(HWND hWnd); // includes killing strongpointers and delegitimating itself.
/* apparently, the default for arguments is __strong...*/
HWND WinWrapRetainController( NSWindowController* controller,  NSView* view, NSString* description);
HWND WinWrapRetainWindow( NSWindow* window,  NSView* view, NSString* description, bool com);
HWND WinWrapRetainView(NSView* view, NSString* description);
HWND WinWrapNoRetain(NSWindowController* controller, NSView* view, NSString* description);
HWND WinWrapControl(NSWindowController* controller, NSView* control, NSInteger control_id, NSString* description);

void WrapSetChildOfMain(HWND hWnd);

NSView * getHWNDView(HWND);
NSWindow * getHWNDWindow(HWND);
NSWindowController * getHWNDController(HWND);
NSInteger getHWNDCtlid(HWND);

NSColor* colorFromRGB(unsigned char r, unsigned char g, unsigned char b);
void setFrameTopLeft(HWND hWnd, int x, int  y);
NSPoint NXViewToScreen (NSPoint point);
void MacDeleteMenuItem(void* menu_cell, int cmd);
void MacEnableMenuItem(void* menu_cell, int cmd, bool yesNo);

/* NXSYS-specific stuff */
HWND getDrafterHWND(); // from main delegate frame.
void NXGOExtractWPCoords(void* NXGObject, long& x, long& y);
NSPoint NXGOLocAsPoint(void* NXGObject);
void RelayGraphicsLeftClick(int x, int y);
const char* RelayGraphicsNameFromXY(int x, int y);

/* Custom unique_ptr class; needed because a custom deleter is involved. 4-10-2022
   When used consistently, removes the need for the custom deallocator. */
/* https://en.wikipedia.org/wiki/C++11#Explicitly_defaulted_and_deleted_special_member_functions */

class HWNDAutoPtr  {
    HWND hWnd = nullptr;  /* What a miracle that this is legal and works these days... */
    /* make non-copyable */
    HWNDAutoPtr & operator=(const HWNDAutoPtr&) = delete;
    HWNDAutoPtr(const HWNDAutoPtr&) = delete;
public:
    HWNDAutoPtr() = default;
    HWNDAutoPtr(HWND h) {hWnd = h;}
    ~HWNDAutoPtr() {
        if (hWnd)
            DeleteHwndObject(hWnd);
    }
    operator HWND () { return hWnd;}
    void set(HWND h){
        assert(hWnd == nullptr);
        hWnd = h;
    }
    void operator = (HWND h) {set(h);}
};

#endif
