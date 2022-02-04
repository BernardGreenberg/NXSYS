//
//  RubberBandView.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/21/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface RubberBandView : NSView
-(void)setBeginning:(NSPoint)p;
-(void)setEnd:(NSPoint)p highlighted:(BOOL)highlighted;
-(void)close;
@end