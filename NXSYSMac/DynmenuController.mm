//
//  DynmenuController.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/7/14.
//  Improved 11/2/14 to use whatever contentView is in the window, not make a new one.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "WinDialogProtocol.h"
#import "LittleYellowView.h"
#import "AppDelegate.h"
#include <vector>
#include <map>

#include "WinMacCalls.h"

static const int DYNMENU_ROW_HEIGHT = 25;

@interface DynmenuController : NSWindowController<WinDialog>
{
 std::map<NSInteger,HWNDAutoPtr>CtlidToHWND;
 std::vector<LittleYellowView*> lights;
}
@property void* actor;
@end

void DynmenuButtonPushCallback(void * actor, int control_id);

@implementation DynmenuController
-(void)updateCallbackActor:(void *)new_actor
{
    _actor = new_actor;
}
-(void)checkRadioButton:(NSInteger)control_id first:(NSInteger)first last:(NSInteger)last
{
    for (auto lyv : lights) {
        int tag = (int)lyv.tag;
        if (tag == control_id) {
            [lyv setIsOn:true];
        } else if (tag >= first && tag <= last) {
            [lyv setIsOn:false];
        }
        [lyv setNeedsDisplay:TRUE];
    }
}

-(HWND)GetControlHWND:(NSInteger)control_id
{
    if (CtlidToHWND.count(control_id) == 0)
        CtlidToHWND[control_id] = WinWrapControl(self, nil, control_id, @"Dynamic menu");
    return CtlidToHWND[control_id];
}
-(NSInteger)GetControlText:(NSInteger)control_id buf:(char*)buf length:(NSInteger)len
{
    return 0;
}
-(IBAction)NXMenuSelectAction:(id)sender
{
    NSControl * ctl = (NSControl*)sender;
    DynmenuButtonPushCallback(_actor, (int)[ctl tag]);
}
-(void)SetControlText:(NSInteger)control_id text:(NSString*)text
{
    NSView* view = self.window.contentView;
    NSRect viewFrame = [view frame];
    NSTextField * textField = [[NSTextField alloc] init];
    [textField setTextColor:NSColor.blackColor];
    [textField setBackgroundColor:NSColor.whiteColor];
    [textField setDrawsBackground:true];  /* doesn't seem to work */
    [view addSubview:textField];
    [textField setStringValue:text];
    [textField setTag:control_id];
    [textField setEditable:FALSE];
    NSRect frame = [textField frame];
    int yfirst = viewFrame.size.height - (control_id - 1000 + 1) * DYNMENU_ROW_HEIGHT;
    int y = yfirst;
    frame.origin.y = y;
    frame.origin.x = 30;
    frame.size.width=120;
    frame.size.height=20;
    [textField setFrame:frame];
    NSButton * button = [[NSButton alloc] init];
    [button setTitle:@""];
    if (!![text compare:@"Unidentified"]) {
        [view addSubview:button];
    }
    frame = [button frame];
    frame.origin.y = y + 3;
    frame.origin.x = 7;
    frame.size.width = 15;
    frame.size.height = 15;
    [button setFrame:frame];
    [[button cell] setBackgroundColor:NSColor.redColor];
    // [button setBezelStyle:NSCircularBezelStyle];  /* makes color unusable */
    [button setTarget:self];
    [button setAction:@selector(NXMenuSelectAction:)];
    [button setTag:control_id];
    frame.origin.x = 154;
    frame.origin.y += 2;
    frame.size.width = 10;
    frame.size.height = 10;
    LittleYellowView * lyv = [[LittleYellowView alloc] initWithFrame:frame];
    lights.push_back(lyv);
    [lyv setTag:control_id];
    if (!![text compare:@"Cancel"]) {
        [view addSubview:lyv];
    }
    [lyv setNeedsDisplay:TRUE];
   
}
-(int)kludgeParam2:(NSInteger)param1 param2:(NSInteger)param2
{
    NSInteger height = param1 * DYNMENU_ROW_HEIGHT + 20;
    NSRect wrect = [self.window frame];
    wrect.size.height = height + 27;  //title bar height.  We have to do better here for nonclient ovhd
    [self.window setFrame:wrect display:NO];

    NSView* view = self.window.contentView;
    NSRect frect = [view frame];
    frect.size.height = height;
    [view setFrame:frect];

    return 0;
}

-(void)DestroyWindow
{
    [self.window close]; // apparently the controller can provably go away just fine and the window stays up!
   // printf("dynmenu DestroyWindow really called\n");
   CtlidToHWND.clear();
}

@end


/* 10/14/2014 -- no more stl array of strongptrs -- make our master hwnd strong, and let c++ manage it
   and call DestroyWindow as the author intended.  Enable dealloc's printf for proof. */

//http://objectmix.com/c/177994-how-create-windows-without-nib.html

void * MacCreateDynmenu (void * actor) {  // pass dims, pos, responder as parameters from Windows
    
    NSRect frame;
    frame.origin.x = 200;
    frame.origin.y = 300;
    frame.size.width = 170;
    frame.size.height = 180;
    
#if 0 // code from above URL for manual-create gymnastics
    NSRect contentRect = [[NSScreen mainScreen] frame];
    /* allocating and initializing NSOutlineView, NSClipView and NSScrollView */
    NSOutlineView *outlineView = [[NSOutlineView alloc] initWithFrame:frame];
    NSClipView *clipView = [[NSClipView alloc] initWithFrame:contentRect];
    NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:contentRect];
    /* setting outlineView as documentView of clipView and clipView as contentView of scrollView */
    [clipView setDocumentView:outlineView];
    [scrollView setContentView:clipView];
    unsigned int styleMask = NSTitledWindowMask | NSClosableWindowMask | NSBorderlessWindowMask;
#endif

    unsigned int styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskBorderless;
    NSWindow * window = [[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:styleMask
                                                      backing:NSBackingStoreBuffered
                                                        defer: NO];
    [window setBackgroundColor:colorFromRGB(180,180,180)];
 
    DynmenuController * dynmenu = [[DynmenuController alloc] initWithWindow:window];
    dynmenu.actor = actor;
    static int  n = 0;
    HWND hWnd = WinWrapRetainController(dynmenu, window.contentView, [[NSString alloc] initWithFormat:@"Dynamic Menu #%d", n++]);
    WrapSetChildOfMain(hWnd);
    return hWnd;
}
