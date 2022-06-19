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

    self.labelAppName.stringValue = stdNS(ABS.ApplicationName);
    self.labelVersion.stringValue = stdNS("Version " + ABS.VersionString());
    self.labelBuild.stringValue = stdNS(ABS.BuildString());
    self.window.title = stdNS("About " + ABS.ApplicationName);

    auto AboutTextURL = [[NSBundle mainBundle]
                         URLForResource: stdNS("About" + ABS.ApplicationName)
                         withExtension: @".html"];
    [self.theWebView loadRequest:[NSURLRequest requestWithURL:AboutTextURL]];

    auto ImageURL =  [[NSBundle mainBundle]
                      URLForResource: stdNS(ABS.ApplicationName + "256x256")
                      withExtension: @".png"];
    self.theImageView.image = [[NSImage alloc] initByReferencingURL: ImageURL];
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
