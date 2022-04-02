//
//  Preferences.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 10/3/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface Preferences : NSWindowController
+(void)initPreferredSettings;
-(void)setBoolBox:(id)control onValue:(bool)value;
-(void)showModal;
-(Preferences*)init;
@end
