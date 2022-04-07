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

typedef void (^TLEditDlgCreator)(class GraphicObject*);

void OfferGenericWindlg(Class clazz, NSString* nib_name, RIDVector rid_vector, GraphicObject* obj);

/* These macros must expand in an NXSYS compile environment with GraphicObject defined.*/
#define REGISTER_DIALOG(RESOURCE_ID, CLASS_NAME, NIB_NAME) \
     static int dumy = RegisterTLEDitDialog \
        (RESOURCE_ID, \
            ^(GraphicObject *obj) {OfferGenericWindlg([CLASS_NAME class], NIB_NAME, RIDS, obj);});

#define REGISTER_DIALOG_2R(RESOURCE_ID, CLASS_NAME, NIB_NAME,RIDS) \
    static int dumy = RegisterTLEDitDialog \
        (RESOURCE_ID, \
           ^(GraphicObject *obj) {OfferGenericWindlg([CLASS_NAME class], NIB_NAME, RIDS, obj);});

#define REGISTER_DIALOG_4R(dumy,RESOURCE_ID,NIB_NAME,RIDS) \
    static int dumy = RegisterTLEDitDialog \
      (RESOURCE_ID, \
          ^(GraphicObject *obj) {OfferGenericWindlg([GenericWindlgController class], NIB_NAME, RIDS, obj);});
       

int RegisterTLEDitDialog(unsigned int resource_id,  TLEditDlgCreator creator);

@interface GenericWindlgController : NSWindowController<WinDialog>

@property void* hWnd;
@property  GraphicObject* NXGObject;

-(GenericWindlgController*)initWithNibAndRIDs:(NSString*)nibName rids:(RIDVector&)rids;

-(IBAction)activeButton:(id)sender;

-(void)showModal:(GraphicObject*)object;
-(void)reflectCommand:(NSInteger)command;
-(void)reflectCommandParam:(NSInteger)command lParam:(NSInteger)param;
-(void)didInitDialog;
@end
