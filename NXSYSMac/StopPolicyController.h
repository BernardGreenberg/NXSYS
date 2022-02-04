//
//  StopPolicyController.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/1/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface StopPolicyController : NSWindowController
@property (weak) IBOutlet NSMatrix *theMatrix;
@property int selectedPolicy;
@end
