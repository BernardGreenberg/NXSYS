//
//  MainView, erstwhile NXSYSView, erstwhile NXMacView.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/14/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import "MainView.h"
#import "WinViewUtils.h"
#include "WinMacCalls.h"

#define WM_USER 2000
#define WM_NXGO_LBUTTONSHIFT (WM_USER+1)
#define WM_NXGO_LBUTTONCONTROL (WM_USER+2)
#define WM_NXGO_RBUTTONCONTROL (WM_USER+3)

#define RODOTRACE(x) 
//printf x

bool REDISPLAYING = false;
@implementation MainView

   //[NSApplication sharedApplication]mainWindow]setContentView:[self view]]

- (BOOL)isFlipped
{
    return TRUE;
}

-(BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    /* Esto nobis praegustatum . . . mew */
    return YES;
}

-(void)magnifyWithRatio:(CGFloat)r
{
    CGFloat curmag =[_theScrollView magnification];
    CGFloat newmag = r*curmag;
    [_theScrollView setMagnification: newmag];
}
-(void)magnifyWithEvent:(NSEvent *)event  //meow Meow MEOW anima mea D~m.
{
    CGFloat rawmag = [event magnification];
    CGFloat m = rawmag + 1.0;
    [self magnifyWithRatio:m];
}
- (void)rodaCommon:(NSEvent*) theEvent Message:(int)message
{
    if (message == WM_LBUTTONDOWN) {
        if ([theEvent modifierFlags] & NSEventModifierFlagShift) {
            message = WM_NXGO_LBUTTONSHIFT;
        }
        if ([theEvent modifierFlags] & NSEventModifierFlagControl) {
            message = WM_NXGO_LBUTTONCONTROL;
        }
    } else if (message == WM_RBUTTONDOWN) {
        if ([theEvent modifierFlags] & NSEventModifierFlagControl) {
            message = WM_NXGO_RBUTTONCONTROL;
        }
    }
    
    NSPoint location = [theEvent locationInWindow];
    NSPoint mpt = [self convertPoint:location fromView:nil]; //from "window" coords to NXview
    NXGO_Rodentate((int)mpt.x, (int)mpt.y, message);
}

- (void)mouseDown:(NSEvent *)theEvent
{
    [self rodaCommon:theEvent Message:WM_LBUTTONDOWN];
}

- (void)mouseUp:(NSEvent*)theEvent
{
    NXGOMouseUp();
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
    [self rodaCommon:theEvent Message:WM_RBUTTONDOWN];
}

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
        GDISetMainWindow(self.window, self);
#ifndef TLEDIT
        _YellowX = [[BigYellowXView alloc] init];
        [self addSubview:_YellowX];
#endif
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];

    // Don't need this any more -- background color set on ScrollView does it (NSRectFill is ok too)
  //  [[NSColor blackColor] setFill];
 //   [[NSBezierPath bezierPathWithRoundedRect:[self bounds] xRadius:3.0 yRadius:3.0] fill];

    REDISPLAYING = true;
    HDC hDC = GetDC_ ();
    SetTextColor(hDC, 0xFFFFFF);
    SetBkMode(hDC, TRANSPARENT);
    RECT winRect;
    winRect.top = (int)dirtyRect.origin.y;
    winRect.left = (int)dirtyRect.origin.x;
    winRect.right = (int)(dirtyRect.origin.x + dirtyRect.size.width);
    winRect.bottom = (int)(dirtyRect.origin.y + dirtyRect.size.height);
    
    DisplayVisibleObjectsRect(hDC, winRect);
    
    ReleaseDC_(NULL, hDC);
    REDISPLAYING = false;
#ifndef TLEDIT
    InvalidateRelayDrafter();
#endif
}
@end
#ifndef TLEDIT

MainView* getMainView();
void ShowBigYellowX(int x, int y) {
    [getMainView().YellowX Expose:NSMakePoint(x,y)];
}
void UnShowBigYellowX() {
    [getMainView().YellowX deExpose];
}
#endif
