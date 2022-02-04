//
//  TrainDlgController.m
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/5/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import "TrainDlgController.h"
#import "AppDelegate.h"
#include "traindlg.h"
#include <map>
#include "WinMacCalls.h"

void sendTrainCommand(HWND, int cmd);
void sendTrainSlider (HWND, int of100);

@interface TrainDlgController ()
{
    void* NXSYSTrain;
    bool initial_observant;
    std::map<NSInteger,HWND> CtlidToHWND;
    NSInteger trainNo;
}
@end

@implementation TrainDlgController
#if 0  //für den Unglaubigen
-(void)dealloc
{
    printf ("train %s dealloc\n", self.window.title.UTF8String);
}
#endif
-(void)checkRadioButton:(NSInteger)ctlid first:(NSInteger)first last:(NSInteger)last
{
}
-(void)EnableControl:(NSInteger)ctl_id yesNo:(NSInteger)yesNo
{
    if (ctl_id == TRD_REV) {
        [_Reverse setEnabled:yesNo];
    }
}

-(HWND)GetControlHWND:(NSInteger) ctlid
{
    assert(CtlidToHWND.count(ctlid));
    return CtlidToHWND[ctlid];
}
-(void)SetControlText:(NSInteger)ctlid text:(NSString*)text
{
    assert (text != nil);
    NSView* view = getHWNDView([self GetControlHWND:ctlid]);
    assert([view isKindOfClass:[NSTextField class]]);
    [(NSTextField*)view setStringValue:text];
}
-(long)GetControlText:(NSInteger)ctlid buf:(char *)buf length:(long)length
{
    NSView* view = getHWNDView([self GetControlHWND:ctlid]);
    assert([view isKindOfClass:[NSTextField class]]);
    NSString * string = [(NSTextField *)view stringValue];
    const char * utf8 = string.UTF8String;
    size_t len = strlen(utf8); // not same as string.length
    assert (len < length);
    strcpy(buf, utf8);
    return len;
}
-(void)setFreeWill
{
     [_theMatrix selectCellWithTag:2];
}
- (id) initWithTrain:(void*)v observant:(bool)observant trainNo:(NSInteger)trainNumber
{
    if ( ! (self = [super initWithWindowNibName: @"TrainDlg"]) )
    {
        NSLog(@"NIB init failed for TrainDialog");
        return nil;
    }
    trainNo = trainNumber;
    NXSYSTrain = v;
    initial_observant = observant;
    return self;
    
} // end init

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {
        // Initialization code here.
    }
    return self;
}
-(void)fabricateCHWND:(NSView*)view ctlid:(NSInteger)ctlid
{
    NSString * descrip = [[NSString alloc] initWithFormat:@"Train dlg #%ld", trainNo];
    CtlidToHWND[ctlid] = WinWrapControl(self, view, ctlid, descrip);
}
- (void)windowDidLoad
{
    [super windowDidLoad];
    if (!initial_observant) {
        [self setFreeWill];
    }
    [self.window setTitle:[[NSString alloc] initWithFormat: @"Train #%ld", trainNo]];
    [self fabricateCHWND:_TrainName ctlid:TRD_TRAIN_ID];
    [self fabricateCHWND:_TrainSpeed ctlid:TRD_SPEED];
    [self fabricateCHWND:_TrainLocation ctlid:TRD_LOC];
    [self fabricateCHWND:_NextSignal ctlid:TRD_NEXT_SIG_NAME];
    [self fabricateCHWND:_NextAspect ctlid:TRD_NEXT_SIG_STATE];
    [self fabricateCHWND:_LastSignal ctlid:TRD_LAST_SIG_NAME];
    [self fabricateCHWND:_LastAspect ctlid:TRD_LAST_SIG_STATE];
    [self fabricateCHWND:_TrainLength ctlid:TRD_LENGTH];
}
- (IBAction)SliderAction:(id)sender {
    sendTrainSlider(NXSYSTrain, (int)[_Slider integerValue]);  // of 100
}
-(void)setSliderValue:(NSInteger)ctlid valof100:(NSInteger)of100
{
    [_Slider setIntegerValue:of100];
}
-(void)showControl:(NSInteger)ctl_id showYes:(NSInteger)yesno
{
    if (ctl_id != TRD_COPB)  // we serve a very exclusive clientele.
        return;
    BOOL v = yesno ? NO : YES;
    [_COPB setHidden:v];
    [_COText setHidden:v];
}
-(void)forwardCommand:(int)command
{
    sendTrainCommand(NXSYSTrain, command);
}
- (IBAction)newFreeWill:(id)sender {
    [self forwardCommand:TRD_DEFIANT];
}
- (IBAction)newObservant:(id)sender {
    [self forwardCommand:TRD_OBSERVANT];
}
- (IBAction)Halt:(id)sender {
    [self forwardCommand:TRD_HALT];
    [self setFreeWill];
}
- (IBAction)Kill:(id)sender {
    [self forwardCommand:TRD_KILL];
}
- (IBAction)Reverse:(id)sender {
    [self forwardCommand:TRD_REV];
}
- (IBAction)COPB:(id)sender {
    [self forwardCommand:TRD_COPB];
}


-(void)DestroyWindow
{
    [self.window orderOut:nil];

    for (auto& iterator : CtlidToHWND) {
        // note that we are NOT erasing map entry here, but in clear(); No skip-step needed.
        DeleteHwndObject(iterator.second);
    }
    CtlidToHWND.clear();

    //We are called by ::DestroyWindow, (called by Train::~Train), which
    //former is subsequently going to call DeleteHWNDObject,
    //which is going to delete exactly that and void the strongptr
    //to the controller, which is (provably) going to dealloc this instance.
}


@end

void RunTrainDlgModalTest() {
    TrainDlgController * train = [[TrainDlgController alloc] init];
    [train showWindow:nil];
    [NSApp runModalForWindow:train.window];
}

void * MacCreateTrainDialog(void* v, int id, bool observant) {
    TrainDlgController * train = [[TrainDlgController alloc]
                                  initWithTrain:v
                                  observant:observant
                                  trainNo:id];
    HWND hWnd = WinWrapRetainController(train, nil, [[NSString alloc] initWithFormat:@"Traindlg #%d", id]);
    WrapSetChildOfMain(hWnd);
    return hWnd;
}

