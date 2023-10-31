//
//  RelayState.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 10/3/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import "RelayListView.h"
#include "MacRlyDlg.h"

@interface RelayState : NSWindowController<NSWindowDelegate>
{
    Relay * relay;
    std::vector<Relay*>dependents;
}

@property (weak) IBOutlet NSTextField* relayNameField;
@property (weak) IBOutlet NSTextField* relayStateField;
@property (weak) IBOutlet NSTextField* depCountField;
@property (weak) IBOutlet RelayListView * listView;
@end

@implementation RelayState
-(id)init
{
    self = [super initWithWindowNibName:@"RelayState"];
    return self;
}
- (void)windowDidLoad {
    [super windowDidLoad];
    
    assert(_listView != nil);
    [self.window setDelegate: self];
    [_listView setDoubleAction:@selector(ListViewDoubleClick:)];
}
-(void)setForRelay:(Relay*)r
{
    assert(_listView != nil);
    relay = r;

    vectorizeDependents(r, dependents);
    [_listView setRelayContent:dependents]; //WILL COPY IT!

    NSString* name = [[NSString alloc] initWithUTF8String:STLRelayName(r).c_str()];
    [_relayNameField setStringValue:name];
    
    [_depCountField setIntegerValue:dependents.size()];
    [_relayStateField setStringValue: (boolRelayState(r) ? @"PICKED" : @"DROPPED")];

}
-(void)ListViewDoubleClick:(id)sender
{
    Relay * r = [_listView getSelectedRelay];
    if (r != NULL)
        [self setForRelay:r];
}

-(IBAction)Draw:(id)sender
{
    DrawRelayAPI(relay);
}
-(IBAction)OK:(id)sender
{
    [self.window performClose: self];
}
-(void) windowWillClose:(NSNotification *)notification
{
    [NSApp stopModal];
}

-(void)showModal:(Relay*)r
{
    [self window];
    [self setForRelay:r];
    [NSApp runModalForWindow:self.window];
}

@end

void ShowStateRelay(Relay* relay) {
    [[[RelayState alloc] init] showModal:relay];
}
