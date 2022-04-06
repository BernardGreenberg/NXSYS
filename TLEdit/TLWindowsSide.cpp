//
//  TLWindowsßSide.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/19/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//


#include "windows.h"
#include "nxgo.h"

bool callWndProcGeneralMessage(HWND hWnd, void * v, int message, int wParam, long lParam) {
    GraphicObject * g = (GraphicObject*)v;
    return g->DlgProc(hWnd, message, wParam, lParam);
}

bool callWndProcInitDialog(HWND hWnd, void * v) {
    return callWndProcGeneralMessage(hWnd, v, WM_INITDIALOG, 0, 0);
}

bool callWndProcGeneralCommandParam(HWND hWnd, void * v, int command, long lParam) {
    return callWndProcGeneralMessage(hWnd, v, WM_COMMAND, command, lParam);
}
