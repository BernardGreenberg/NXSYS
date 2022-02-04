//
//  RelayDrafterController.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/25/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface RelayDrafterController : NSWindowController<NSWindowDelegate>
@property (weak) IBOutlet NSView *theView;  // it's classier than that, but need not know that.
@property (weak) NSMenuItem * mainMenuItem;
@property void* hWnd;
-(IBAction)drawNew:(id)sender;
-(IBAction)toggle:(id)sender;
-(IBAction)recede:(id)sender;
-(IBAction)clear:(id)sender;
-(IBAction)closeCmd:(id)sender;
-(IBAction)sourceCmd:(id)sender;
-(void)setSourceString:(NSString*)source_string;
-(bool)isSourceLocationAvailable;
@end
