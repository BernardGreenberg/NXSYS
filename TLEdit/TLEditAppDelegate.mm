//
//  TLEditAppDelegate.mm
//  TLEdit
//
//  Created by Bernard Greenberg on 9/16/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import "TLEditAppDelegate.h"
#import "ToolbarController.h"
#import "CanvasExtentDialogController.h"
#include <string>
#include <filesystem>
#include "tlecmds.h"
#include "DesignWindowDims.h"
#include <string>
#include "resource.h"
#include "MessageBox.h"
#include "ShiftLayoutDialog.h"
#include "HelpController.h"
#include "LayoutModified.h"
#include "MacAppwinAPIs.h"
#include "AppBuildSignature.h"
#include "CustomAboutController.h"

namespace fs = std::filesystem;

static NSString * LastPathnameKey = @"LastInterlockingEditPathname";
static NSArray *allowedTypes = [NSArray arrayWithObject:@"trk"];
#define APDTRACE(x)

TLEditAppDelegate* getTLEDelegate() {
    NSApplication * app = [NSApplication sharedApplication];
    TLEditAppDelegate * tlad = (TLEditAppDelegate*)[app delegate];
    return tlad;
}
@interface TLEditAppDelegate ()
{
    NSString * currentFileName;
    bool did_finish_launching;
    id eventMonitor;
}
@end

@implementation TLEditAppDelegate

-(id)init {
    self = [super init];
    eventMonitor = nil;
    return self;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
    return YES;
}
-(BOOL)modCheck:(const char *) activity
{
    if (!IsLayoutModified())
      return TRUE;
    
    std::string msg =  "This layout has been modified since it was last read in or written out.  "
    "Do you really want to ";
    msg += activity;
    msg += " and discard your current modifications?";
    return (MessageBox(NULL, msg.c_str(), "TLEdit application -- READ CAREFULLY!",
                       MB_YESNOCANCEL | MB_ICONEXCLAMATION) == IDYES);
}
-(void)windowWillClose:(NSNotification*) note
{
    [_toolbar close];
    _toolbar = nil;
    
}
-(void)SaveDefaultPath:(NSURL*)pathurl
{
    APDTRACE(("Save Default path: %s\n", pathurl.path.UTF8String));
    NSUserDefaults * dfts = [NSUserDefaults standardUserDefaults];
    [dfts setURL:pathurl forKey:LastPathnameKey];
    currentFileName = pathurl.path;
}
-(void)MacOnSuccessfulLayoutLoad:(NSString *)fileName
{
    
    NSURL* url = [NSURL fileURLWithPath:fileName];
    APDTRACE(("On successful load: saving SaveDefaultPath, path for reload: %s\n", fileName.UTF8String));
    [self SaveDefaultPath:url];
    SetMainWindowTitle(fileName.UTF8String);
    if (_wporg_set) {
        [_theView scrollPoint:_wporg];
    }

    APDTRACE(("Successful load final.\n"));
}
- (BOOL) readLayout:(NSString*)fileName {
    
    const char* fnu8 = [fileName UTF8String];
    APDTRACE(("ReadLayout: %s\n", fnu8));
    _wporg_set = false;
    _wporg = NSMakePoint(0,0);
    if (ReadItKludge(fnu8)) {
        [self MacOnSuccessfulLayoutLoad:fileName];
        return TRUE;
    }
    [self SaveDefaultPath: nil];
    return FALSE;
}

-(void)changeFont:(id)sender
{
   // printf("change font sent to app delegate\n");
    
}
-(void)setUpEventMonitor
{
//http://www.ideawu.com/blog/2013/04/how-to-capture-esc-key-in-a-cocoa-application.html
    //block funarg closure magic
    NSEvent* (^handler)(NSEvent*) = ^(NSEvent *theEvent) {
        DragonAbortOnChar();
        return theEvent;
    };
    eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:handler];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    APDTRACE(("Entering didFinishLaunching\n"));
    [self createTLEToolbar];
    [self setUpEventMonitor];
}
- (void) createTLEToolbar
{
    APDTRACE(("entering createTLEToolbar"));
    _toolbar = [[ToolbarController alloc] initWithWindowNibName:@"Toolbar"];
   [_toolbar showWindow:self];
}
- (void)applicationWillFinishLaunching:(NSNotification *)aNotification
{
    [self.window setDelegate:self];
    [_theScrollView setBackgroundColor:[NSColor blackColor]];
    _theView.theScrollView = _theScrollView;

    InitTLEditApp(NXSYS_DESIGN_WINDOW_DIMS::WIDTH, NXSYS_DESIGN_WINDOW_DIMS::HEIGHT);
// was    InitTLEditApp(1280,960); -- "horsey" objects easier to edit, but aux key spacing looks wrong.
    did_finish_launching = true;
    APDTRACE(("willFinish Launching entered.\n"));
    currentFileName = nil;

    NSUserDefaults * dfts = [NSUserDefaults standardUserDefaults];
    NSURL *url = [dfts URLForKey:LastPathnameKey];
    if (url != nil) {
        APDTRACE(("Read dfts LastIntlkgPath: %s\n", url.path.UTF8String));
        [self readLayout:[url path]];
    }
    APDTRACE(("willFinishLaunching final.\n"));
}

-(IBAction)HandleFileOpen:(id)sender {
    if (![self modCheck:"open a new file"])
        return;
    APDTRACE(("Handle menu file open with dlg.\n"));
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    
    [openDlg setCanChooseFiles:YES];
    [openDlg setCanChooseDirectories:NO];
    [openDlg setAllowedFileTypes:allowedTypes];
    [openDlg setTitle:@"TLEdit - Choose .trk layout def, not top-level"];

    // Display the dialog.  "If OK", process the file.
    if ( [openDlg runModal] == NSModalResponseOK )
    {
        [openDlg close];
        NSURL* url = [openDlg URL];
        openDlg = nil;
        NSString* fileName = [url path];
        if ([self readLayout:fileName]) {
            APDTRACE(("Open File with dlg saving recent: %s\n", fileName.UTF8String));;
            [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:url];
            NSUserDefaults * dfts = [NSUserDefaults standardUserDefaults];
            [dfts setURL:url forKey:LastPathnameKey];
        }
    }
}
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename {
    APDTRACE(("App open file 0: %s\n", filename.UTF8String));
    if (![self modCheck:"open the new file"])
        return YES; // don't give silly "no type" msg
    if (!did_finish_launching) {
        [self createTLEToolbar]; // didFinishLaunching requires notif, don't have.
    }
    APDTRACE(("application:(openFile): %s\n", filename.UTF8String));
    [self readLayout:filename];
    return YES; // return 'yes' unconditionally.
}
-(void)heartOfSave:(NSString *)path
{
    if (SaveItForReal(path.UTF8String)) {
        SetMainWindowTitle(path.UTF8String);
        NSURL* url = [NSURL fileURLWithPath:path];
        [self SaveDefaultPath:url];
        
    } else {
        MessageBox(NULL, "Unable to save file.", "TLEdit Application", MB_OK);
    }
}
-(IBAction)newDocument:(id)sender
{
    if ([self modCheck:"clear for a new document"]) {
        ClearItOut();
        currentFileName = nil;
        SetMainWindowTitle("New layout");
        [_theView setNeedsDisplay:YES];
        [[[CanvasExtentDialogController alloc] init] showModal:YES];
    }
}
-(IBAction)saveDocument:(id)sender
{
    if (!IsLayoutModified()) {
        MessageBox(NULL, "The layout has not been modified since last save (or read-in). Will not save.", "TLEdit Application", MB_OK | MB_ICONEXCLAMATION);
    }
    else if (currentFileName == nil) {
        [self saveDocumentAs:self];
    } else {
        [self heartOfSave:currentFileName];
    }
}
-(NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    if ([self modCheck:"exit the application"]) {
        return NSTerminateNow;
    } else {
        return NSTerminateCancel;
    }
}
-(void)applicationWillTerminate:(NSNotification*) notification {
    [NSEvent removeMonitor:eventMonitor];
}
-(BOOL)windowShouldClose:(id)Sender
{
    if ([self modCheck:"close this window"]) {
        ClearLayoutModified();  // don't complain a second time.
        return TRUE;
    }
    return FALSE;
}
-(IBAction)saveDocumentAs:(id)sender
{

    NSSavePanel * savePanel = [NSSavePanel savePanel];
    if (currentFileName != nil) {
        auto cfnfs = fs::path([currentFileName UTF8String]);
        std::string name = cfnfs.filename().string();
        std::string dir = cfnfs.parent_path().string();
        [savePanel setNameFieldStringValue:[NSString stringWithUTF8String:name.c_str()]];
        [savePanel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:dir.c_str()]]];
    }
    [savePanel setAllowedFileTypes:allowedTypes];
    [savePanel setTitle:@"Save layout to .trk file"];
    [savePanel setCanCreateDirectories:FALSE];
    [savePanel setCanSelectHiddenExtension:FALSE];
    NSInteger result = [savePanel runModal];
    if (result == NSModalResponseOK) {
        [self heartOfSave: savePanel.URL.path];
    }
}
-(IBAction)toggleToolbarShown:(id)sender
{
    static bool visible = true; /* asking it seems difficult */
    if (visible) {
        [_toolbar close];
    } else {
        [_toolbar showWindow:self];
    }
    visible = !visible;
}

-(void)statusDisplay:(NSString *)s
{
    [_StatusBar setStringValue:s];
}
-(void)ensureHelp
{
    if (_helpController == nil) {
        _helpController = [[HelpController alloc] init];
        [[_helpController window] setTitle:@"TLEdit help"];
    }
}

/* Actions from App Menu */
- (IBAction)ZoomOut:(id)sender {
    [_theView ZoomOut:sender];
}
- (IBAction)ZoomIn:(id)sender {
    [_theView ZoomIn:sender];
}
- (IBAction)Help:(id)sender {
    [self ensureHelp];
    [_helpController PDFHelp:@"TLEDocumentation/TLEdit" tag:nil];
}
- (IBAction)MacHelp:(id)sender
{
    [self ensureHelp];
    [_helpController HTMLHelp:@"TLEDocumentation/MacTleHelp" tag:nil];
}
- (IBAction)FixView:(id)sender
{
    AppCommand(IDM_FIX_ORIGIN);
}
- (IBAction)Cut:(id)sender {
    AppCommand(CmCut);
}
- (IBAction)Properties:(id)sender {
    AppCommand(CmEditProperties);
}
- (IBAction)Undo:(id)sender {
    AppCommand(CmUndo);
}
- (IBAction)Redo:(id)sender {
    AppCommand(CmRedo);
}
-(IBAction)ShiftLayout:(id)sender {
    [[[ShiftLayoutDialog alloc] init] showModal];
}
-(IBAction)SetCanvasExtent:(id)sender
{
    [[[CanvasExtentDialogController alloc] init] showModal:NO];
}
-(IBAction)toggleInsulate:(id)sender
{
    AppCommand(CmIJ);
}
-(IBAction)About:(id)sender {
    if (_about_dialog == nil)
        _about_dialog = [[CustomAboutController alloc] init];
    [_about_dialog Show:_window];
}
-(void)setTheWporg:(NSPoint)point
{
    _wporg = point;
    _wporg_set = true;
}
-(NSPoint)getTheWporg:(bool)really_get_it_from_window
{
    if (really_get_it_from_window)
        return [_theScrollView documentVisibleRect].origin;
    else
        return _wporg;
}
-(void)setUrMenu:(NSMenuItem*) item str:(const char *)sp dft:(NSString*)deft
{
    if (sp == nullptr) {
        [item setTitle: deft];
        [item setEnabled: NO];
    } else {
        [item setTitle: [NSString stringWithUTF8String:sp]];
        [item setEnabled: YES];
    }
}
-(void)setUndoMenu:(const char*)undo Redo:(const char *)redo
{
    [self setUrMenu: _UndoMenuItem str:undo dft:@"Undo"];
    [self setUrMenu: _RedoMenuItem str:redo dft:@"Redo"];
    [_SaveMenuItem setEnabled: (undo == nullptr) ? NO : YES];
}
- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
    return [menuItem isEnabled];
}
@end

NSView* getMainView() {
    return getTLEDelegate().theView;
}

NSWindow* getMainWindow() {
    return getTLEDelegate().window;
}

NSWindow* getNXWindow() {
    return getMainWindow();
}

/* These are all called from the C++ portable core */
void DisplayStatusString (const char * s) {
    NSString *ns = [[NSString alloc] initWithUTF8String:s];
    [getTLEDelegate() statusDisplay:ns];
}

/* Is called from above, too. */
void SetMainWindowTitle(const char * text) {
    AppBuildSignature ABS;
    ABS.Populate();
    std::string temp = text;
    temp += " — " + ABS.TotalBuildString();
    NSString* title = [[NSString alloc] initWithUTF8String:temp.c_str()];
    [getMainWindow() setTitle:title];
}

void SetUndoRedoMenu(const char * undo, const char* redo) {
    [getTLEDelegate() setUndoMenu:undo Redo:redo];
}

void Mac_SetDisplayWPOrg(long x, long y) {
    [getTLEDelegate() setTheWporg:NSMakePoint(x, y)];
}
void Mac_GetDisplayWPOrg(int coords[2], bool really_get_it_from_window) {
    NSPoint p = [getTLEDelegate() getTheWporg:really_get_it_from_window];
    coords[0] = (int)p.x;
    coords[1] = (int)p.y;
}

void QuitMacApp() {
    NSApplication* app =  [NSApplication sharedApplication];
    [app terminate:nil];
}

void ExtSaveDocumentMac() {
    TLEditAppDelegate* delegate = getTLEDelegate();
    [delegate saveDocument:nil];
}

void MacFileOpen() {
    TLEditAppDelegate* delegate = getTLEDelegate();
    [delegate HandleFileOpen:nil];
}

void MacTLEditHelp() {
    TLEditAppDelegate* delegate = getTLEDelegate();
    [delegate Help:nil];
}
