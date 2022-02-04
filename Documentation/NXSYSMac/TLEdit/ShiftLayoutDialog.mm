//
//  ShiftLayoutDialog.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/30/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import "ShiftLayoutDialog.h"

@interface ShiftLayoutDialog ()
@property IBOutlet NSTextField* X;
@property IBOutlet NSTextField* Y;
@end

void ShiftLayout(int, int);

@implementation ShiftLayoutDialog
- (id) init {
    if ( ! (self = [super initWithWindowNibName: @"ShiftLayoutDialog"]) )
    {
        NSLog(@"init failed for shift layout dialog");
    }
    return self;
}
-(void)showModal
{
    // "no [show window]" encourages rumModalForWindow to expose app-centrally...
    [NSApp runModalForWindow:[self window]];
}
-(IBAction)OK:(id)sender
{
    ShiftLayout((int)_X.integerValue, (int)_Y.integerValue);
    [NSApp stopModal];
    [[self window] close]; // in Yosemite, omit this => crash
}
-(IBAction)Cancel:(id)sender
{
    [NSApp stopModal];
    [[self window] close];
}

@end
