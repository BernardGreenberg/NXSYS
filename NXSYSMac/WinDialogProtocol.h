//
//  WinDialogProtocol.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/6/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <vector>

typedef void* HWND;

@protocol WinDialog

-(HWND)GetControlHWND:(NSInteger) control_id;
-(void)SetControlText:(NSInteger)ctl_id text:(NSString *)text;
-(long)GetControlText:(NSInteger)ctl_id buf:(char*)buf length:(long)length;
-(void)DestroyWindow;
@optional
-(void)showControl:(NSInteger)ctl_id showYes:(NSInteger)yesno;
-(void)setSliderValue:(NSInteger)ctl_id valof100:(NSInteger)valof100;
-(void)checkRadioButton:(NSInteger)ctl_id first:(NSInteger)first last:(NSInteger)last;
-(bool)getDlgItemState:(NSInteger)ctl_id;
-(bool)getDlgItemCheckState:(NSInteger)ctl_id;
-(void)setDlgItemCheckState:(NSInteger)ctl_id value:(NSInteger)yesNo;
-(void)EnableControl:(NSInteger)ctl_id yesNo:(NSInteger)yesNo;
-(void)updateCallbackActor:(void*)new_actor;
/* This is used for outer partners of this protocol to communicate around it
   in any way they choose. They can use one param to say what the other means.
   Think SendMessage(HWND, MESSAGE, WPARAM, LPARAM);
 */
-(int)kludgeParam2:(NSInteger)param1 param2:(NSInteger)param2;

@end


