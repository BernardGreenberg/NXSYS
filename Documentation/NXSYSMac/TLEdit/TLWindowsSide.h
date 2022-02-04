//
//  TLWindowsSide.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/19/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#ifndef NXSYSMac_TLWindowsSide_h
#define NXSYSMac_TLWindowsSide_h

bool callWndProcInitDialog(HWND hWnd, void*v);
bool callWndProcIdOK(HWND hWnd, void*v);
bool callWndProcIdCancel(HWND hWnd, void*v);
bool callWndProcGeneralCommandParam(HWND hWnd, void * v, int command,  long lparam);

#endif
