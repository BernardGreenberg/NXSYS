//
//  CustomAboutController.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 1/28/21.
//  Copyright © 2021 BernardGreenberg. All rights reserved.
//

#import "CustomAboutController.h"
#import "AppBuildSignature.h"

static NSString* stdNS(const std::string s) {
    return [NSString stringWithUTF8String:(s.c_str())];
}

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

    [_labelAppName setStringValue:stdNS(ABS.ApplicationName)];
    [_labelLine1 setStringValue:stdNS(ABS.VersionString())];
    [_labelLine2 setStringValue:stdNS(ABS.BuildString())];
    self.window.title = stdNS("About " + ABS.ApplicationName);

    NSString* nssAboutURL = stdNS("About" + ABS.ApplicationName);
    NSURL * url = [[NSBundle mainBundle] URLForResource:nssAboutURL withExtension:@".html"];
    [self.theWebView loadRequest:[NSURLRequest requestWithURL:url]];

    auto nssImageURL = stdNS(ABS.ApplicationName + "256x256");
    auto urlimg =  [[NSBundle mainBundle] URLForResource:nssImageURL withExtension:@".png"];
    [_theImageView setImage:[[NSImage alloc] initByReferencingURL: urlimg]];
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
