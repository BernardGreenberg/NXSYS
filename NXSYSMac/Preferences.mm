//
//  Preferences.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 10/3/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import "Preferences.h"
#include "ssdlg.h"

static NSString * FullSigKey = @"FullSignalDisplaysAreViews";
static NSString * ShowStopsKey = @"LastShowStopsPolicy";
static NSString * SignalRightMenuKey = @"SignalRightClickIsMenu";

extern bool FullSigsAreViews;
extern bool SignalRIsMenu;

@interface Preferences ()
{
    NSUserDefaults * defaults;
}
@property (weak) IBOutlet NSButtonCell* fullSigsWindows;
@property (weak) IBOutlet NSButtonCell* fullSigsViews;
@property (weak) IBOutlet NSButtonCell* rightClickFSDs;
@property (weak) IBOutlet NSButtonCell* rightClickMenus;
@property (weak) IBOutlet NSButtonCell* stopsNever;
@property (weak) IBOutlet NSButtonCell* stopsTripping;
@property (weak) IBOutlet NSButtonCell* stopsAlways;
@end

@implementation Preferences
+(void)initPreferredSettings
{
    NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
    
    bool rightClickIsMenu = [defaults boolForKey:SignalRightMenuKey] ? true: false;
    bool fullSigsAreViews = [defaults boolForKey:FullSigKey] ? true : false;
    int stop_policy = (int)[defaults integerForKey:ShowStopsKey];
    if (stop_policy == 0) {
        stop_policy = SHOW_STOPS_RED;
    }
   
    ImplementShowStopPolicy(stop_policy);
    FullSigsAreViews = fullSigsAreViews;
    SignalRIsMenu = rightClickIsMenu;
    
}
-(Preferences*)init
{
    return (Preferences*)[self initWithWindowNibName:@"Preferences"];
}
- (void)windowDidLoad {
    [super windowDidLoad];
    
    defaults = [NSUserDefaults standardUserDefaults];
    NSInteger ssp = [defaults integerForKey:ShowStopsKey];
    if (ssp == 0) {
        ssp = SHOW_STOPS_RED;
    }

    bool rightClickIsMenu = [defaults boolForKey:SignalRightMenuKey] ? true: false;
    bool fullSigsAreViews = [defaults boolForKey:FullSigKey] ? true : false;

    [((NSButton*)_stopsTripping) setState: (ssp == SHOW_STOPS_RED) ? NSControlStateValueOn : NSControlStateValueOff];
    [((NSButton*)_stopsNever) setState: (ssp == SHOW_STOPS_NEVER) ? NSControlStateValueOn : NSControlStateValueOff];
    [((NSButton*)_stopsAlways) setState: (ssp == SHOW_STOPS_ALWAYS) ? NSControlStateValueOn : NSControlStateValueOff];

    [(NSButton*)_fullSigsViews setState: fullSigsAreViews? NSControlStateValueOn: NSControlStateValueOff];
    [(NSButton*)_fullSigsWindows setState: fullSigsAreViews? NSControlStateValueOff: NSControlStateValueOn];

    [(NSButton*)_rightClickMenus setState: rightClickIsMenu? NSControlStateValueOn: NSControlStateValueOff];
    [(NSButton*)_rightClickFSDs setState: rightClickIsMenu ? NSControlStateValueOff: NSControlStateValueOn];

}
-(void)showModal
{
    [NSApp runModalForWindow:[self window]];
}
-(IBAction)Cancel:(id)sender
{
    [[self window] close];
    [NSApp stopModal];
  
}
-(IBAction)OK:(id)sender
{
    int nsp = 0;
    if ([(NSButton*)_stopsTripping state] == NSControlStateValueOn)
        nsp = SHOW_STOPS_RED;
    if ([(NSButton*)_stopsAlways state] == NSControlStateValueOn)
        nsp = SHOW_STOPS_ALWAYS;
    if ([(NSButton*)_stopsNever state] == NSControlStateValueOn)
        nsp = SHOW_STOPS_NEVER;
    if (nsp != 0) {
        [defaults  setInteger:nsp forKey:ShowStopsKey];
        ImplementShowStopPolicy(nsp);
    }
    
    FullSigsAreViews = ([(NSButton*)_fullSigsViews state] == NSControlStateValueOn);
    [defaults setBool:(FullSigsAreViews ? YES : NO) forKey:FullSigKey];

    SignalRIsMenu = ([(NSButton*)_rightClickMenus state] == NSControlStateValueOn);
    [defaults setBool:(SignalRIsMenu ? YES : NO) forKey:SignalRightMenuKey];
    [[self window] close];
    [NSApp stopModal];
}

-(void)windowWillClose:(NSNotification*)notification
{
    [NSApp stopModal];
}

@end
