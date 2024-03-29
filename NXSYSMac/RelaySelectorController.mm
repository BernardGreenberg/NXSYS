//
//  RelaySelectorController.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/29/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import "AppDelegate.h"
#include "WinMacCalls.h"
#include "MacRlyDlg.h"
#include "RelayListView.h"

@interface RelaySelectorController : NSWindowController<NSTableViewDelegate, NSWindowDelegate>
{
    Relay* chosenRelay;
 }
@property (weak) IBOutlet NSTextField *Label;
@property (weak) IBOutlet RelayListView *theRelayListView;
@property (weak) IBOutlet NSButton* okButton;
@end

extern unsigned NXGOHitX, NXGOHitY;

@implementation RelaySelectorController

- (id) init {
    return [super initWithWindowNibName: @"RelaySelector"];  // checking is worthless
}
- (void)windowDidLoad
{
    [super windowDidLoad];
    chosenRelay = NULL;
    assert(_theRelayListView != nil);
    [_theRelayListView setDoubleAction:@selector(UseItHit:)];
    [_theRelayListView setNomenclatureOnly:YES];
    [_theRelayListView setDelegate:self];
}
- (IBAction)UseItHit:(id)sender {
    chosenRelay = [_theRelayListView getSelectedRelay];
    [NSApp stopModal];
    [self.window close];
}
-(IBAction)tableViewSelectionDidChange:(NSNotification*)note
{
    [_okButton setEnabled:TRUE];
}
-(IBAction)RelayListViewSelect:(id) sender
{
    [_okButton setEnabled:TRUE];
}

- (IBAction)CancelHit:(id)sender
{
    [self.window performClose: self]; //caused WillClose to be invoked properly
}
-(void) windowWillClose:(NSNotification *)notification
{
    [NSApp stopModal];
}
-(void) position
{
    NSPoint p = NXViewToScreen(NSMakePoint(NXGOHitX, NXGOHitY));
    p.y -= self.window.frame.size.height/2;
    [self.window setFrameOrigin:p];
}

-(Relay*)run: (const RelayVec&)theRelays
 description:(NSString *)tag op:(NSString*)op
{
    //By the first days of October, 2014, Bernie had discovered how to force
    // windows to load their NIB so you could futz around with the content
    // and placement before exposure.

    // Mac programming in 5 lines
    // INIT store params from caller
    // LOAD NIB instantiate pieces and get pointers to them
    // STUFF CONTROLS with params from (1)
    // SHOW WINDOW
    // RUN MODAL. (can combine)
    
    // 2. LOAD NIB.
    [self window]; // loads nib, makes these properties meaningful

    // 3. STUFF STUFF
    [_theRelayListView setRelayContent:theRelays];
    [_Label setStringValue:tag];
    chosenRelay = NULL;
    [self.window setTitle: [NSString stringWithFormat:@"%@ relay", op]];
    [self position];
    [getNXWindow() addChildWindow:self.window ordered:NSWindowAbove];
    [_okButton setTitle:op];

    // 5. RUN MODAL
    [NSApp runModalForWindow:[self window]];
    return chosenRelay;
}
@end

Relay * RelayListDialog(int objNo, const char *typeName, const RelayVec& relays, const char * operation) {
    NSString * tag = [[NSString alloc] initWithFormat:@"%s %d", typeName, objNo];
    NSString * op = [[NSString alloc] initWithFormat:@"%s", operation];
    return [[[RelaySelectorController alloc] init] run: relays
                                           description: tag
                                                    op: op];
}
