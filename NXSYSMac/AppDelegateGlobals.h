//
//  AppDelegateGlobals.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/13/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#ifndef NXSYSMac_AppDelegateGlobals_h
#define NXSYSMac_AppDelegateGlobals_h

void ScenarioHelpTitler(const char * s);
void Mac_SetDisplayWPOrg (long x, long y);
void MacDemoSay(const char * what);
void MacDemoHide();
void MacBeforeLayoutLoad();
void MacOnSuccessfulLayoutLoad(const char * fname);

// Currently in winapi.mm, probably not right.
void MacAssertTrueLayoutDims(int width, int height);

#endif
