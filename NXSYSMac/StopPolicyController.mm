//
//  StopPolicyController.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/1/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//  This is just about a textbook case of the minimal working modal dialog.

#import "StopPolicyController.h"

@implementation StopPolicyController
- (id) init:(int)policy
{
    _selectedPolicy = policy;
    self = [super initWithWindowNibName: @"StopPolicy"];
    if (!self || self==nil)
    {
        NSLog(@" Nib init failed for stop policy controller");
        printf("Init failed\n");
        return nil;
    }
     return self;
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    //So sagt man "WM_INITDIALOG" auff Mack.
    [_theMatrix selectCellWithTag:_selectedPolicy];
}

- (IBAction)Cancel:(id)sender {
    _selectedPolicy = 0;
    [NSApp stopModal];
}
- (IBAction)OK:(id)sender {
    NSButtonCell *selCell = [_theMatrix selectedCell];
    _selectedPolicy = (int)[selCell tag];
    [NSApp stopModal];
}
-(void)showModal
{
    [self showWindow:nil];
    [NSApp runModalForWindow:self.window];
}
@end

int StopPolicyDialog(int currentPolicy) {
    StopPolicyController * controller = [[StopPolicyController alloc] init:currentPolicy];
    [controller showModal];
    return controller.selectedPolicy;
}
