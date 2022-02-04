//
//  RubberBandView.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/21/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import "RubberBandView.h"

#include "WinViewUtils.h" //see MainView

static NSPoint pdif(NSPoint p1, NSPoint p2) {
    return NSMakePoint(p1.x - p2.x, p1.y - p2.y);
}
static NSPoint psum(NSPoint p1, NSPoint p2) {
    return NSMakePoint(p1.x + p2.x, p1.y + p2.y);
}
static NSPoint vectimes(NSPoint p1, CGFloat r) {
    return NSMakePoint(p1.x*r, p1.y*r);
}

@interface RubberBandView ()
{
    NSColor * color;
    NSColor * hlColor;
    NSPoint lineOrigin;
    NSPoint inFrameOrigin;
    NSPoint inFrameEnd;
    BOOL highlighted;

}
@end

@implementation RubberBandView


-(BOOL)isFlipped
{
    return YES;  // same coordinates as main view, crazifying if not.
}
- (id)init
{
    NSRect frame = NSMakeRect(0, 0, 50, 50);
    self = [super initWithFrame:frame];
    color = [NSColor colorWithCalibratedRed:(128.0/255.0f)
                                        green:(255.0/255.0f)
                                        blue:(255.0/255.0f)
                                       alpha:.7];

    hlColor = [NSColor colorWithCalibratedRed:(64.0/255.0f)
                                       green:(255.0/255.0f)
                                        blue:(255.0/255.0f)
                                       alpha:.9];
    [self setHidden:TRUE];
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];

    NSBezierPath * bezier = [NSBezierPath bezierPath];
    
    [bezier setLineWidth: (highlighted ? 5 : 3)];
    [bezier moveToPoint:inFrameOrigin];
    [bezier lineToPoint:inFrameEnd];
    [(highlighted ? hlColor : color) set];
    [bezier stroke];
}

-(void)setEnd:(NSPoint)p highlighted:(BOOL)ahighlighted
{
    highlighted = ahighlighted;
    NSRect frame = [self frame];
    NSPoint doubledVec = vectimes(pdif(p, lineOrigin), 2.0);
    frame.size.width = fmax (fabs(doubledVec.x), 10.0);
    frame.size.height = fmax(fabs(doubledVec.y), 10.0);
    inFrameOrigin = NSMakePoint(frame.size.width/2.0, frame.size.height/2.0);
    inFrameEnd = psum(pdif(p, lineOrigin), inFrameOrigin);
    frame.origin = pdif(lineOrigin, inFrameOrigin);
    [self setFrame:frame];
    [self setHidden:FALSE];
}
-(void)setBeginning:(NSPoint)p
{
    lineOrigin = p;
}
-(void)close
{
    [self setHidden:TRUE];
}
@end

