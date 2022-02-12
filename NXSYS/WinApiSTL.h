//
//  WinApiSTL.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/21/19.
//  Copyright © 2019 BernardGreenberg. All rights reserved.
//

#ifndef WinApiSTL_h
#define WinApiSTL_h

#include <string>

static inline void SetDlgItemTextS(HWND hWnd, UINT control_id, const std::string& s) {
	SetDlgItemText(hWnd, control_id, s.c_str());
}
static inline void SetWindowTextS(HWND hWnd, const std::string& s) {
	SetWindowText(hWnd, s.c_str());
}
static std::string GetDlgItemText(HWND hWnd, int control_id) {
    char buff[128];
    UINT len = GetDlgItemText(hWnd, control_id, buff, 128);
    if (len == 0)
        return "";
    char* cp = buff;
    while (*cp == ' ')
        cp++;
    if (*cp != '\0') {
        char* endp = buff + len;
        while (endp[-1] == ' ')
            endp--;
        *endp = '\0';
    }
    return cp;
}
#endif /* WinApiSTL_h */
