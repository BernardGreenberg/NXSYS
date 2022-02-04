//
//  CustomAboutController.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 1/28/21.
//  Copyright © 2021 BernardGreenberg. All rights reserved.
//

#import "CustomAboutController.h"
#import "AppDelegate.h"

@implementation CustomAboutController
static NSString* label1String;
static NSString* label2String;

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
    return self;
}
-(void)SetVersionData:(NSString *)version date:(NSString *)date build_number:(NSString*)build_number {
    label1String = version;
#if DEBUG
    label2String = [NSString stringWithFormat:@"Debug build %@ %@", build_number, date];
#else
    label2String = [NSString stringWithFormat:@"Build %@ %@", build_number, date];
#endif
}
- (void)windowDidLoad
{
    [super windowDidLoad];

    [_labelLine1 setStringValue:label1String];
    [_labelLine2 setStringValue:label2String];
    NSURL * url = [[NSBundle mainBundle] URLForResource:@"About" withExtension:@".html"];
    auto urlrq = [NSURLRequest requestWithURL:url];
    [self.theWebView loadRequest:urlrq];
}
- (bool)isWindowVisible
{
    return [self.window isVisible];
}

-(void)Show
{
    [self showWindow:nil];
    [self.window makeKeyAndOrderFront:nil];
    [getNXWindow() addChildWindow:self.window ordered:NSWindowAbove];
}


@end
