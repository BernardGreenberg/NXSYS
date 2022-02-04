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

std::string GetDlgItemText(HWND hWnd, int control_id);
void SetDlgItemText(HWND hWnd, int control_id, const std::string& text);
void SetWindowText(HWND hWnd, const std::string& title);
#endif /* WinApiSTL_h */
