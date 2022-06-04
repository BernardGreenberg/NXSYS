//
//  MacAppwinAPIs.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 6/3/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#ifndef MacAppwinAPIs_h
#define MacAppwinAPIs_h

void Mac_GetDisplayWPOrg(int[2], bool really_get_it_from_window);
void DisplayStatusString(const char * s);
void EnableCommand(unsigned int, bool);
void QuitMacApp();
void ExtSaveDocumentMac();
void MacFileOpen();
void MacTLEditHelp();

void AppCommand(unsigned int);
void InitTLEditApp(int w, int h);
BOOL ReadItKludge(const char *);
bool SaveItForReal(const char * path);
void ClearItOut();
void SetMainWindowTitle(const char * text);

#endif /* MacAppwinAPIs_h */
