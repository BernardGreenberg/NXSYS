//
//  RelayDrafterView.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/25/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "AppDelegate.h"
#include "WinViewUtils.h"
#include "WinMacCalls.h"

void RenderRelayPage(struct __DC_*);

@interface RelayDrafterView : NSView
@end


@implementation RelayDrafterView

- (BOOL) isFlipped
{
    return TRUE;
}
- (BOOL)acceptsFirstMouse:(NSEvent*)theEvent
{
    return TRUE;
}

- (void)mouseDown:(NSEvent *)theEvent
{
    NSPoint mpt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    RelayGraphicsLeftClick(mpt.x, mpt.y);
}


- (void)rightMouseDown:(NSEvent *)theEvent
{
    id controller = self.window.windowController;
    assert([controller isKindOfClass:[RelayDrafterController class]]);
    NSPoint mpt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    NSString * source_string;
    source_string = nullptr;
    if ([controller isSourceLocationAvailable])
        if (auto r = RelayGraphicsNameFromXY(mpt.x, mpt.y))
            source_string = [NSString stringWithUTF8String:r];

    [controller setSourceString: source_string];
    
    NSMenu *theMenu = [[NSMenu alloc] initWithTitle:@"Drafter"];
    NSMenuItem * m1 = nil;
        m1 = [theMenu insertItemWithTitle:@"Close"                                                       action:@selector(closeCmd:) keyEquivalent:@"" atIndex: 0];
    [m1 setTarget:controller];
    m1 = [theMenu insertItemWithTitle:@"Clear"                                                       action:@selector(clear:) keyEquivalent:@"" atIndex: 1];
    [m1 setTarget:controller];
    m1 = [theMenu insertItemWithTitle:@"Recede"                                                       action:@selector(recede:) keyEquivalent:@"" atIndex: 2];
    [m1 setTarget:controller];
    m1 = [theMenu insertItemWithTitle:@"New Relay"                                                       action:@selector(drawNew:) keyEquivalent:@"" atIndex: 3];
    [m1 setTarget:controller];
    if (source_string != nullptr) {
        m1 = [theMenu insertItemWithTitle:[NSString stringWithFormat:@"Source %@", source_string]                                 action:@selector(sourceCmd:) keyEquivalent:@""
                            atIndex: 4];
        [m1 setTarget:controller];
    }

    
    [NSMenu popUpContextMenu:theMenu withEvent:theEvent forView:self];
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    [[NSColor whiteColor] setFill];
    [[NSBezierPath bezierPathWithRoundedRect:[self bounds] xRadius:0.0 yRadius:0.0] fill];

    struct __DC_* hDC = GetDC_ ();
    SelectObject(hDC, GetStockObject(BLACK_PEN));
    SetTextColor(hDC, 0x000000);
    SetBkMode(hDC, TRANSPARENT);
    RenderRelayPage(hDC);
    
    ReleaseDC_(NULL, hDC);
}

-(void)MakeScrollVisible:(int)scrollpos
{
    NSPoint p = NSMakePoint(0, scrollpos);
    [self scrollPoint:p];
}
@end

void DrafterMakeScrollVisible (void* hwnd, int x, int  y, int h) {
    NSView* nsview = getHWNDView(hwnd);
    RelayDrafterView * view = (RelayDrafterView*)nsview;
    
    [view MakeScrollVisible:y];
}

void DraftViewScroll(NSView* vv, int pos) {
    RelayDrafterView* rdv = (RelayDrafterView*) vv;
    [rdv MakeScrollVisible:pos];
}
