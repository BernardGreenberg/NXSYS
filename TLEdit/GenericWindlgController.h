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

class GraphicObject;  // don't want nxgo.h

struct RIDPair {
    const char * Symbol;
    int resource_id;
};

/* https://stackoverflow.com/questions/8945909/how-to-use-nsstring-as-key-in-objective-c-stdmap */
struct CompareNSString: public std::binary_function<NSString*, NSString*, bool> {
    bool operator()(NSString* lhs, NSString* rhs) const {
        if (rhs != nil)
            return (lhs == nil) || ([lhs compare: rhs] == NSOrderedAscending);
        else
            return false;
    }
};

typedef const std::initializer_list<RIDPair> RIDVector;

typedef void (^TLEditDlgCreator)(GraphicObject*);

void OfferGenericWindlg(Class clazz, NSString* nib_name, RIDVector rid_vector, GraphicObject* obj);

/* Macros to declare property dialogs at top level load time. They capture their arguments in an
   Objective-C closure, which is saved in a map by resource ID for invocation at run time. */

#define REGISTER_DIALOG_GENERAL(DUMMY, RESOURCE_ID, CLASS_NAME, NIB_NAME, RID_LIST) \
static int DUMMY = RegisterTLEDitDialog \
    (RESOURCE_ID, \
       ^(GraphicObject *object) {OfferGenericWindlg([CLASS_NAME class], NIB_NAME, (RID_LIST), object);});

/* Form used in files that subclass Generic Window controller, only one allowed per such file */
#define REGISTER_DIALOG_2R(RESOURCE_ID, CLASS_NAME, NIB_NAME, RID_LIST) \
    REGISTER_DIALOG_GENERAL(dumy1, RESOURCE_ID, CLASS_NAME, NIB_NAME, RID_LIST)

/* Form used in MacObjDlgs.mm for all the rest */
#define REGISTER_DIALOG_4R(DUMMY, RESOURCE_ID, NIB_NAME, RID_LIST) \
    REGISTER_DIALOG_GENERAL(DUMMY, RESOURCE_ID, GenericWindlgController, NIB_NAME, (RID_LIST))

int RegisterTLEDitDialog(unsigned int resource_id,  TLEditDlgCreator creator);

@interface GenericWindlgController : NSWindowController<WinDialog>

@property void* hWnd;                  /* the Windows simulation HWND object for the dialog*/
@property  GraphicObject* NXGObject;   /* the NXSYS graphic object on which it is to be applied */

-(GenericWindlgController*)initWithNibAndRIDs:(NSString*)nibName rids:(RIDVector&)rids;

-(IBAction)activeButton:(id)sender;    /* Target action from buttons, set in IB */

-(void)showModal:(GraphicObject*)object;
-(void)reflectCommand:(NSInteger)command;
-(void)reflectCommandParam:(NSInteger)command lParam:(NSInteger)param;
-(void)didInitDialog;  /* "virtual" to be overriden */
@end
