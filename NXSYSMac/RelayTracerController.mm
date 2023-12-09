//
//  NXRelayWindowController.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/22/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import "RelayTracerController.h"
#import "AppDelegate.h"
typedef void* HINSTANCE;
#include "rlytrapi.h"

static void ConsoleRelayTracer(const char* name, int state) {
    printf("%-8s  %s\n", name, state ? "PICK" : "DROP");
}

static void WindowRelayTracer(const char* name, int state) {
    [getNXAppDelegate().rlyTraceWindow
     log:[[NSString alloc] initWithFormat:@"%-8s  %s\n", name, state ? "PICK" : "DROP"]];
}

@interface RelayTracerController ()
{
    int lines;
    BOOL moron;
    bool more_stuck;

    __strong NSDictionary * theFontDictionary;
    NSAttributedString* MORE;
}
@property (weak) IBOutlet NSButtonCell *moreButton;
@property (weak) IBOutlet NSButtonCell *moreingButton;
@end

@implementation RelayTracerController

/*These ought be static if they're reflected in the global menu. */

bool to_console = false;
bool to_window = false;

- (id) init {
    if ( ! (self = [super initWithWindowNibName: @"RelayTrace"]) )
    {
        NSLog(@"init failed for RelayTrace");
        return nil;
    }
    lines = 0;
    moron = false;
    more_stuck= false;
    to_console = false;
    to_window = false;
    MORE = [[NSAttributedString alloc] initWithString:@"**MORE** (any key)"];
    return self;

} // end init

+ (RelayTracerController*) CreateWithMenu:(NSMenu*)mainMenu
{
    RelayTracerController * r = [[RelayTracerController alloc] init];
    if (r != nil) {
        long indexOfRelays = [mainMenu  indexOfItemWithTitle:@"Relays"];
        NSMenu* relayMenu  = [[mainMenu itemAtIndex:indexOfRelays] submenu];

        long indexOfWItem  = [relayMenu indexOfItemWithTitle:@"Trace to window"];
        r.windowMenuItem   = [relayMenu itemAtIndex:indexOfWItem];

        long indexOfCItem  = [relayMenu indexOfItemWithTitle:@"Trace to console"];
        r.consoleMenuItem  = [relayMenu itemAtIndex:indexOfCItem];
    }
    return r;
}

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {
        NSFont * font = [NSFont fontWithName:@"Courier New" size:12];
        id hfdObjects[] = {font, NSColor.textColor};
        id hfdKeys[] = {NSFontAttributeName, NSForegroundColorAttributeName};
        NSUInteger hfdCount = sizeof(hfdObjects)/sizeof(id);
        theFontDictionary = [NSDictionary dictionaryWithObjects:hfdObjects forKeys:hfdKeys count:hfdCount];

        // Control outlets NOT SET at this point!!!
    }
    return self;
}
- (void)windowDidLoad
{
    [super windowDidLoad];
    [self.window setDelegate:self];
    [getNXWindow() addChildWindow:self.window ordered:NSWindowAbove];
}

- (void)log:(NSString*) string
{
    lines += 1;
    NSTextStorage * ts = _textView.textStorage;
    if (moron && ((lines % 40) == 0)) {
        long len_before = ts.length;
        [ts appendAttributedString:MORE];
        more_stuck = true;
        [_textView scrollRangeToVisible:NSMakeRange(ts.length, 0)];
        [_moreButton.controlView setHidden:NO];
        [NSApp runModalForWindow:self.window];
        if (more_stuck) // can happen if close while stuck!!!
            [self coherentOff];
        more_stuck=false;
        [ts deleteCharactersInRange:NSMakeRange(len_before, MORE.length)];
    }
   // http://stackoverflow.com/questions/15172971/append-to-nstextview-and-scroll
   
    NSAttributedString* attr = [[NSAttributedString alloc]
                                initWithString:string
                                attributes:theFontDictionary];
    [ts appendAttributedString:attr];

    [_textView scrollRangeToVisible:NSMakeRange(ts.length, 0)];
}
-(bool)TryCharTrap:(NSInteger)ch
{

    if (more_stuck) {
        [self MoreButton:nil];
        return true;
    }
    return false;
}
- (IBAction)Clear:(id)sender {
    if (more_stuck) {
        [self MoreButton:nil];
    }
    NSTextStorage * ts = _textView.textStorage;
    [ts deleteCharactersInRange:NSMakeRange(0, ts.length)];
    lines = 0;
}
- (IBAction)MoreIngButton:(id)sender {
    if (more_stuck) {
        [self MoreButton:nil];
    }
    
    NSButton * b = (NSButton*)sender;
    moron = b.state;
    if (moron)
        lines = 0;
}
- (IBAction)MoreButton:(id)sender {
    if (more_stuck) {
        [NSApp stopModal];
        more_stuck = false; // see the morer
        [_moreButton.controlView setHidden:YES];
    }
}
-(void) ClickTraceToWindow
{
    if (!to_window) {
        to_console = false;
        [_consoleMenuItem setState: NSControlStateValueOff];

        [_windowMenuItem setState: NSControlStateValueOn];
        to_window = true;
        [self showWindow:self];
        NSWindow * mw = getNXWindow();
        [mw addChildWindow:self.window ordered:NSWindowAbove];
        [mw makeKeyWindow];
        SetRelayTrace(WindowRelayTracer);
    } else {
        [self coherentOff];
    }
}
-(void) ClickTraceToConsole
{
    if (!to_console) {
        [self coherentOff];
        [self.window orderOut:self];
        
        to_console = true;
        [_consoleMenuItem setState: NSControlStateValueOn];
        SetRelayTrace(ConsoleRelayTracer);
    } else {
        [_consoleMenuItem setState: NSControlStateValueOff];
        SetRelayTrace(NULL);
        to_console = FALSE;
    }
}

-(void)coherentOff
{
    if (more_stuck) {
        //If app is stuck in infinite loop, which we do not know, we are loosing trappage.
        // But we cannot know.
        [self MoreButton:nil];
    }
    moron = false;
    [_moreingButton setState:FALSE];
    [_windowMenuItem setState: NSControlStateValueOff]; // main wnd menu
    SetRelayTrace(NULL);
    to_window = FALSE;
}
-(void)windowWillClose:(NSNotification *)notification
{
    [self coherentOff];
}
@end
