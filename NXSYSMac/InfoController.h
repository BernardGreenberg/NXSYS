//
//  InfoController.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 10/1/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface InfoController : NSWindowController
-(id)init;
-(void)showModal:(NSString*)data;
@end
