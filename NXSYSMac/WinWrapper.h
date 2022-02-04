//
//  WinWrapper.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 10/13/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//



#ifndef NXSYSMac_WinWrapper_h
#define NXSYSMac_WinWrapper_h

#ifndef IS_WINAPI_MM
#error WinWrapper.h may only be included by Winapi.mm!!!
#endif

#import <Cocoa/Cocoa.h>

typedef void* HFONT;

/* There is only one structure for these window wrappers, whose addresses are passed around as "hWnds".
  There are four kinds, though,
   (1) For real windows, such as old-style full signals and nonmodal dlgs like trains etc.  These
    have the strongptrs set, and retain the pointed-to.  When this is deleted, the Cocoa object goes away.
    The strongptrs are not "used", they are only retained; the weak pointers are used compatibly with (2).
   (2) views such as new-style full signals.  These only use the weak pointers. Obviously, they do not retain,
    and there is no consequence to their deletion.
   (3) dialog controls, where control_id != 0.  These work pretty much as the views in (2).  Maybe
    we don't really need the control_id (the dialogs maintain maps from control_id to these).
   (4) modal dialogs - do not have strongptrs -- the stack retains the dialog, we need not retain it.
 All of these are registered by address in pWindMap in Winapi.mm, and are validated as appearing in it
 before dereference in any way, esp. deletion.  The methods getHWNDView, getHWNDWindow, etc. must be used,
 except in Winapi.mm, where the check is made every time.
 */
 
static void LegitimateWinWrapper(struct __WND_*);
static void DelegitimateWinWrapper(struct __WND_*);

struct __WND_ {
    NSInteger control_id;
    __weak NSView* view;
    __weak NSWindow* window;
    __weak NSWindowController* controller;
    HFONT font;
    bool childOfMain;

    NSString * Description;  // strong, but not important to proclaim it.
    __strong NSWindowController* strongController;
    __strong NSView* strongView;
    __WND_ () {  // cocoa pointers will not go uninitted by Cocoa.
        childOfMain = false;
        font = NULL;
        control_id = 0;
        LegitimateWinWrapper(this);
    }
    ~__WND_() {
        DelegitimateWinWrapper(this);
    }
    
};

typedef struct __WND_ * PWND;





#endif
