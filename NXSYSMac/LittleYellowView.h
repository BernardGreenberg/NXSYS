//
//  LittleYellowView.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/8/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NSColor *colorFromRGB(unsigned char r, unsigned char g, unsigned char b);

@interface LittleYellowView : NSView
@property NSColor* onColor;
@property NSColor* offColor; // as it were
@property bool isOn;
@property NSInteger tag;
@end
