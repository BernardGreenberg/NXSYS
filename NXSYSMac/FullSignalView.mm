//
//  FSWindowView.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/23/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import "FullSignalView.h"
#include "WinMacCalls.h"

#include "WinViewUtils.h"

NSView* getMainView();

void CallFSWinDisp(void*, __DC_*, int);

@interface FullSignalView ()
{
    NSPoint dragMouse;
    NSColor* plateColor;
}
@end

@implementation FullSignalView
-(void) dealloc

{
 //   printf("oh\n");
}

- (BOOL)isFlipped
{
    return YES;
}
-(BOOL)mouseDownCanMoveWindow
{
    return YES;
}

- (id)initWithFrame:(NSRect)frame
{
    _pSig = NULL;
    self = [super initWithFrame:frame];
    if (self) {
        _floating = false;
        plateColor = [NSColor colorWithCalibratedRed:.75 green:.75 blue:.75 alpha:.75];
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    if (_pSig == NULL)  // will redraw when object is created, seemingly.
        return;

    HDC hDC = GetDC_ ();
    SelectObject(hDC, GetStockObject(BLACK_PEN));
    SetTextColor(hDC, 0x000000);
    SetBkMode(hDC, TRANSPARENT);
    CallFSWinDisp(_pSig, hDC, 0);
    
    ReleaseDC_(NULL, hDC);
}

-(void)fillPlate:(NSRect)rect
{
    if (_floating) {
       NSBezierPath* thePath = [NSBezierPath bezierPath];
       [thePath appendBezierPathWithRect:rect];
       [plateColor set];
       [thePath fill];
    }
}

-(void)mouseDown:(NSEvent*)theEvent
{
    if (theEvent.clickCount == 2) {
        if (_floating) {
            [self setHidden:TRUE];
        } else {
            [[self window] orderOut:self];
        }
        return;
    }
    dragMouse = [theEvent locationInWindow];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    NSPoint location = [theEvent locationInWindow];
    if (_floating) {     //if not _floating, System seems to mousedrag for free.
        NSRect frame = [self frame];
        frame.origin.x += location.x - dragMouse.x;
        frame.origin.y -= location.y - dragMouse.y;
        [self setFrame:frame];
    }
    dragMouse = location;
}

+(void*)CreateFloatingViewFromRectHWND:(NSRect)r
{
    FullSignalView* view = [[FullSignalView alloc] initWithFrame:r];
    [view setFloating:true];
    [getMainView() addSubview:view];
    // must be strong, because we're gonna remove as subview when double-clicked and put back later.
    return WinWrapRetainView(view, @"Floating Fullsig");
}
@end

void MacFillPlateSexily(HWND hWnd, int x, int y, int w, int h) {
    NSView* view = getHWNDView(hWnd);
    assert ([view isKindOfClass:[FullSignalView class]]);
    [(FullSignalView*)view fillPlate:NSMakeRect(x,y,w,h)];
}

