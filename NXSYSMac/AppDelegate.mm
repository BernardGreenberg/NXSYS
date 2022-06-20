//
//  AppDelegate.mm, was NXMacAppDelegate.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/14/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#import "AppDelegate.h"
#import "HelpController.h"
#include "AppDelegateGlobals.h"

#include "CustomAboutController.h"

#include "stdio.h"
#include <vector>
#include <fstream>
#include "StartShut.h"
#include "InfoController.h"
#include "StatusReport.h"
#include "RelayTracerController.h"
#include "Preferences.h"
#import "HelpController.h"
#import "MainView.h"
#import "WinMacCalls.h"
typedef void *HINSTANCE;
#include "rlytrapi.h"
#include "MessageBox.h"
#include "commands.h"
#include <filesystem>
#include "InterlockingLibrary.hpp"
#include "HelpDirectory.hpp"
#include "GetResourceDirectoryPathname.h"
#include <regex>
#include "AppBuildSignature.h"

namespace fs = std::filesystem;

#include <mach-o/dyld.h>

static NSString * FullSigKey = @"FullSignalDisplaysAreViews";
static NSString * LastPathnameKey = @"LastInterlockingPathname";
static NSString * ShowStopsKey = @"LastShowStopsPolicy";
static NSArray* allowedFileTypes = [NSArray arrayWithObject:@"trk"];

#include <stdarg.h>
#if 0
static void xlog(const char * ctlstring, ...) {
    char buf[2000];
    va_list(ap);
    va_start(ap, ctlstring);
    vsprintf(buf, ctlstring, ap);
    NSLog(@"%s", buf);
    
}
#endif
#define ESC_KEY_CODE 53

#define APDTRACE(x)
//printf x

void DropAllSignals();
void NormalAllSwitches();
void DropAllApproach();
void ClearAllTrackSecs();
void ClearAllAuxLevers();
void BobbleRGPs();
void AskForAndDrawRelay(void*);
void NXGO_ComputeTrueLayoutDimensions();

void DeInstallLayout();
void CloseAllFSWs(bool release);
void ClearHelpMenu();
bool ToggleRelayDraftsperson(bool current);
void ScenarioHelp(int n);
int StopPolicyDialog(int policy);
int ShowStopDlg(void*, void*, int);
void RunTrainDlg();
void OfferChooseTrackDlg(bool go);
void LoseChooseTrack();
void TrainMiscCtl(int cmd);
void EnableAutomaticOperation(bool);
bool AutoControlRelayExists();
void DestroyDynMenus();
bool DemoWindowFilterKey(long);
void AllAbove();

extern struct __WND_ * G_mainwindow;
extern std::string InterlockingName;
extern int EnableAutoOperation;
extern bool FullSigsAreViews;
struct RECT;
void InvalidateRect(void*, RECT*, int);
bool TestRunExprFile(const char * fileName);
void AskForAndShowStateRelay(void*);
void GDISetMainWindow(NSView*);
void Demo(const char * DemoFileName);

@interface AppDelegate ()  //supplementary declaration.
{
    __strong NSString * pathForReload;
    __strong RelayDrafterController * RelayDrafter;
    BOOL haveSetScenarioHelpItem;
    id eventMonitor;
    NSPoint wporg;
    BOOL wporg_set;
    BOOL aop;
    bool did_finish_launching; // better be initted to false!
}
@end


AppDelegate * getNXAppDelegate () {
    NSApplication * app = [NSApplication sharedApplication];
    assert(app != nil);
    return (AppDelegate *)[app delegate];
}

NSWindow * getNXWindow() {
    return getNXAppDelegate().window;
}


@implementation AppDelegate
static NSString* buildSignature;
static InterlockingLibrary interlockingLibrary;
static HelpDirectory helpDirectory;

-(id)init  // this actually gets run, proven.
{
    self = [super init];
    aop = false;
    wporg_set = false;
    did_finish_launching = false;
    eventMonitor = nil;
    haveSetScenarioHelpItem = false;
    AppBuildSignature ABS;
    ABS.Populate();
    buildSignature = [NSString stringWithUTF8String: ABS.TotalBuildString().c_str()];
    return self;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
    return YES;
}

-(void)SaveDefaultPath:(NSURL*)pathurl
{
    APDTRACE(("Save Default path: %s\n", pathurl.path.UTF8String));
    NSUserDefaults * dfts = [NSUserDefaults standardUserDefaults];
    [dfts setURL:pathurl forKey:LastPathnameKey];
}

- (BOOL) readLayout:(NSString*)fileName {

    const char* fnu8 = [fileName UTF8String];

    APDTRACE(("ReadLayout %s\n", fnu8));
    /* GetLayout calls DeInstall Layout */
    return GetLayout(fnu8, TRUE) ? TRUE : FALSE;
}
-(void)MacBeforeLayoutLoad // this is BS, find some better way to manage menu.
{
    APDTRACE(("Before Layout Load\n"));
    haveSetScenarioHelpItem = NO;
    ClearHelpMenu(); // not really menu; array of old strings.
}

-(void)LibEntryClicked:(NSEvent*)event
{
    NSMenuItem* item = (NSMenuItem*)event;
    int tag = (int)item.tag;
    InterlockingLibraryEntry& E = interlockingLibrary[tag];
    [self readLayout:[NSString stringWithUTF8String:E.Pathname.string().c_str()]];
}
-(void)HelpEntryClicked:(NSEvent*)event
{
    NSMenuItem* item = (NSMenuItem*)event;
    int tag = (int)item.tag;
    HelpDirectoryEntry& E = helpDirectory[tag];
    NSString * nss;
    if (E.isLocalPath) {
        /* Windows can't tolerate %20's in pathnames.  Mac can't tolerate spaces. @$@#$!*/
        std::string local_path = std::regex_replace
            (E.LocalPathname.string(), std::regex(" "), "%20");
        nss = [NSString stringWithUTF8String: ("file://" + local_path).c_str() ];
    }
    else
        nss = [NSString stringWithUTF8String: E.URL.c_str() ];
    NSURL * url = [NSURL URLWithString: nss];
    [self.helpController HTMLView:url];

}

-(void)PopulateHelpMenu
{
    helpDirectory = GetHelpDirectory();
    NSMenu * topLevelMenu = [[NSApplication sharedApplication] mainMenu];
    
    NSMenuItem* help_item = [topLevelMenu itemWithTitle:@"Help"];
    NSMenu* help_menu = help_item.submenu;
    int ix = 0;
    for (auto& helpe : helpDirectory) {
        NSMenuItem *item = [[NSMenuItem alloc]
                            initWithTitle:[NSString stringWithUTF8String: helpe.Title.c_str()]
                            action:@selector(HelpEntryClicked:)
                            keyEquivalent:@""];
        item.tag = ix++;
        [help_menu addItem:item];
    }
}

-(void)PopulateLibraryMenu
{
    interlockingLibrary = GetInterlockingLibrary();
    NSMenu * topLevelMenu = [[NSApplication sharedApplication] mainMenu];
    
    NSMenuItem* file_menu_item = [topLevelMenu itemWithTitle:@"File"];
    if (file_menu_item.hasSubmenu) {
        NSMenu* file_menu = file_menu_item.submenu;
        NSArray* file_menu_array = file_menu.itemArray;
        NSMenuItem* library_menu_item = file_menu_array[1];
        if (library_menu_item.hasSubmenu) {
            NSMenu* library_menu = library_menu_item.submenu;
            int ix = 0;
            for (auto& libe : interlockingLibrary) {
                NSMenuItem *item = [[NSMenuItem alloc]
                                    initWithTitle:[NSString stringWithUTF8String: libe.Title.c_str()]
                                    action:@selector(LibEntryClicked:)
                                    keyEquivalent:@""];
                item.tag = ix++;
                [library_menu addItem:item];
            }
        }
    }
}
-(void)OnSuccessfulLayoutLoad:(NSString *)fileName
{
    
    NSURL* url = [NSURL fileURLWithPath:fileName];
    APDTRACE(("On successful load: saving SaveDefaultPath, path for reload %s\n", fileName.UTF8String));
    [self SaveDefaultPath:url];
    pathForReload = fileName;
    
    NSString* title = [NSString stringWithFormat:@"%s - %@", InterlockingName.c_str(), buildSignature];
    [_window setTitle: title];

    if (!haveSetScenarioHelpItem) {
        [_ScenarioHelpItem setTitle:@""];
    }
    if (wporg_set) {
        [_theView scrollPoint:wporg];
    }
    APDTRACE(("Successful load 2\n"));
    [Preferences initPreferredSettings];
    APDTRACE(("Successful load 3\n"));
    bool azExists = AutoControlRelayExists();
    //EnableAutoOperation = azExists;
    [_AutomaticOperation setEnabled:azExists ? YES: NO];
    APDTRACE(("Successful load done\n"));
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename {
    APDTRACE(("App open file 0, %s\n", filename.UTF8String));
#if 0
    if (!did_finish_launching) {
        [self applicationDidFinishLaunching:nil]; // ours doesn' do much, "nil" rejected.
    }
#endif
    APDTRACE(("application:(openFile) %s\n", filename.UTF8String));
    if ([self readLayout:filename]) {
        return YES;
    }
    return NO;
}

-(void)windowWillClose:(NSNotification *)notification
{
    APDTRACE(("windowWillClose notification\n"));
    [self closeAuxWindows];
}
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    APDTRACE(("Really didFinishLaunching\n"));
}
- (void)applicationWillFinishLaunching:(NSNotification *)aNotification
{
    did_finish_launching = true;
    APDTRACE(("willFinish Launching entered.\n"));
    _myScrollView.backgroundColor = [NSColor blackColor];
    _theView.theScrollView = _myScrollView;

    [_demoView setFont:[NSFont fontWithName:@"Menlo" size:18]];
    [_demoView setBackgroundColor:colorFromRGB(0,200,255)];
    
    pathForReload = nil;
    
    [_window setDelegate:self];
    APDTRACE(("WillFinishLaunching 2"));
    // Why does this work at all?  The NXSYS View is already created, and its
    // init code established G_mainwindow via GDISetMainWindow.
    // (Note - misspell the linkage and there is no diagonstic.)
    StartUpNXSYS(NULL, G_mainwindow, NULL, NULL, 0);

    APDTRACE(("WillFinishLaunching 3\n"));
    [self HideDemo];

    NSMenu * topLevelMenu = [[NSApplication sharedApplication] mainMenu];
    _rlyTraceWindow = [RelayTracerController CreateWithMenu:topLevelMenu];

    APDTRACE(("willFinishLaunching 4\n"));
    EnableAutoOperation = FALSE;

    NSUserDefaults * dfts = [NSUserDefaults standardUserDefaults];
    NSURL *url = [dfts URLForKey:LastPathnameKey];
    APDTRACE(("Read dfts LastIntlkgPath %s\n", url.path.UTF8String));
    if (url != nil) {
        [self readLayout:[url path]];
    }
    [self PopulateHelpMenu];
    [self PopulateLibraryMenu];

    APDTRACE(("willFinishLaunching 5\n"));
    [self setUpEventMonitor];
    APDTRACE(("willFinishLaunching 6\n"));
}

-(void)setUpEventMonitor
{
//http://www.ideawu.com/blog/2013/04/how-to-capture-esc-key-in-a-cocoa-application.html
    //block funarg closure magic
    NSEvent* (^handler)(NSEvent*) = ^(NSEvent *theEvent) {
        NSWindow *targetWindow = theEvent.window;
        if (theEvent.keyCode == ESC_KEY_CODE
            && self.aboutController != nil && [self.aboutController isWindowVisible]) {

            [self.aboutController close];
            return (NSEvent*) nil;
        }
        if (self->_rlyTraceWindow && [self->_rlyTraceWindow TryCharTrap:(NSInteger)theEvent.keyCode]) {
            return (NSEvent*)nil;
        }
        if (targetWindow != self.window) {
            return theEvent;
        }
        // NSLog(@"event monitor: %@", theEvent);

        if (DemoWindowFilterKey(theEvent.keyCode)) {
            return (NSEvent*)nil;
        }
        return theEvent;
    };
    eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:handler];
}

- (IBAction)HandleFileOpen:(id)sender {
    APDTRACE(("Handle menu file open with dlg.\n"));
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    
    // Enable the selection of files, disable directories in the dialog.
    [openDlg setCanChooseFiles:YES];
    [openDlg setCanChooseDirectories:NO];
    [openDlg setAllowedFileTypes:allowedFileTypes];
    [openDlg setTitle:@"NXSYS - Choose .trk top-level interlocking definition"];

    // Display the dialog.  "If OK", process the file.
    if ( [openDlg runModal] == NSModalResponseOK )
    {
        [openDlg close];
        NSURL* url = [openDlg URL];
        openDlg = nil;
        NSString* fileName = [url path];
        if ([self readLayout:fileName]) {
            APDTRACE(("Open File with dlg saving recent %s\n", fileName.UTF8String));;
            [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:url];
            NSUserDefaults * dfts = [NSUserDefaults standardUserDefaults];
            [dfts setURL:url forKey:LastPathnameKey];
        }
    }
}
- (IBAction)HandleReload:(id)sender {
    APDTRACE(("RELOAD command issued.\n"));
    if (pathForReload != nil) {
        /* Whether to supply TRUE or FALSE here played key roles
         in the fetal development of this code.  There's no issue now. */
        /* GetLayout starts with DeInstallLayout */
        GetLayout(pathForReload.UTF8String, FALSE);
    }
}

- (NSApplicationTerminateReply) applicationShouldTerminate:(NSNotification *)aNotification
{
    [self closeAuxWindows];
    return NSTerminateNow;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    /* really does it, app menu quit or close main window */
    [NSEvent removeMonitor:eventMonitor];
    TrainMiscCtl(CmKillTrains); /* deinstall layout does it earlier now, but there
        have been rogue trains; more killing is better */
    [self closeAuxWindows];
    DeInstallLayout();
}

- (IBAction)ClearAllTrack:(id)sender {
    ClearAllTrackSecs();
}
- (IBAction)CancelAllSignals:(id)sender {
    DropAllSignals();
}
- (IBAction)ReleaseAllApproachLocking:(id)sender {
    DropAllApproach();
}
- (IBAction)ClearAllAuxSwitches:(id)sender {
    ClearAllAuxLevers();
}

- (IBAction)NormalAllSwitches:(id)sender {
    NormalAllSwitches();
}
- (IBAction)ClearAllTheAbove:(id)sender {
    AllAbove();
}
- (IBAction)Bobble:(id)sender {
    BobbleRGPs();
}
- (IBAction)Invalidator:(id)sender {
    InvalidateRect(G_mainwindow, NULL, 0);
}
- (IBAction)callRelayQuery:(id)sender {
    AskForAndShowStateRelay(NULL);
}
- (IBAction)FileInfo:(id)sender
{
    InterlockingFileStatus ifs;
    NSString* report = [NSString stringWithUTF8String:ifs.Report().c_str()];
    [[[InfoController alloc] init] showModal:report];
}

- (IBAction)TraceToConsole:(id)sender {
    [_rlyTraceWindow ClickTraceToConsole];
}
- (IBAction)TraceToWindow:(id)sender {
    [_rlyTraceWindow ClickTraceToWindow];
}
-(void)closeAuxWindows
{
    /* The basic contract of this fcn is to close down
     the Mac side, but random shots in the dark won't hurt. */
    TrainMiscCtl(CmKillTrains); // can't kill 'em too many times.
    LoseChooseTrack();
    [_rlyTraceWindow close];
    [RelayDrafter close]; //nil s/b ok
    CloseAllFSWs(true);  //it's known that uninstall does this.
    DestroyDynMenus();
    /* And these, too!  11 May 2021 */
    [_aboutController close]; //nil is ok!!
    [_helpController close];
}

- (IBAction)FlushFullsigWins:(id)sender {
    CloseAllFSWs(FALSE);
}
-(HelpController *)helpController
{
    if (_helpController == nil)
        _helpController = [[HelpController alloc] init];
    return _helpController;
}
- (IBAction)V2Help:(id)sender
{
    [self.helpController TextHelp:@"v2nxhelp"];
}
- (IBAction)ScenarioHelp1:(id)sender {

    [self helpController];
    ScenarioHelp(1);
}
- (IBAction)NXV1Help:(id)sender {
    [self.helpController HTMLHelp:@"Documentation/NXSYS" tag:nil];
}
- (IBAction)MacHelp:(id)sender {
    [self.helpController HTMLHelp:@"Documentation/MacNXSYS" tag:nil];
}
- (IBAction)ReleaseNotes:(id)sender {
    [self.helpController HTMLHelp:@"Documentation/ReleaseNotes" tag:nil];
}

-(void)ScenarioHelpSetTitle:(NSString*)s
{
    [_ScenarioHelpItem setTitle:s];
    haveSetScenarioHelpItem = YES;
}

-(void)SetWPOrg:(long)x y:(long)y
{
    wporg_set = YES;
    wporg = NSMakePoint(x,y);
}

- (IBAction)ZoomOut:(id)sender {
    //It should be obvious that .909090...*1.1 = .9999999 = 1.
    [_theView magnifyWithRatio:0.90909090909];
}
- (IBAction)ZoomIn:(id)sender {
    [_theView magnifyWithRatio:1.1];
}
- (IBAction)Train:(id)sender {
    OfferChooseTrackDlg(true);
}
- (IBAction)TrainStopped:(id)sender {
    OfferChooseTrackDlg(false);
}
- (IBAction)TrainHaltAll:(id)sender {
    TrainMiscCtl(CmHaltTrains);
}
- (IBAction)TrainKillAll:(id)sender {
    TrainMiscCtl(CmKillTrains);
}
- (IBAction)AutomaticOperation:(id)sender {
    EnableAutoOperation = !EnableAutoOperation;
    [_AutomaticOperation setState:(EnableAutoOperation ? NSControlStateValueOn : NSControlStateValueOff)];
    EnableAutomaticOperation(EnableAutoOperation); // Windows BOOL -> bool under iNXSYS regime.
}
-(RelayDrafterController*)getDrafter:(bool)force
{
    if (RelayDrafter == nil) {
        if (!force)
            return nil;
        RelayDrafter = [[RelayDrafterController alloc] init];
        RelayDrafter.mainMenuItem = _DraftspersonItem;
    }
    return RelayDrafter;
}
- (IBAction)DraftspersonClick:(id)sender {
    [[self getDrafter:true] toggle:sender];
}
- (IBAction)DrawRelayCmd:(id)sender {
    [[self getDrafter:true] drawNew:sender];
}
- (IBAction)Demo:(id)sender {
   
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];

    [openDlg setCanChooseFiles:YES];
    [openDlg setAllowedFileTypes: @[ @"xdo" ]];
    [openDlg setCanChooseDirectories:NO];
    [openDlg setTitle:@"Interlocking script file for demo"];
    // Display the dialog.  "If OK", process the file.
    if ( [openDlg runModal] == NSModalResponseOK )
    {
        [openDlg close];
        NSURL* url = [openDlg URL];
        openDlg = nil;
        Demo([[url path] UTF8String]);
    }

}
-(void)DemoSay:(NSString*) what
{
    [_demoView.superview.superview setHidden:NO];
    [_demoView setString:what];
}

-(void)HideDemo
{
    [_demoView.superview.superview setHidden:YES];
}

-(IBAction)Preferences:(id)sender
{
    [[[Preferences alloc] init] showModal];
}
- (IBAction)NewAbout:(id)sender {
    if (_aboutController == nil)
        _aboutController = [[CustomAboutController alloc] init];
    [_aboutController Show:_window];
}

@end

MainView* getMainView() {
    return [getNXAppDelegate() theView];
}

void ScenarioHelpTitler(const char * s) {
    NSString * ns = [[NSString alloc] initWithUTF8String:s];
    [getNXAppDelegate() ScenarioHelpSetTitle:ns];
}

void Mac_SetDisplayWPOrg (long x, long y) {
    [getNXAppDelegate() SetWPOrg:x y:y];
}

void MacDemoSay(const char * what) {
    NSString * nss = [[NSString alloc] initWithUTF8String:what];
    [getNXAppDelegate() DemoSay:nss];
}

void MacDemoHide() {
    [getNXAppDelegate() HideDemo];
}

void MacBeforeLayoutLoad() {
    [getNXAppDelegate() MacBeforeLayoutLoad];
}
void MacOnSuccessfulLayoutLoad(const char * fname) {
    NSString * nsfname = [[NSString alloc] initWithUTF8String:fname];
    [getNXAppDelegate() OnSuccessfulLayoutLoad:nsfname];
}

void HelpSystemDisplay(const char * text) {
    [getNXAppDelegate().helpController HelpSystemDisplay:text];
}

void HelpSystemDisplayURL(const char * earl) {
    NSString* tag;
    NSString* url = [NSString stringWithUTF8String:earl];
    const char * pound = strchr(earl, '#');
    if (pound) {
        tag = [url substringFromIndex:(pound-earl+1)];
        url = [url substringToIndex:(pound-earl)];
    }
    [getNXAppDelegate().helpController HTMLHelp:url tag:tag];
}

HWND getRelayDrafterHWND(bool force) {
    RelayDrafterController * wc = [getNXAppDelegate() getDrafter:force];
    if (!force && wc == nil)
        return NULL;
    if (force)
        [wc  window];
    return wc.hWnd;
}

fs::path GetResourceDirectoryPathname() {
    // https://stackoverflow.com/questions/7004401/c-find-execution-path-on-mac
    char buf [PATH_MAX];
    uint32_t bufsize = PATH_MAX;
    if(!_NSGetExecutablePath(buf, &bufsize)){
        
        fs::path exe(buf),
                 macOS = exe.parent_path(),
                 Contents = macOS.parent_path(),
                 Resources = Contents / "Resources";
        
        return Resources;
    }
    return "";
}
