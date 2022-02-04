//
//  MacFullSignals.mm, erstwhile FSWindowController.mm, but there is no more controller.
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/23/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//
////////////////

/* 11/04/2014

 This has become dirt-simple as it should be. Read my paper about the Mac port to learn about
 all the stages it went through and what it used to be. Erstwhile King of Confusion, now 
 Duke of Simplicity. */

#import "AppDelegate.h"
#import "FullSignalView.h"
#include "WinMacCalls.h"

//see http://objc.toodarkpark.net/AppKit/Classes/NSWindow.html#//apple_ref/occ/instm/NSWindow/
// http://objc.toodarkpark.net/AppKit/Classes/NSPanel.html on panel vs window as below

bool FullSigsAreViews = false;  //option persisted and maintained by Preferences dialog

/* Quite a remarkable thing. Don't need any controller at all. */

static HWND createFullFullSignalWindowHWND(NSRect frame) {
    NSPanel * panel = [[NSPanel alloc] initWithContentRect:frame /* panel vs window is exactly right*/
                                                 styleMask:NSWindowStyleMaskTitled
                                                   backing:NSBackingStoreBuffered
                                                     defer:NO];
    /* Title gets set by Windows code */
    [panel setFrameTopLeftPoint:NXViewToScreen(frame.origin)]; /*this positions it */
    [panel setMovableByWindowBackground:YES];  /* documented NXSYS feature */
    [panel setBecomesKeyOnlyIfNeeded:YES]; /* "no become Main" is default for panels */
    [panel setContentView:[[FullSignalView alloc] initWithFrame:frame]]; /* this implements it */
    [getNXWindow() addChildWindow:panel ordered:NSWindowAbove]; /* this displays it */

    return WinWrapRetainWindow(panel, panel.contentView, @"FullsigWin", true); /* this retains it */
}

HWND MacMakeFSW(int x, int y, int w, int h, void*signal) {
    NSRect r = NSMakeRect(x, y, w, h);
    HWND hWnd = FullSigsAreViews
            ?  [FullSignalView CreateFloatingViewFromRectHWND:r]
            : createFullFullSignalWindowHWND(r);
    [(FullSignalView*)(getHWNDView(hWnd)) setPSig:signal]; /* works either case */
    return hWnd;
}

