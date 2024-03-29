//
//  GenericWindlgControllerWindowController.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/19/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <initializer_list>
#include <functional>
#include "WinDialogProtocol.h"

struct RIDPair {
    const char * Symbol;
    int resource_id;
};

typedef const std::initializer_list<RIDPair> RIDVector;

/* https://stackoverflow.com/questions/8945909/how-to-use-nsstring-as-key-in-objective-c-stdmap */
struct CompareNSString {
//: public std::__binary_function<NSString*, NSString*, bool> { //out in C++17
    bool operator()(NSString* lhs, NSString* rhs) const {
        if (rhs != nil)
            return (lhs == nil) || ([lhs compare: rhs] == NSOrderedAscending);
        else
            return false;
    }
};

int DefineWindlgGeneric(int resource_id, NSString* nib_name, RIDVector rid_vector);
int DefineWindlgWithClass(Class clazz, int resource_id, NSString* nib_name, RIDVector rid_vector);

#define DEFINE_WINDLG_WITH_CLASS(RESOURCE_ID, CLASS_NAME, NIB_NAME, RID_LIST) \
static int dlgRslt_ ## RESOURCE_ID = DefineWindlgWithClass([CLASS_NAME class], RESOURCE_ID, NIB_NAME, RID_LIST);

@interface GenericWindlgController : NSWindowController<WinDialog>

-(IBAction)activeButton:(id)sender;    /* Target action from buttons, set in IB */

-(void)reflectCommand:(NSInteger)command;
-(void)reflectCommandParam:(NSInteger)command lParam:(NSInteger)param;
-(void)didInitDialog;  /* "virtual" to be overriden */
-(void)controlTextDidChange:(NSNotification *)notification;
@end
