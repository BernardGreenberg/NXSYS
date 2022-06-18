//
//  CustomAboutController.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 1/28/21.
//  Copyright © 2021 BernardGreenberg. All rights reserved.
//

#import "CustomAboutController.h"
#include "AppBuildSignature.h"

@implementation CustomAboutController
static AppBuildSignature ABS;

- (id) init {
    if ( ! (self = [super initWithWindowNibName: @"CustomAbout"]) )
    {
        NSLog(@"init failed for CustomAbout");
        return nil;
    }
    return self;
    
} // end init

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    ABS.Populate();
    return self;
}
- (void)windowDidLoad
{
    [super windowDidLoad];

    [_labelAppName setStringValue:[NSString stringWithUTF8String:ABS.ApplicationName.c_str()]];
    [_labelLine1 setStringValue:[NSString stringWithUTF8String:ABS.VersionString().c_str()]];
    [_labelLine2 setStringValue:[NSString stringWithUTF8String:ABS.BuildString().c_str()]];
    std::string AboutURL = "About" + ABS.ApplicationName;
    NSString* nssAboutURL = [NSString stringWithUTF8String:AboutURL.c_str()];
    NSURL * url = [[NSBundle mainBundle] URLForResource:nssAboutURL withExtension:@".html"];
    auto urlrq = [NSURLRequest requestWithURL:url];
    [self.theWebView loadRequest:urlrq];
}
- (bool)isWindowVisible
{
    return [self.window isVisible];
}

-(void)Show: (NSWindow*)parent
{
    [self showWindow:nil];
    [self.window makeKeyAndOrderFront:nil];
    [parent addChildWindow:self.window ordered:NSWindowAbove];
}


@end
