#define MACDONT 1  // stops windows.h from defining contentious names (e.g., BOOL, Polygon)
#include "windows.h"
#include <vector>
#include <string>
#include <unordered_set>
#include "WinMacCalls.h"

#import "AppDelegate.h"
#import "WinDialogProtocol.h"

extern HWND G_mainwindow;
void SetViewportDimsFromWindow(HWND);

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
    HFONT font;          /* Windows model maintains this quantity and plumbs it to DrawText */
    bool childOfMain;    /* Affects ShowWindow */
    
    NSString * Description;  // strong, but not important to proclaim it.
    /* these are not actually referenced, just stored into and cleared to retain their pointees */
    __strong NSWindowController* strongController;
    __strong NSWindow * strongWindow;
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
    void ShowWindow(int mode);
    void EnableWindow(bool b);
    void DestroyWindow();
    void SetText(NSString* text);
    id<WinDialog> WinDlg() {
        return (id<WinDialog>)controller;
    }
};

typedef struct __WND_ * PWND;



/* There is no reason not to use the actual pointers as hwnds as long as they are completely validated by inclusion in this set, which is meticulously managed to always be accurate. 
    Read the Great White Paper on the port.
 */

static std::unordered_set<PWND> pWindMap;        //unordered 8/23/2019

static struct __WND_ DesktopAvatar;
static PWND DesktopWindow = &DesktopAvatar;


HWND GetDesktopWindow(){
    return DesktopWindow;
}

static PWND redeemHWND(HWND hWnd) { // no one else should be calling this.
    assert(hWnd != NULL);
    PWND pWnd = (PWND)hWnd;
    assert(pWindMap.count(pWnd) != 0);
    return pWnd;
}


static void LegitimateWinWrapper(PWND wr) {
    assert(pWindMap.count(wr) == 0);
    pWindMap.insert(wr);
}

static void DelegitimateWinWrapper(PWND wr) {
    assert(pWindMap.count(wr) != 0);
    pWindMap.erase(wr);
}


void GDISetMainWindow(NSWindow * window, NSView * view) {
    DesktopWindow->Description = @"DesktopWindow";
    PWND pWnd = new __WND_;
    pWnd->window = window;
    pWnd->view = view; /* sehr wichtig */
    pWnd->controller = nil; // should be only window-but-no-controller in the store.
    pWnd->childOfMain = false;
    pWnd->strongController = nil; // of necessity, must be that already, no?
    pWnd->strongView = nil; // of necessity, must be that already, no?
    pWnd->Description = @"Main Window";

    
    HWND hWnd = (HWND)pWnd;
    G_mainwindow = hWnd;
    SetViewportDimsFromWindow(G_mainwindow);
}


HWND WinWrapRetainView(NSView* view, NSString* description) {
    PWND pWnd = new __WND_;
    pWnd->controller = nil;
    pWnd->control_id = 0;
    pWnd->childOfMain = false;
    pWnd->window = nil;
    pWnd->view = view;
    pWnd->strongView = view;
    pWnd->Description = [[NSString alloc] initWithFormat:@"__strong view %@", description];
    return (HWND)pWnd;
};

HWND WinWrapRetainController(NSWindowController* controller,  NSView* view, NSString* description) {
    PWND pWnd = new __WND_;
    assert(controller != nil);
    pWnd->control_id = 0;
    pWnd->controller=controller; //could well be nil
    pWnd->window = controller.window;
    pWnd->strongController = controller;
    pWnd->view = view;
    pWnd->childOfMain = false;
    pWnd->Description = [[NSString alloc] initWithFormat:@"__strong ctlr %@", description];
    return (HWND)pWnd;
};


HWND WinWrapRetainWindow( NSWindow* window,  NSView* view, NSString* description, bool com) {
    PWND pWnd = new __WND_;
    pWnd->controller = nil;
    pWnd->control_id = 0;
    pWnd->window = window;
    pWnd->strongWindow = window;
    pWnd->view = view;
    pWnd->childOfMain = com;
    pWnd->Description = [[NSString alloc] initWithFormat:@"__strong window %@", description];
    return (HWND)pWnd;
};

HWND WinWrapNoRetain(NSWindowController* controller, NSView* view, NSString* description) {
    PWND pWnd = new __WND_;
    assert(controller != nil);
    pWnd->controller = controller;
    pWnd->window = controller.window;
    pWnd->view = view;
    pWnd->childOfMain = false;
    pWnd->Description = [[NSString alloc] initWithFormat:@"__weak ctlr %@", description];
    return (HWND)pWnd;
};


HWND WinWrapControl(NSWindowController* controller, NSView* control, NSInteger control_id, NSString* description) {
    PWND pWnd = new __WND_; // will legitimate
    assert(controller!=nil);
    pWnd->controller = controller;
    pWnd->window = nil;
    pWnd->view = control;
    pWnd->control_id = control_id;
    pWnd->childOfMain = false;
    pWnd->Description = [[NSString alloc] initWithFormat:@"%@ dlg control %ld",description, control_id];
    return (HWND)pWnd;
}

void WrapSetChildOfMain(HWND hWnd) {
    redeemHWND(hWnd)->childOfMain = true;
}

NSView* getHWNDView(HWND hWnd) {
    return redeemHWND(hWnd)->view;
}

NSWindow* getHWNDWindow(HWND hWnd) {
    return redeemHWND(hWnd)->window;
}

NSWindowController* getHWNDController(HWND hWnd) {
    return redeemHWND(hWnd)->controller;
}

NSInteger getHWNDCtlid(HWND hWnd) {
    return redeemHWND(hWnd)->control_id;
}

HFONT getHWNDFont(HWND hWnd) {
    return redeemHWND(hWnd)->font;
}

void ShowWindow(HWND hWnd, int mode){
    redeemHWND(hWnd)->ShowWindow(mode);
}

void EnableWindow(HWND hw, BOOL B){
    redeemHWND(hw)->EnableWindow(B ? true : false);
}

void EnableWindow(HWND hw, bool b){
    redeemHWND(hw)->EnableWindow(b);
}

void DestroyWindow(HWND hWnd) {
    redeemHWND(hWnd)->DestroyWindow();
}

void SetWindowText(HWND hWnd, const char * text) {
    redeemHWND(hWnd)->SetText([[NSString alloc] initWithUTF8String:text]);
}


void DeleteHwndObject(HWND hWnd) {  // releases strongptrs and reclaims storage.
    /* destructor should release all the strongptrs and deregister all the weak ones.
       We know for a fact it delegitimates the value of hWnd. */
    delete redeemHWND(hWnd);
}

void __WND_::ShowWindow(int mode) {
    switch (mode) {
        case SW_SHOW:
        case SW_SHOWNOACTIVATE:
        case SW_SHOWNORMAL:
            if (control_id != 0) {
                [WinDlg() showControl:control_id showYes:1];
            } else if (childOfMain) {
                NSWindow * mainwnd = getNXWindow();
                [mainwnd addChildWindow:window ordered:NSWindowAbove];
                [mainwnd makeKeyWindow];  //set focus back to avoid double-click --
                // assumption is that most NXSYS artifacts are display-only, not input.
            } else if (!window && view) {
                [view setHidden:NO];
            } else {
                [window makeKeyAndOrderFront:nil];
            }
            break;
        case SW_HIDE:
            if (control_id != 0) {
                [WinDlg() showControl:control_id showYes:0];
            } else if (childOfMain) {
                [window close];
                [getNXWindow() removeChildWindow:window];
            } else if (!window && view) {
                [view setHidden:YES];
            } else {
                [window orderOut:nil];
            }
            break;
        default:
            //Called by WinMain, show parameter passed from OS -- hey, wait a minute . .
            // assert(!"Unknown ShowWindow param.");
            break;
    }
}


void __WND_::SetText(NSString* text) {
    //maybe need "set dlg item text" different.
    if (control_id != 0) {
        [WinDlg() SetControlText:control_id text:text];
    } else {
        //assert(window != nil);
        //Most unfortunately, full signal views are "windows" with no (Mac) window that
        //truly want to ignore the SetText that NXSYS sends to them, and NXSYS isn't
        //supposed to know about it.  O woe.
        if (window != nil)
            [window setTitle:text];
    }
}

void __WND_::DestroyWindow() {
    assert(control_id == 0);  // not supposed to use this on controls!
    if (controller != nil) {
        if ([controller conformsToProtocol:@protocol(WinDialog)])
            [WinDlg() DestroyWindow];
    }

    if (window == nil && view != nil) {   //is this a window-which-is-really-a-view?
        /* This is truly black magic here.  Apparently, removeFromSuperviewWithoutNeedingDisplay
         tosses the removed view into "the current autorelease pool", effectively retaining it
         until that pool's draining, which is end of command dispatch.  This causes the last assert,
         which verifies that it really went away, to complain that it didn't.
         This Pre-ARC article http://www.cocoabuilder.com/archive/cocoa/124231-nsview-removefromsuperview-not-affecting-retain-count.html
         by an Apple "Cocoa Sr. Developer" describes a motivation for this, viz., to be able
         to move a view from one superview to another "in the same command dispatch" without its
         going away.   There is sample code there.  The situation under ARC doesn't seem much
         different.  Fortunately, I retain my views in my own structures, so I now feel justified
         in setting up my own pool of promissory releases and fulfilling it at the } .
         */
        @autoreleasepool {

            [view removeFromSuperviewWithoutNeedingDisplay];
        }
    }
#ifdef NXCheckWinRelease
    NSWindow __weak *weakwindow = strongWindow;
    NSView __weak *weakview = strongView;
    NSWindowController __weak *weakctrlr = strongController; //see if Heaven really works.
#endif

    delete this;    // "we can't delete this!" (this statement, that is)
   
#ifdef NXCheckWinRelease // just too iffy, can't trust it not to autorelease
    assert(weakctrlr == nil);  // verify the work of the Hand of Heaven.
    assert(weakview == nil);
    if (getenv("NXSYSDEBUG")) {
        // Atlantic avenue: show 244 FFSW; close main window from redbutton icon, crash here.
        assert(weakwindow == nil); // so don't bomb out users
    }
#endif
}

void EndDialog(HWND hWnd, bool) {
    PWND pWnd = redeemHWND(hWnd);
    assert(pWnd != DesktopWindow);
    [NSApp stopModal];
    //TLedit dialogs don't flush unless you do this.
    [pWnd->window orderOut:nil];
    [pWnd->window close];
 
}

void __WND_::EnableWindow(bool b) {
    if (control_id) {
        if ([controller conformsToProtocol:@protocol(WinDialog)])
            [WinDlg() EnableControl:control_id yesNo:(b ? YES : NO)];
    }
}

/* These two need windows.h -- need better solution */

void DeleteMenu(HMENU menu, int cmd, int flags) {
#ifndef TLEDIT
 if (flags & MF_BYCOMMAND) {
        MacDeleteMenuItem(menu, cmd);
    }
#endif
}

void EnableMenuItem(HMENU menu, int cmd, int flags) {
#ifndef TLEDIT  // MacFooItem's not built in TLEDIT
    if (menu == NULL)  // calls on global menu during init. Big kludge.
        return;
    if (flags & MF_BYCOMMAND) {
        MacEnableMenuItem(menu, cmd, !(flags & MF_GRAYED));
    }
#endif
}
