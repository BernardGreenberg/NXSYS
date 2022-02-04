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

std::string GetDlgItemText(HWND hWnd, int control_id) {
    char buff[128];
    UINT len = GetDlgItemText(hWnd, control_id, buff, 128);
    if (len == 0)
        return "";
    char * cp = buff;
    while (*cp == ' ')
        cp++;
    if (*cp != '\0') {
        char* endp = buff+len;
        while (endp[-1] == ' ')
            endp--;
        *endp = '\0';
    }
    return cp;
}

void SetWindowText(HWND hWnd, const std::string& text) {
    SetWindowText(hWnd, text.c_str());
}
