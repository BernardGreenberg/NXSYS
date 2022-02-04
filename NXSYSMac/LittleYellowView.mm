//
//  LittleYellowView.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/8/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import "LittleYellowView.h"

@implementation LittleYellowView
@synthesize tag;
- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
        _onColor = colorFromRGB(255, 255, 196);
        _offColor = colorFromRGB(128, 128, 128);
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    NSRect rect = [self frame];
    rect.origin.x = 0;
    rect.origin.y = 0;
    NSBezierPath* thePath = [NSBezierPath bezierPath];
    [thePath appendBezierPathWithOvalInRect:rect];
    NSColor * c= _isOn ? _onColor : _offColor;
    [c set];
    [thePath fill];
}

@end
