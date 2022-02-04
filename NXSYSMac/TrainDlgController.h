//
//  TrainDlgController.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/5/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "WinDialogProtocol.h"
#include <vector>

@interface TrainDlgController : NSWindowController<WinDialog>
@property (weak) IBOutlet NSButton *COPB;
@property (weak) IBOutlet NSTextField *COText;
@property (weak) IBOutlet NSButton *Reverse;
@property (weak) IBOutlet NSMatrix *theMatrix;
@property (weak) IBOutlet NSButtonCell *FreeWill;
@property (weak) IBOutlet NSSlider *Slider;
@property (weak) IBOutlet NSTextField *TrainName;
@property (weak) IBOutlet NSTextField *TrainLength;
@property (weak) IBOutlet NSTextField *TrainLocation;
@property (weak) IBOutlet NSTextField *TrainSpeed;
@property (weak) IBOutlet NSTextField *LastSignal;
@property (weak) IBOutlet NSTextField *LastAspect;
@property (weak) IBOutlet NSTextField *NextSignal;
@property (weak) IBOutlet NSTextField *NextAspect;

@end
