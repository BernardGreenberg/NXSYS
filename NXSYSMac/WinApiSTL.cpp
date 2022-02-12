//
//  WinApiSTL.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/21/19.
//  Copyright © 2019 BernardGreenberg. All rights reserved.
//


#include <stdio.h>
#include <string>
#include "windows.h"

#include "MessageBox.h"
#include "WinApiSTL.h"

int MessageBox(HWND hWnd, const std::string& msg, const std::string& label, int flags) {
    return MessageBox(hWnd, msg.c_str(), label.c_str(), flags);
}

void SetDlgItemText(HWND hWnd, int control_id, const std::string& text) {
    SetDlgItemText(hWnd, control_id, text.c_str());
}

void SetWindowText(HWND hWnd, const std::string& text) {
    SetWindowText(hWnd, text.c_str());
}
