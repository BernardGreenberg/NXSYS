//
//  ToolbarController.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/22/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import "ToolbarController.h"
#import "TLEditAppDelegate.h"

@interface TLENeverSelectPanel : NSPanel
@end
@implementation TLENeverSelectPanel
-(BOOL)canBecomeKeyWindow {return NO;}
-(BOOL)canBecomeMainWindow {return NO;}
@end


NSWindow* getMainWindow();
void AppCommand(unsigned int);
@implementation ToolbarController

- (void)windowDidLoad {
    [super windowDidLoad];
    [getMainWindow() addChildWindow:self.window ordered:NSWindowAbove];
}
-(IBAction)ToolbarButton:(id)sender
{
    // go learn about "acceptsFirstMouse".
    NSButton* b = (NSButton*)sender;
    AppCommand((unsigned int)b.tag);
}
-(NSButton*)findButton:(int)tag
{
    NSView * cview = self.window.contentView;
    for (NSView* v in cview.subviews) {
        if (v.tag == tag) {
            return (NSButton*)v;
        }
    }
    return nil; // we can get called with menu commands not in the tb.
}
-(void)EnableButton:(int)tag state:(BOOL)state
{
    NSButton* b = [self findButton:tag];
    if (b != nil) {
        [b setEnabled:state];
    }
}
-(BOOL)getCheck:(int)tag
{
    NSButton* b = [self findButton:tag];
    if (b != nil) {
        return [b state];
    }
    return NO;
}
@end

void EnableToolButton(void*, int cmd, int whichWay) {
    ToolbarController* tb = (ToolbarController*)getTLEDelegate().toolbar;
    BOOL state = whichWay ? YES : NO;
    [tb EnableButton:cmd state:state];
}

bool GetToolbarCheckState(void*, int cmd) {
    ToolbarController* tb = (ToolbarController*)getTLEDelegate().toolbar;
    return ([tb getCheck:cmd] != NO);
}