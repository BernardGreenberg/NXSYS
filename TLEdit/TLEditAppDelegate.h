//
//  TLEditAppDelegate.h
//  TLEdit
//
//  Created by Bernard Greenberg on 9/16/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "MainView.h"
#import "HelpController.h"
#import "ToolbarController.h"
@interface TLEditAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@property (strong) HelpController* helpController;
@property (weak) IBOutlet NSScrollView *theScrollView;
@property (weak) IBOutlet MainView *theView;
@property (weak) IBOutlet NSTextField *StatusBar;
@property (weak) IBOutlet NSMenuItem *UndoMenuItem;
@property (weak) IBOutlet NSMenuItem *RedoMenuItem;
@property (strong) ToolbarController * toolbar;
@property (assign) IBOutlet NSWindow *window;
@property bool wporg_set;
@property NSPoint wporg;
- (BOOL)validateMenuItem:(NSMenuItem *)item;
-(void)setUndoMenu:(const char*)undo Redo:(const char *)redo;
-(IBAction)saveDocument:(id)sender;
@end

TLEditAppDelegate* getTLEDelegate();
