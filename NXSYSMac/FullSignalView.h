//
//  FullSignalView.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/23/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface FullSignalView : NSView;
-(void)fillPlate:(NSRect) rect;
+(void*)CreateFloatingViewFromRectHWND:(NSRect)r;
@property void* pSig;
@property bool floating;

@end
