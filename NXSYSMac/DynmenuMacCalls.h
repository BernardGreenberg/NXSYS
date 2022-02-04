//
//  DynmenuMacCalls.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/14/19.
//  Copyright © 2019 BernardGreenberg. All rights reserved.
//

#ifndef DynmenuMacCalls_h
#define DynmenuMacCalls_h

/* Calls needed by Dynamic memu system */
HWND MacCreateDynmenu(void * DynMenu);
int MacKludgeParam2(HWND hWnd, int param1, int param2);
void setFrameTopLeft(HWND hWnd, int x, int y);
void UpdateDlgCallbackActor(HWND hWnd, void* new_actor);

#endif /* DynmenuMacCalls_h */
