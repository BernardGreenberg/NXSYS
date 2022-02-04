//
//  MiscApi.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 11/6/14 from Winapi.mm.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"
#include <sys/time.h>
#include "WinMacCalls.h"
#include "WinViewUtils.h" // declares RECT
#include "MessageBox.h"

HWND GetDesktopWindow();

void FatalAppExit(int, const char * text) {
    MessageBox(NULL, text, "NXSYS Fatal App Exit", MB_ICONSTOP|MB_OK);
    abort();
}

void DebugBreak() {
    abort();
}

void DeleteObject(void *) {
    //This is actually called on fonts and menus a lot. Let it pass.
}

long GetTickCount(){   // Lyme disease is no joke!
    /* http://brian.pontarelli.com/2009/01/05/getting-the-current-system-time-in-milliseconds-with-c/ */
    timeval time;
    gettimeofday(&time, NULL);
    return (time.tv_sec * 1000) + (time.tv_usec / 1000);
}

void MacAssertTrueLayoutDims(int width, int height) {
    NSRect frame = [getMainView() frame];
    frame.size.width = width;
    if (height > frame.size.height)
        frame.size.height = height;
    
    [getMainView() setFrame:frame];
}

static void NSRectToRECT(NSRect nsr, RECT* pr) {
    pr->top = (int)nsr.origin.y;
    pr->left = (int)nsr.origin.x;
    pr->bottom = (int)nsr.size.height + pr->top; /* is this even directionwise right? */
    pr->right = (int)nsr.size.width + pr->left;
}

void GetWindowRect(HWND hWnd, RECT*r) {
    /* distinction between frame and bounds is relevant here -- fudge it for now, all origins at 0 ++++ */
    if (hWnd == GetDesktopWindow()) {
        NSRectToRECT([[NSScreen mainScreen] visibleFrame], r);
    } else {
        NSRectToRECT([getHWNDView(hWnd) bounds], r);
    }
}

void GetClientRect(HWND hWnd, RECT*r) {
    if (hWnd == GetDesktopWindow()) {
        GetWindowRect(hWnd, r);
        return;
    }
    NSRectToRECT([getHWNDView(hWnd) bounds], r);
}

NSRect RectToMac(RECT* wr) {
    NSRect nsr;
    nsr.origin = NSMakePoint(wr->left, wr->top);
    nsr.size.width = wr->right - wr->left;
    nsr.size.height = wr->bottom - wr->top;
    return nsr;
}


void InvalidateRect(HWND hWnd, RECT* r, int) {
    if (hWnd == NULL) //can happen during initialization.
        return;
    __weak NSView* view = getHWNDView(hWnd); //? why weak ?
    if (r == NULL) {
        [view setNeedsDisplay: TRUE];
    }
    else {
        [view setNeedsDisplayInRect:RectToMac(r)];
    }
}

NSPoint NXViewToScreen (NSPoint point) {
    NSPoint tp = [getMainView() convertPoint:point toView:nil];
    NSRect mf = getNXWindow().frame;
    // Don't know why apple "base" primitives don't do this for me . . .
    tp.x += mf.origin.x;
    tp.y += mf.origin.y;
    return tp;
}

void setFrameTopLeft(HWND hWnd, int x, int y) {
    [getHWNDWindow(hWnd) setFrameTopLeftPoint:NXViewToScreen(NSMakePoint(x,y))];
}

void Sleep(long msec) {
    /* Never the right way -- just here to get TLEdit to compile and link */
    assert(!"Meine Seele, Meine Seele, warum schläfst du doch?"); //кондак Св. Андрея Критскаго
    double sec = ((double) msec)/1000.0;
    [NSThread sleepForTimeInterval:(NSTimeInterval)sec];
}

/*  This is really  nxsys-specific, but so is AppDelegate.h .... */

NSPoint NXGOLocAsPoint(void* NXGObject) {
    long x, y;
    NXGOExtractWPCoords(NXGObject, x, y); //by reference params
    return NSMakePoint(x, y);
}

