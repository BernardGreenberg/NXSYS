//
//  Created by Bernard Greenberg on 9/20/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.

#import "GenericWindlgController.h"
#include "resource.h"

#ifndef IDC_EDIT_SEG_NSEGS
#define IDC_EDIT_SEG_NSEGS 1041  //WTF is wrong!?
#endif

@interface TrackSegDlg : GenericWindlgController
@end

static RIDVector rids {
    {"IDC_EDIT_SEG_TC", IDC_EDIT_SEG_TC},
    {"IDC_EDIT_SEG_NSEGS", IDC_EDIT_SEG_NSEGS}
};
DEFINE_WINDLG_WITH_CLASS(IDD_SEG_ATTRIBUTES, TrackSegDlg, @"TrackSegProperties", rids);

@implementation TrackSegDlg

- (IBAction)ShowCircuit:(id)sender {
    [self reflectCommandParam:IDC_EDIT_SEG_SHOW_TC lParam:0];
    NSTimer* myTimer = [NSTimer
                        scheduledTimerWithTimeInterval:(NSTimeInterval)2
                                 target:self
                                selector:@selector(TDispTimer:)
                                userInfo:nil
                                 repeats:FALSE];
    //yogi dharma. -- http://www.cocoabuilder.com/archive/cocoa/145851-nstimer-not-firing.html
    [[NSRunLoop currentRunLoop] addTimer:myTimer forMode:NSModalPanelRunLoopMode];
}
-(void)TDispTimer:(NSTimer*)timer
{
    //Apparently, the timer holds a strongptr to the dialog
    //(close the dlg in the 2 sec and the tseg still shuts off).
    // so we have to deal with deleting the TSEG out from under
    // the (unprotected) timer.  Quelle cirque!
    [self reflectCommandParam:IDC_EDIT_SEG_SHOW_TC lParam:1];
}

@end

