//
//  NXMacView.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/14/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#ifndef TLEDIT
#import "BigYellowXView.h"
#endif

@interface MainView : NSView
@property (weak) NSScrollView *theScrollView;
-(void)magnifyWithRatio:(CGFloat)r;
#ifndef TLEDIT
@property (strong) BigYellowXView* YellowX;
#endif
@end

int ContextMenuHandle(NSMenuItem*item);
