//
//  InfoController.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 10/1/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import "InfoController.h"

@interface InfoController ()
@property (unsafe_unretained) IBOutlet NSTextView *textView;
@end

@implementation InfoController

-(id)init
{
    return [super initWithWindowNibName:@"Info"];  //check is worthless.
}
-(void)showModal:(NSString *)data
{
    [self window];//this side-effects nib loading, but must be base class' "window"
    [_textView setString:data];
    [NSApp runModalForWindow:self.window]; // does its own showWindow, and positions better.
}
-(IBAction)onOK:(id)sender
{
    [self close];
    [NSApp stopModal];
}
@end
