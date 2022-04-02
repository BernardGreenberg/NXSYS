//
//  Preferences.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 10/3/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import "Preferences.h"
#include "ssdlg.h"
#include <map>
static NSString * FullSigKey = @"FullSignalDisplaysAreViews";
static NSString * ShowStopsKey = @"LastShowStopsPolicy";
static NSString * SignalRightMenuKey = @"SignalRightClickIsMenu";

extern bool FullSigsAreViews;
extern bool SignalRIsMenu;

@interface Preferences ()
{
    NSUserDefaults * defaults;
    std::map<id, int> StopsMap; /* map works, but not unordered_map. */
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
-(void)setBoolBox:(id)control onValue:(bool)value {
    [control setState: value ? NSControlStateValueOn : NSControlStateValueOff];
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
    
    StopsMap[_stopsTripping] = SHOW_STOPS_RED;
    StopsMap[_stopsNever] = SHOW_STOPS_NEVER;
    StopsMap[_stopsAlways] = SHOW_STOPS_ALWAYS;

    bool rightClickIsMenu = [defaults boolForKey:SignalRightMenuKey] ? true: false;
    bool fullSigsAreViews = [defaults boolForKey:FullSigKey] ? true : false;
    
    for (auto [control, policy] : StopsMap)
        [control setState: (ssp == policy) ? NSControlStateValueOn : NSControlStateValueOff];

    [self setBoolBox: _fullSigsViews   onValue: fullSigsAreViews];
    [self setBoolBox: _fullSigsWindows onValue: !fullSigsAreViews];

    [self setBoolBox: _rightClickMenus onValue: rightClickIsMenu];
    [self setBoolBox: _rightClickFSDs  onValue: !rightClickIsMenu];
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
    for (auto [control, policy] : StopsMap)
        if ([control state] == NSControlStateValueOn) {
            [defaults setInteger:policy forKey:ShowStopsKey];
            ImplementShowStopPolicy(policy);
            break;
        }

    FullSigsAreViews = ([_fullSigsViews state] == NSControlStateValueOn);
    [defaults setBool:(FullSigsAreViews ? YES : NO) forKey:FullSigKey];

    SignalRIsMenu = ([_rightClickMenus state] == NSControlStateValueOn);
    [defaults setBool:(SignalRIsMenu ? YES : NO) forKey:SignalRightMenuKey];
    [[self window] close];
    [NSApp stopModal];
}

-(void)windowWillClose:(NSNotification*)notification
{
    [NSApp stopModal];
}

@end
