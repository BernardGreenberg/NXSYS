//
//  CustomAboutController.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 1/28/21.
//  Copyright © 2021 BernardGreenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

@interface CustomAboutController : NSWindowController
-(void)Show: (NSWindow*)parent;
-(bool)isWindowVisible;
@property (weak) IBOutlet WKWebView *theWebView;
@property (weak) IBOutlet NSImageView *theImageView;
@property (weak) IBOutlet NSTextField *labelAppName;
@property (weak) IBOutlet NSTextField *labelLine1;
@property (weak) IBOutlet NSTextField *labelLine2;

@end


