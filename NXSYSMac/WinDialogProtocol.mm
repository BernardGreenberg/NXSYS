//
//  WinDialogProtocol.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 10/29/14 from Winapi.mm
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "WinMacCalls.h"
#import "WinDialogProtocol.h"

/* Here are the implementations of all of the Windows API functions which are fronts for
   the methods on WinDialogProtocol */

static id<WinDialog> getDialogController(HWND hWnd) {
    NSWindowController* controller = getHWNDController(hWnd);
    assert([controller conformsToProtocol:@protocol(WinDialog)]);
    return (id<WinDialog>)controller;
}

HWND GetDlgItem(HWND hWnd, int ctlid) {
    return [getDialogController(hWnd) GetControlHWND:ctlid];
}

unsigned int GetDlgItemText(HWND hWnd, int ctlid, char* buf, int len) {
    return (unsigned int) [getDialogController(hWnd) GetControlText:ctlid buf:buf length:len];
}

void SetDlgItemText(HWND hWnd, int ctlid, const char * text) {
    [getDialogController(hWnd) SetControlText:ctlid text:[[NSString alloc] initWithUTF8String:text]];
}

void SetDlgItemInt(HWND hWnd, int ctlid, int value, bool fsigned) {
    [getDialogController(hWnd) SetControlText:ctlid text:[[NSString alloc] initWithFormat:@"%d", value]];
}

bool GetDlgItemState(HWND hWnd, int ctlid) {
    return [getDialogController(hWnd) getDlgItemState:ctlid] ? true : false;
}

BOOL GetDlgItemCheckState(HWND hWnd, unsigned int ctlid) {
    return [getDialogController(hWnd) getDlgItemCheckState:(int)ctlid] ? YES : NO;
}

void SetDlgItemCheckState(HWND hWnd, unsigned int ctlid, bool val) {
    [getDialogController(hWnd) setDlgItemCheckState:(NSInteger)ctlid value:(val ? YES:NO)];
}

void setSliderValue(HWND hWnd, int ctlid, int of100) {
    [getDialogController(hWnd) setSliderValue:ctlid valof100:of100];
}

void CheckRadioButton(HWND hWnd, int first_ctlid, int last_ctlid, int chosen_ctlid) {
    [getDialogController(hWnd) checkRadioButton:chosen_ctlid first:first_ctlid last:last_ctlid];
}

/* This is really "SendMessage" in meaning and function. */
int MacKludgeParam2(HWND hWnd, int param1, int param2) {
    return (int)[getDialogController(hWnd) kludgeParam2:(NSInteger)param1 param2:param2];
}

int GetDlgItemInt(HWND hWnd, int ctlid, bool*goodp, bool) {
    char buf[128];
    buf[0] = '\0';
    GetDlgItemText(hWnd, ctlid, buf, sizeof(buf)/sizeof(buf[0]));
    char * ebufp;
    long longValue = strtol(buf, &ebufp, 10);
    *goodp = (ebufp == buf + strlen(buf));
    if ((longValue < 0) || (longValue > 1000000000))
        *goodp = false;
    return (int)longValue;
}

void UpdateDlgCallbackActor(HWND hWnd, void*actor) {
    [getDialogController(hWnd) updateCallbackActor:actor];
}
