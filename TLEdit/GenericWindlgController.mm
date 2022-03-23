//
//  GenericWindlgController.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/19/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//
typedef void *HWND;
#import "TLWindowsSide.h"

#import "GenericWindlgController.h"
#include "MessageBox.h"
#include "WinMacCalls.h"
#include <map>

/* While I am proud of this piece of work, it is a but a kludge to reconcile the
 respective deficiencies of the Windows and Macintosh dialog systems.
 The Windows system requires defining a file full of batty control ID numbers, which,
 from their header file lairs, act in both resource definitions and your code, yet 
 easily inspectable, maintainable, etc., while requiring an army of middlepersons,
 e.g., GetDlgItemInt, to interpret them.
 
 The Mac system is 'much better': you create variables into which solid (or not so solid)
 references to your controls are seamlessly inserted by the Interface Builder and its
 runtime, the NIB loader.  You must be aware of each control you intend to stuff or read
 or (buttons) be activated by, although not others (e.g., labels).  No numbers, no shim agents.
 
 But my system doesn't want to supply code for each dialog; the code already exists in C++,
 and deals in middlepersons, whom we must, and do, implement in the WindowsDlg protocol
 regime.  The hard part is identifying the controls.  We do not want to make variables
 for each control, because that must be done at design/build time, and is a major pain in the
 Interface Builder if you have more than 2 or 3 of them.  Doing so would require special-case
 code for each dialog to interpret windows control ID's as tickets to its variables.
 
 Mac allows you to store "tags" in controls, but they are limited to integers, and cannot
 be symbolic references or strings.  Managing these integers, e.g., to be Windows resource ID's
 (done in the Stop Policy dialog in the main app), has already proven beyond human capacity,
 and lacks basic maintainability: one can't search for uses.
 
 The tack chosen here is to search the list of instantiated controls for "key" text strings, or
 little snippets of them (and find input controls by their labels and the "tab order"). While 
 proclaimed a "no no" by wise engineers, as it defeats localizability, c'est la vie, 
 mein guter amigo.

*/

@interface GenericWindlgController () // () means append to def. given already.
{
    /* Finally, here is how you "do" instance variables:  { } at the beginning of @interface. */
    std::vector<char>boxboeuf;
    DefVector* pDefs;
    std::map<NSInteger,HWND> CtlidToHWND;
}
@end

@implementation GenericWindlgController

-(GenericWindlgController*)initWithNibAndObject:(NSString *)nibName object:(void *)object
{
    self = [super initWithWindowNibName:nibName];
    if (self != nil) {
        _NXGObject = object;
        return self;
    } else {
        /* this never happens, no matter how bad the Nib name is.  You get nil in
        windowDid(supposedly)Load if it's bad. */
        MessageBox(NULL, nibName.UTF8String, "Cannot initialize GenericWndlg from NIB", MB_ICONSTOP|MB_OK);
        abort();
    }
    return nil;
}
-(void)showModal
{
    NSPoint p = NXViewToScreen(NXGOLocAsPoint(_NXGObject)); //whole panel -> whole mac screen
    NSPoint placement = NSMakePoint(p.x-150, p.y-60); //mac coord "yup".
    [self.window setFrameTopLeftPoint:placement];

    [self showWindow:self]; // if you don't do this yourself, runModal will toss your frame.
         //Don't believe me, try it yourself (comment out this line).
    
    [NSApp runModalForWindow:self.window];
}
-(void)DestroyWindow
{
    assert(!"DestroyWindow in GenericWindlgController shouldn't be called.");
}

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    return self;
}
-(void)real_deall
{

  /* this will get called at exit time because the hWnd is weak and not retaining us. */

    for (auto& iterator : CtlidToHWND) // not deleting from map here, but in clear(), no skip-step.
         DeleteHwndObject(iterator.second);
    CtlidToHWND.clear(); // probably not necessary, hope C++ dtor will do it.
    DeleteHwndObject(_hWnd);
}
-(IBAction)GenericCancel:(NSNotification*)noti
{
    /* See EndDialog */
    callWndProcIdCancel(_hWnd, _NXGObject);
}
-(IBAction)GenericOK:(NSNotification*)noti
{
    /* See EndDialog */
    callWndProcIdOK(_hWnd, _NXGObject);
}
- (void)windowDidLoad
{
    
    [self setControlMap];

    [super windowDidLoad];

    /* we are retained on the stack, not in c++ code */
    _hWnd = WinWrapNoRetain(self, self.window.contentView, @"Generic Dialog");
    
   /* Find and direct the OK and Cancel buttons to this good office. */

    NSView * view = self.window.contentView;
    for (NSWindow* childView in view.subviews) {
        if ([childView isKindOfClass:[NSButton class]]){
            NSButton * button = (NSButton* )childView;
            NSString * title = button.title;
            if (![title compare:@"Cancel"]) {
                [button setTarget: self];
                [button setAction: @selector(GenericCancel:)];
            } else if (![title compare:@"OK"]) {
                [button setTarget: self];
                [button setAction: @selector(GenericOK:)];
            }
        }
    }

    /* now run all the code that uses the stuff set before we were called*/

    callWndProcInitDialog(_hWnd, _NXGObject);
}

-(int)lookUpInDefs:(NSString*)s
{
    const char * bb = &boxboeuf[0];
    const char * u = s.UTF8String;
    for (auto& def : *pDefs) {
        const char * key = def.key;
        const char * slasher = strchr(key, '/');
        
        if (slasher != NULL) {
            if (!!strncmp(bb, key, slasher-key))
                continue;
            key = slasher + 1;
        }
        if (strstr(u, key) != NULL) {
            return def.control_id;
        }
    }
    return 0;
}

-(void)maybeSubregisterButton:(NSButton*)button
{
    int rid = [self lookUpInDefs:button.title];
    if (rid > 0) {
        [button setState:NO];
        [self recordIt:button rid:rid];
    }
}
-(BOOL)jtbp:(NSTextField*)textField
{
    // At this point, we know that
    // 1) The current field is a text field
    // 2) Its string-content, possibly box-qualified, is on our watch-list.
    if (textField.isEditable) // Now we must be pure
        return NO;
    NSView* nextField = textField.nextKeyView;
    if (nextField == nil) // we must be chained on.
        return NO;
    if ([nextField isKindOfClass:[NSStepper class]]) // why not? we need this.
        return YES;
    if (![nextField isKindOfClass:[NSTextField class]])
        return NO; // he'd better be a text field.
    return YES; // the sample text pane in the text dlg isn't editable!#@#$@
    // This will screw up if you are careless and nextKey one label to the next; the second
     // will get clobbered with dlg content.
//    NSTextField * nextTextField = (NSTextField*)nextField;
  //  return YES;
 //    return nextTextField.isEditable; // and editable.

}

-(void)recordIt:(NSView*)view rid:(int)rid
{
    printf("recordIt: %d %p\n", rid, view);
    assert(CtlidToHWND.count(rid) == 0); /* should not ever be found twice!  */
    CtlidToHWND[rid] = WinWrapControl(self, view, rid, @"Generic Windlg"); // wwc adds ctlid#
}

-(void)setControlMapRecurseViews:(NSView*)view
{
    for (NSView* childView in view.subviews) {
        if ([childView isKindOfClass:[NSTextField class]]){
            NSTextField * textField = (NSTextField*)childView;
            int rid = [self lookUpInDefs:textField.stringValue];
            if (rid != 0) {
                /* We like this guy. Is he our man, or his John-the-Baptist? */
                if ([self jtbp:textField])
                    [self recordIt:textField.nextKeyView rid:rid];
                else
                    [self recordIt:textField rid:rid];
            }
        } else if ([childView isKindOfClass:[NSButton class]]) {
            [self maybeSubregisterButton:(NSButton*)childView];
        } else if ([childView isKindOfClass:[NSBox class]]) {
            NSBox * box = (NSBox*)childView;
            [self setToBoxboeuf:((NSTextField*)box.titleCell).stringValue];
            //not sure what this [0] indirection is, but wtf
            [self setControlMapRecurseViews:childView.subviews[0]];
            [self setToBoxboeuf:nil];
        } else if ([childView isKindOfClass:[NSMatrix class]]) {
            NSMatrix * matrix = (NSMatrix*)childView;
            for (NSButtonCell* cell in matrix.cells) {
                [self maybeSubregisterButton:(NSButton*)cell];
            }
        }
    }
}

-(void)setToBoxboeuf:(NSString* )ns
{
    size_t len = 0;
    if (ns!= nil) {
        assert(boxboeuf[0] == '\0'); // no real recursion yet.
        const char * u = ns.UTF8String;
        len = strlen(u); //ns.len no good for chars that expand to more than 1.
        strncpy(&boxboeuf[0], u, len);
    }
    boxboeuf[len] = '\0';
}

-(void)setControlMap
{
    assert(self.window != nil);
    
    boxboeuf.resize(128);
    [self setToBoxboeuf:nil];

    [self setControlMapRecurseViews:self.window.contentView];
 
    for (auto& def : *pDefs) {
        if (! CtlidToHWND.count(def.control_id)) {
            char buf[128];
            sprintf(buf, "Did not find control for resource id %d, matching \"%s\".",
                    def.control_id,
                    def.key);
            MessageBox(NULL, buf, "Generic Windlg Controller init", MB_OK|MB_ICONSTOP);
            abort();
        }
    }
}

-(void)SetControlText:(NSInteger)ctlid text:(NSString*)text
{
    NSView* view = [self GetControlView:ctlid];
    assert([view isKindOfClass:[NSTextField class]]);
    [(NSTextField*)view setStringValue:text];
}
-(long)GetControlText:(NSInteger)ctlid buf:(char*)buf length:(NSInteger)len
{

    NSView* view = [self GetControlView:ctlid];
    assert([view isKindOfClass:[NSTextField class]]);
    NSString *ns = ((NSTextField *)view).stringValue;
    strncpy(buf, ns.UTF8String, len);
    buf[len-1] = '\0';
    return strlen(buf);
}
-(void)setDlgItemCheckState:(NSInteger)ctl_id value:(NSInteger)yesNo
{
    NSView* view = [self GetControlView:ctl_id];
    assert([view isKindOfClass:[NSButtonCell class]] |[view isKindOfClass:[NSButton class]]);
    [(NSButton*)view setState:yesNo];
}
-(bool)getDlgItemCheckState:(NSInteger)ctl_id
{
    NSView* view = [self GetControlView:ctl_id];
    //checkboxes report as "nsbutton", but radios as "nsbuttoncell"
    assert([view isKindOfClass:[NSButtonCell class]] |[view isKindOfClass:[NSButton class]]);
    NSButton* button = (NSButton*)view; // don't understand class structure
    return [button state] ? true : false;
}
-(HWND)GetControlHWND:(NSInteger)ctlid
{
    assert(CtlidToHWND.count(ctlid) > 0);
    return CtlidToHWND[ctlid];
}
-(NSView*)GetControlView:(NSInteger)ctlid
{
    assert(CtlidToHWND.count(ctlid) > 0);
    return getHWNDView(CtlidToHWND[ctlid]);
}
-(void)showControl:(NSInteger)control showYes:(NSInteger)yesno
{
    [[self GetControlView:control] setHidden: ((yesno == 0) ? YES : NO)];
}

-(void)reflectCommand:(NSInteger)command
{
    callWndProcGeneralCommandParam(_hWnd, _NXGObject, (int)command, 0);
}

-(void)reflectCommandParam:(NSInteger)command lParam:(NSInteger)param
{
    callWndProcGeneralCommandParam(_hWnd, _NXGObject, (int)command, param);
}
-(void)dealloc
{
 // important to debug here.  This really gets called when you manage
// your strongptrs correctly.
 //   printf("GenericWindlgController dealloc called\n");
    [self real_deall];
}

-(GenericWindlgController*)initWithNibObjectAndDefs:(NSString*)nibName
                                             object:(void*)object
                                             defs:(DefVector&)defs
{
    pDefs = &defs;
    return [self initWithNibAndObject:nibName object:object];
}
-(IBAction)activeButton:(id)sender
{
    /* If you route buttons here via IB, that is, in effect, routing the Windows IDC_FOO_BAR
       to the Windows DlgProc of this dialog. */

    if ([sender isKindOfClass:[NSMatrix class]]) {
        NSMatrix * mater = (NSMatrix *)sender;  // juxta filium clamantem
        sender = mater.selectedCell;
    }
    /* A reverse-hash-map on strongptrs is really called for, but this is for $@$@# buttons
     that get pushed by humans; scanning a 2-dozen-long map for EQ is no issue. */
    for (auto& iterator : CtlidToHWND) {
        auto hWnd = iterator.second;
        if (getHWNDView(hWnd) == sender) {
            [self reflectCommand:getHWNDCtlid(hWnd)];
            return;
        }
    }
    assert(!"Didn't find here-routed active button in data structure.");
}
@end

/* Can't use STL -- load time initializes it at unpredictable time compared to our registrees */
/* Can't use structs, either, as funargs get gc'ed if not protected by strong pointer */
/* This is a lot easier. */
static size_t nRegisteredStaticDialogs = 0;
#define MAX_DLG_STATIC_INIT_TABLE 40
static unsigned int RIDS[MAX_DLG_STATIC_INIT_TABLE];
static __strong TLEditDlgCreator creators[MAX_DLG_STATIC_INIT_TABLE]; /* let 'em lie at app close - no dealloc needed. */

/* this gets called at load-time in unpredictable order */
int RegisterTLEDitDialog(unsigned int resource_id,  TLEditDlgCreator creator) {
    assert(nRegisteredStaticDialogs < MAX_DLG_STATIC_INIT_TABLE);
    size_t ix = nRegisteredStaticDialogs++;
    creators[ix] = creator;
    RIDS[ix] = resource_id;
    return 0;
}


void MacDialogDispatcher(unsigned int resource_id, void * obj) {
    for (size_t i = 0; i < nRegisteredStaticDialogs; i++) {
        if (RIDS[i] == resource_id) {
            (creators[i])(obj);
            return;
        }
    }
    char barf[200];
    sprintf(barf, "The dialog with Resource ID %d hasn't been implemented yet.  How lucky we didn't crash!", (int)resource_id);
    MessageBox(NULL, barf, "TLEdit Attribute Dialog Manager", MB_OK);
};
