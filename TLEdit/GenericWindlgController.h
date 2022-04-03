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

struct TextKey {
    int control_id;
    const char * key;
};

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

typedef const std::initializer_list<TextKey> DefVector;
typedef const std::initializer_list<RIDPair> RIDVector;

typedef void (^TLEditDlgCreator)(void*);

#define REGISTER_DIALOG(RESOURCE_ID, CLASS_NAME, NIB_NAME) \
     static int dumy = RegisterTLEDitDialog \
        (RESOURCE_ID, ^(void *obj) { \
        [[[CLASS_NAME alloc] initWithNibAndObject:NIB_NAME \
                     object:obj] showModal];});


#define REGISTER_DIALOG_2S(RESOURCE_ID, CLASS_NAME, NIB_NAME,DEFS) \
    static int dumy = RegisterTLEDitDialog \
        (RESOURCE_ID, ^(void *obj) { \
            [[[CLASS_NAME alloc] initWithNibObjectAndDefs:NIB_NAME \
                object:obj defs:DEFS] showModal];});

#define REGISTER_DIALOG_2R(RESOURCE_ID, CLASS_NAME, NIB_NAME,RIDS) \
    static int dumy = RegisterTLEDitDialog \
        (RESOURCE_ID, ^(void *obj) { \
            [[[CLASS_NAME alloc] initWithNibObjectAndRIDs:NIB_NAME \
                object:obj rids:RIDS] showModal];});

#define REGISTER_DIALOG_4(dumy,RESOURCE_ID,NIB_NAME,DEFS) \
    static int dumy = RegisterTLEDitDialog \
      (RESOURCE_ID, ^(void *obj) { \
        [[[GenericWindlgController alloc] initWithNibObjectAndDefs:NIB_NAME \
                object:obj defs:DEFS] showModal];});

#define REGISTER_DIALOG_4R(dumy,RESOURCE_ID,NIB_NAME,RIDS) \
    static int dumy = RegisterTLEDitDialog \
      (RESOURCE_ID, ^(void *obj) { \
        [[[GenericWindlgController alloc] initWithNibObjectAndRIDs:NIB_NAME \
                object:obj rids:RIDS] showModal];});

int RegisterTLEDitDialog(unsigned int resource_id,  TLEditDlgCreator creator);

@interface GenericWindlgController : NSWindowController<WinDialog>

@property void* hWnd;
@property void* NXGObject;

-(GenericWindlgController*)initWithNibObjectAndDefs:(NSString*)nibName
                                             object:(void*)object
                                               defs:(DefVector&)defs;
-(GenericWindlgController*)initWithNibObjectAndRIDs:(NSString*)nibName
                                             object:(void*)object
                                               rids:(RIDVector&)rids;

-(IBAction)activeButton:(id)sender;

-(void)showModal;
-(void)reflectCommand:(NSInteger)command;
-(void)reflectCommandParam:(NSInteger)command lParam:(NSInteger)param;

@end
