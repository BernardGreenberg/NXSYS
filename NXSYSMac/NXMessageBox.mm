//
//  NXMessageBox.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 10/2/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#include "MessageBox.h"

@interface NXMessageBox : NSWindowController
{
    NSString * header;
    NSString * text;
    NSString * image_name;
    int flags;
}
@property (weak) IBOutlet NSTextField* headerMsg;
@property (weak) IBOutlet NSTextField* detailMsg;
@property (weak) IBOutlet NSImageView* image;

@property (weak) IBOutlet NSButton* yes;
@property (weak) IBOutlet NSButton* no;
@property (weak) IBOutlet NSButton* cancel;
@property int result;

@end

@implementation NXMessageBox
-(id)init
{
     return [super initWithWindowNibName:@"NXMessageBox"]; // check not worth warm spit.
}
- (void)windowDidLoad {
    [super windowDidLoad];
    
    [_headerMsg setStringValue:header];
    [_detailMsg setStringValue:text];
    [_yes setTag:IDYES];
    [_cancel setTag:IDCANCEL];
    
    switch (flags & MB_LOWMASK) {
        case MB_OK:
        default:
            [_yes setHidden:TRUE];
            [_cancel setHidden:TRUE];
            [_no setTitle:@"OK"];
            [_no setTag:IDOK];
            break;
        case MB_YESNO:
            [_yes setHidden:FALSE];
            [_cancel setHidden:TRUE];
            [_no setTitle:@"No"];
            [_no setTag:IDNO];
            break;
        case MB_YESNOCANCEL:
            [_yes setHidden:FALSE];
            [_cancel setHidden:FALSE];
            [_no setTitle:@"No"];
            [_no setTag:IDNO];
            break;
        case MB_OKCANCEL:
            [_yes setHidden:TRUE];
            [_cancel setHidden:FALSE];
            [_no setTitle:@"OK"];
            [_no setTag:IDOK];
            break;
    }
    if (image_name) {
        [_image setImage:[NSImage imageNamed:image_name]];
        [_image setHidden:FALSE];
    } else if (flags & MB_ICONEXCLAMATION) {
        [_image setImage:[NSImage imageNamed:@"dialog_warning.png"]];
        [_image setHidden:FALSE];
    } else if (flags & MB_ICONSTOP) {
        [_image setImage:[NSImage imageNamed:@"agt_stop_256.png"]];
        [_image setHidden:FALSE];
    } else {
        [_image setHidden:TRUE];
    }
}
-(IBAction)buttons:(id)sender
{
    NSButton* b = (NSButton*)sender;
    _result = (int)[b tag];
    [self close];
    [NSApp stopModal];
}
-(void)showModal:(int)aflags hdr:(NSString*)hdr text:(NSString *)atext image_name:(NSString*)aimage
{
    header = hdr;
    text = atext;
    flags = aflags;
    image_name = aimage;
    [NSApp runModalForWindow:[self window]];
}
@end

int MessageBoxWithImage(void* hWnd, const char * text, const char * hdr, const char * image, int flags) {
        NSString * nstext = [NSString stringWithUTF8String:text];
    NSString * nshdr = [NSString stringWithUTF8String:hdr];
    NSString * image_name = nullptr;
    if (image) {
        image_name = [NSString stringWithUTF8String:image];
    }
    NXMessageBox *  mb = [[NXMessageBox alloc] init];
    [mb showModal:flags hdr:nshdr text:nstext image_name:image_name];
    return mb.result;
}

int MessageBox(void* hWnd, const char * text, const char * hdr, int flags) {
    return MessageBoxWithImage(hWnd, text, hdr, nullptr, flags);
}
