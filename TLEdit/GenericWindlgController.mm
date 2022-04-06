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
#include "STLExtensions.h"
#include <unordered_map>
#include <exception>
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
 
 [OBSOLETE: The tack chosen here is to search the list of instantiated controls for "key" text strings, or
 little snippets of them (and find input controls by their labels and the "tab order"). While 
 proclaimed a "no no" by wise engineers, as it defeats localizability, c'est la vie, 
 mein guter amigo. ]
 
 3-24-2022
 
 Nevertheless, we can use that strategy to stuff the "tags" fields programmatically, which, while
 not solving the dialog specification design problem, at least greatly simplifies the
 taking of the desired action at button-push time.  Making hash-tables of strong pointers to
 integers, which would be otherwise required, is quite difficult.  Linear lookup wasn't bad,
 but this is better.

4-4-2022
 
 This house was cleaned from basement to spire by taking advantage of the (new since 2014)
 "identifier" field in Cocoa controls, where not only can we store an arbitrary string, but
 the string turns up in Xcode searches!  The new system puts the actual name of the intended
 Windows resource ID in it (e.g., "IDC_EDIT_SWITCH"), which is supplied by macros with the
 corresponding value in the calls to instantiate this class (and its progeny). The entire
 system of searching label texts and tracking container hierarchy names is gone. The
 numeric resource codes are stored in the tag fields by the code below at dialog creation time.
 With this scheme, the code now can diagnose missing, unknown, and duplicate tags/id's!

*/

static std::map<NSString*, int> ByTitle{{@"OK", IDOK}, {@"Cancel", IDCANCEL}};

struct GenericWindlgException : public std::exception {};

@interface GenericWindlgController () // () means append to def. given already.
{
    /* Finally, here is how you "do" instance variables:  { } at the beginning of @interface. */
    std::unordered_map<NSInteger,HWND> CtlidToHWND;
    std::map<NSString*, int, CompareNSString> RIDValMap;
}
@end

@implementation GenericWindlgController

-(GenericWindlgController*)initWithNibAndObject:(NSString *)nibName object:(void *)object
{
   // nibName = @"Bad Nib Name"; // for (unrewarding) testing.
    self = [super initWithWindowNibName:nibName];
    if (self == nil)
    /* this never happens, no matter how bad the Nib name is. Not at all clear how to catch
     the error. Will throw an unhandled exception if ever happens. */
        [self barf: FormatString("Cannot initialize from NIB %s", nibName.UTF8String)];

    _NXGObject = object;
    return self;
}
-(void)showModal
{
    try {
        if (!self.window) //Not clear why this is the first we see it.
            [self barf: FormatString("Dialog window did not load from nib %s", self.windowNibName.UTF8String)];

        NSPoint p = NXViewToScreen(NXGOLocAsPoint(_NXGObject)); //whole panel -> whole mac screen
        NSPoint placement = NSMakePoint(p.x-150, p.y-60); //mac coord "yup".

        // Oddly enough, this seems to be the place where WindowDidLoad gets invoked.
        // Thus, CtlidToHwnd will be empty at this point even if all is well.
        [self.window setFrameTopLeftPoint:placement];

        if (CtlidToHWND.size() == 0) // If there was an error, don't show
           return;

        [self showWindow:self]; // if you don't do this yourself, runModal will toss your frame.
        //Don't believe me, try it yourself (comment out this line).
        [NSApp runModalForWindow:self.window];
    } catch (GenericWindlgException e) {
        //pass;
    }
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
    if(_hWnd) // can be null in error situations
        DeleteHwndObject(_hWnd);
}
- (void)windowDidLoad
{
    assert(self.window);  // this never happens, even with deliberately bad nib.

    try {
        [self createControlMap];  //Set up all the HWND and resource-id linkages.
        
        [super windowDidLoad];   //Do whatever Cocoa needs/wants.

        /* we are retained on the stack, not in c++ code */
        _hWnd = WinWrapNoRetain(self, self.window.contentView, @"Generic Dialog");
    
        /* now run all the code that uses the stuff set before we were called, all the Windows code */
        callWndProcInitDialog(_hWnd, _NXGObject);

    } catch (GenericWindlgException e){
        /* If there was a "crash", don't crash, but leave an empty basket to assert it.
         Can't be here if message box was not already raised.*/

        CtlidToHWND.clear();
    }

}
-(void)barf:(std::string) message
{
    /* These are not user errors, but logic/coding resource-tagging errors that should be
     debugged out.  Nevertheless, it is better to say something that a user can report, something
     better than "Internal logic error... please do so and so ..." Breakpoint here
     to debug. */

    MessageBoxS(NULL, message, "Generic Windlg Controller internal error", MB_OK|MB_ICONSTOP);
    throw GenericWindlgException();
}

-(bool)maybeRegisterView:(NSView*)view
{
    /* Controls of any type which have a non-_ "identifier" string get registered
       by the value of the Windows symbol so named.  Allow unknown identifiers
       if they don't start with IDC_.
     */
    if (!view.identifier)  // test for null -- not sure it can happen.
        return false;

    NSString* identifier = (NSString*)view.identifier;
    
    /* Avoid hash search if Cocoa internal. No Windows RID starts with _. */
    if ([identifier characterAtIndex:0] == '_')
        return false;

    if (RIDValMap.count(identifier)) {  //If we have it
        int resource_id = RIDValMap[identifier];
        if (CtlidToHWND.count(resource_id))
            [self barf: FormatString("More than one control has identifier: %s", identifier.UTF8String)];

        /* Create and link an HWND object to the control, and register it in the
         resource_id to HWND map. Insert the numeric ID into the control's "tag" attribute. */
        CtlidToHWND[resource_id] = WinWrapControl(self, view, resource_id, @"Generic Windlg");
        [(NSControl*)view setTag: resource_id];
        return true;
    }
    /* Not found.  Diagnose IDC_ (but not IDCANCEL!). Leave others alone. */
    if (identifier.length >= 4)
        if ([[identifier substringToIndex:4] isEqualTo:@"IDC_"])
            [self barf: FormatString("Identifier %s in control not known to RID table.", identifier.UTF8String)];
    return false;
}

-(void)createControlMapRecurse:(NSView*)parentView
{
    for (NSView* view in parentView.subviews) {
        if ([self maybeRegisterView: view]) {
            // "pass;"
        }
        else if ([view isKindOfClass:[NSBox class]]) {
            [self createControlMapRecurse: view.subviews[0]];
        }
        /* Necessary until all OK/Cancel buttons supplied witn "identifier"s.
         Don't even need entry in CtlidToHWND. */
        else if ([view isKindOfClass:[NSButton class]] && ByTitle.count(((NSButton*)view).title)) {
            NSButton* button = (NSButton*)view;
            [button setTag:ByTitle[button.title]]; //Install numeric Windows resource_id.
            [button setTarget: self];    //should be redundant for current controls
            [button setAction: @selector(activeButton:)];  //but this will be different!
        }
        else if ([view isKindOfClass:[NSMatrix class]]) {
            NSMatrix * matrix = (NSMatrix*)view;
            for (NSButtonCell* cell in matrix.cells) {
                if ([self maybeRegisterView: (NSView*)cell])
                    [cell setState: NO];  // not clear if this be useful
            }
        }
    }
}

-(void)createControlMap
{
    [self createControlMapRecurse:self.window.contentView];
 
    for (auto [nss, rid] : RIDValMap)
        if (!CtlidToHWND.count(rid))
            [self barf: FormatString("Did not find control for resource id %d (%s)", rid, nss.UTF8String)];
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
    //checkboxes report as "NSButton", but radios as "NSButtonCell"
    assert([view isKindOfClass:[NSButtonCell class]] |[view isKindOfClass:[NSButton class]]);
    NSButton* button = (NSButton*)view; // don't understand class structure
    return [button state] ? true : false;
}
-(HWND)GetControlHWND:(NSInteger)ctlid
{
    if (CtlidToHWND.count(ctlid) == 0)
        [self barf: FormatString("Received action from Cocoa control tagged %d, for which no emulation exists.", ctlid)];
    return CtlidToHWND[ctlid];
}
-(NSView*)GetControlView:(NSInteger)ctlid
{
    return getHWNDView([self GetControlHWND: ctlid]);
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

-(GenericWindlgController*)initWithNibObjectAndRIDs:(NSString*)nibName
                                             object:(void*)object
                                               rids:(RIDVector&)rids
{
    /* Make the RID map. It is worth it because it is going to be searched
    n times (n = number of entries) when dialog is recursively-scanned */

    for (auto p : rids)
        RIDValMap[[NSString stringWithUTF8String:p.Symbol]] = p.resource_id;

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
    /* The control's "tag" is the Windows resource.h command code for the control */
    /* Might not be NSControl (e.g., for radio-buttons), but if it supports "tag", good enough */

    auto tag = [sender tag];
    assert(tag != 0);
    [self reflectCommand:tag];
}
-(void)EnableControl:(NSInteger)ctl_id yesNo:(NSInteger)yesNo;
{
    NSView* view = [self GetControlView:ctl_id];
    assert([view isKindOfClass:[NSControl class]]);
    NSControl* control = (NSControl*)view;
    [control setEnabled: yesNo ? YES : NO];
}
@end

/* Can't use static STL -- load time initializes it at unpredictable time compared to our registrees */
/* Can't use structs, either, as funargs get gc'ed if not protected by strong pointer */
/* This is a lot easier. */

/* solution found 3-24-2022 -- create it first time called. */
typedef std::unordered_map<unsigned int, TLEditDlgCreator> t_TabulaCreatorum;
static t_TabulaCreatorum *TabulaCreatorum = nullptr;

int RegisterTLEDitDialog(unsigned int resource_id,  TLEditDlgCreator creator) {
    if (TabulaCreatorum == nullptr)
        TabulaCreatorum = new t_TabulaCreatorum;
    (*TabulaCreatorum)[resource_id] = creator;
    return 0;
}

void MacDialogDispatcher(unsigned int resource_id, void * obj) {
    if ((*TabulaCreatorum).count(resource_id) == 0)
        return;  /*this happens sometimes -- clicking on odd things -- just ignore */
    (*TabulaCreatorum)[resource_id](obj);
};
