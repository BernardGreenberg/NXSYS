//
//  BigYellowXView (formerly, bogusly RedXView)
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/13/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

// King of Simplicity of a transparent Cocoa (sub)view.  No static-pointers, no cleanup handlers, no knowledge
// of who owns me.

#import "BigYellowXView.h"

#include "WinViewUtils.h" //see MainView

typedef void* HPEN;
extern HPEN YellowXPen; //ok, so it's yellow

@implementation BigYellowXView

- (id)init
{
    NSRect frame = NSMakeRect(0, 0, 50, 50);
    self = [super initWithFrame:frame];
    [self setHidden:TRUE]; //lest it try to draw before pens are set up, let alone before wanted.
    // The perils of adoption as a newborn.
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    int s = self.frame.size.width;
    __DC_ * hDC = GetDC_ ();
    // These Windows calls are a lot easier than Cocommerce with Bezier.
    SelectObject(hDC, YellowXPen);
    SetBkMode(hDC, TRANSPARENT);
    MoveTo(hDC, 0, 0);
    LineTo(hDC, s, s);
    MoveTo(hDC, 0, s);
    LineTo(hDC, s, 0);
    ReleaseDC_(NULL, hDC);
}
-(void)Expose:(NSPoint) p
{
    NSRect frame = [self frame];
    frame.origin = p;
    frame.origin.x -= frame.size.width/2;
    frame.origin.y -= frame.size.height/2;
    [self setFrame:frame];
    [self setHidden:FALSE]; // apparently causes dirtification just fine.
}
-(void)deExpose
{
    [self setHidden:TRUE];
}
@end


