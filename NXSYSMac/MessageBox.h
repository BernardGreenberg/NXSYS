//
//  MessageBox.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/24/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#ifndef NXSYSMac_MessageBox_h
#define NXSYSMac_MessageBox_h

#define MB_ICONSTOP 0x2000
#define MB_ICONEXCLAMATION 0x4000
#define MB_OK 0x0100
#define MB_YESNO 0x0101
#define MB_YESNOCANCEL 0x0102
#define MB_OKCANCEL 0x0103
#define MB_LOWMASK 0x03FF

#define IDYES 11
#define IDNO 10
#define IDCANCEL 800
#define IDOK 12


int MessageBox(void* hWnd, const char * message, const char * title, int flags);

#include <string>   // 21 Aug 2019
int MessageBox(void* hWnd, const std::string& message, const std::string& title, int flags);
#ifdef NXSYSMac
int MessageBoxWithImage(void* hWnd, const char * text, const char * hdr, const char * image, int flags);
#endif

#endif
