//
//  ChooseTrackController.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/5/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"

extern bool GlobalOfferingChooseTrack;

//Not much doin' here...
@interface ChooseTrackController : NSWindowController
@end

static ChooseTrackController * chooser = nil;

@implementation ChooseTrackController
- (id) init {
    if ( ! (self = [super initWithWindowNibName: @"ChooseTrack"]) )
    {
        NSLog(@"init failed for ChooseTrack");
    }
    return self;
    
} // end init


- (IBAction)CloseButton:(id)sender {
    [chooser.window close];
    GlobalOfferingChooseTrack = false;
}
@end

void OfferChooseTrack() {

    if (chooser == nil)
        chooser = [[ChooseTrackController alloc] init];
    [chooser showWindow:nil];
    [getNXWindow() addChildWindow:chooser.window ordered:NSWindowAbove];
}

void LoseChooseTrack () {
    GlobalOfferingChooseTrack = false;
    if (chooser) {
        [getNXWindow() removeChildWindow:chooser.window];
        [chooser.window close];
    }
    chooser = nil;  //ought be plenty.
}
