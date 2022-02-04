//
//  NXRelayWindowController.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/22/14.
//  before he knew jack sugar about proper delegate functionality implementation.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface RelayTracerController : NSWindowController<NSWindowDelegate>
//You have to set "Custom Class" and "File owner" for ib to allow you to drop these in.
@property (unsafe_unretained) IBOutlet NSTextView *textView;
-(void)log:(NSString*)nsstring;
@property (weak) NSMenuItem *windowMenuItem;
@property (weak) NSMenuItem *consoleMenuItem;

+ (RelayTracerController*) CreateWithMenu:(NSMenu*) topLevelMenu;
- (void)ClickTraceToConsole;
- (void)ClickTraceToWindow;
- (bool)TryCharTrap:(NSInteger)ch;
@end
