//
//  RelayDrafterController.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/25/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import "RelayDrafterController.h"
#import "AppDelegate.h"

#include "WinMacCalls.h"

void ClearRelayGraphics();
void AskForAndDrawRelay(void*);

static NSString* SourceLocatorScriptKey = @"SourceLocatorScript";
@interface RelayDrafterController ()
{
    bool first;
}

@end

@implementation RelayDrafterController

NSString* sourceLocatorScript = nullptr;
NSString* savedSourceString = nullptr;
-(void)windowWillClose:(NSNotification*)notification
{
    [_mainMenuItem setState:NSOffState];
}

- (id) init {
    savedSourceString = nullptr;
    NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
    sourceLocatorScript = [defaults stringForKey:SourceLocatorScriptKey];

    if ( ! (self = [super initWithWindowNibName: @"RelayDrafter"]) )
    {
        NSLog(@"init failed for Relay Drafter");
    }
    return self;
}
-(IBAction)closeCmd:(id)sender
{
    [self.window close];
}
-(bool)isSourceLocationAvailable {
    return sourceLocatorScript != nullptr && sourceLocatorScript != nil;
}
-(void)setSourceString:(NSString*) source_string {
    savedSourceString = source_string;
}


-(IBAction)sourceCmd:(id)sender
{
    NSString* cmdline = [NSString stringWithFormat:@"%@ \"%@\"", sourceLocatorScript, savedSourceString];
    system([cmdline UTF8String]);
}
-(IBAction)clear:(id)sender
{
    ClearRelayGraphics();
    [_theView setNeedsDisplay:YES];
}
-(IBAction)drawNew:(id)sender
{
    AskForAndDrawRelay(NULL);
    if ([self.window isVisible])
        [_mainMenuItem setState:NSOnState];
}
-(IBAction)recede:(id)sender
{
    if (self.window.parentWindow)
        [self.window.parentWindow removeChildWindow:self.window];
    [getNXWindow() orderFront:nil];
    
}
- (IBAction)setLocatorPath:(id)sender {
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    
    // Enable the selection of files, disable directories in the dialog.
    [openDlg setCanChooseFiles:YES];
    [openDlg setCanChooseDirectories:NO];
    [openDlg setAllowedFileTypes: @[ @"sh" ]];
    [openDlg setTitle:@"NXSYS - Set source locator script pathname"];

    // Display the dialog.  "If OK", process the file.
    if ( [openDlg runModal] == NSModalResponseOK )
    {
        [openDlg close];
        NSURL* url = [openDlg URL];
        openDlg = nil;
        NSUserDefaults * dfts = [NSUserDefaults standardUserDefaults];
        NSString * path = url.path;
        [dfts setValue:path forKey:SourceLocatorScriptKey];
        sourceLocatorScript = path;  // allow use now!
    }
}
- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    return self;
}

- (void)windowDidLoad
{
    [super windowDidLoad]; // control iboutlets are really set now.

    AppDelegate * apdel = getNXAppDelegate();
    _mainMenuItem = apdel.DraftspersonItem;
    [[apdel window] addChildWindow:self.window ordered:NSWindowAbove];
     /* we don't have to set self as delegate -- dragging the outlet hook to "file owner: in the IB
      does it "silently", although [self.window setDelegate:self] is a lot more helpful. */
    /* This must be weak because we keep it as a property, or, more importantly,
     the AppDelegate is the official owner of the official strongptr to the Draftsperson, not
     C++ Windows code. */
    _hWnd = WinWrapNoRetain(self, self.theView, @"Relay Draftsperson");
    WrapSetChildOfMain(_hWnd);
}

-(IBAction)toggle:(id)sender
{
    if (![self isWindowLoaded]) {// isVisible seems to lie in this case, but see FSWindow.
        [self showWindow:sender];
        [_mainMenuItem setState:NSOnState];
    }
    else if ([self.window isVisible])
        [self close];
    else {
        [self showWindow:self];
        [_mainMenuItem setState:NSOnState];
    }
}
@end


