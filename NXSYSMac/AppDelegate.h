//
//  AppDelegate.h, again, was NXMacAppDelegate.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/14/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "MainView.h"
#import "RelayTracerController.h"
#import "HelpController.h"
#import "RelayDrafterController.h"
#import "CustomAboutController.h"

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@property (weak) IBOutlet NSMenuItem *FSViewItem;
@property (unsafe_unretained) IBOutlet NSTextView *demoView;
@property (weak) IBOutlet NSMenuItem *DraftspersonItem;
@property (weak) IBOutlet NSMenuItem *AutomaticOperation;
@property (weak) IBOutlet NSMenuItem *ScenarioHelpItem;
@property (weak) IBOutlet MainView *theView;

@property (weak) IBOutlet NSScrollView *myScrollView;
@property (assign) IBOutlet NSWindow *window;
@property (strong) RelayTracerController *rlyTraceWindow;
@property (strong, nonatomic) HelpController* helpController;

@property (strong, nonatomic) CustomAboutController* aboutController;
@property (weak) IBOutlet NSMenuItem *NewAbout;

@end

AppDelegate * getNXAppDelegate();
NSWindow * getNXWindow();
MainView * getMainView();
