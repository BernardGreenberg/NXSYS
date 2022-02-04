//
//  DumbStubs.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 10/29/14 from Winapi.mm
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

/*  "Mr. Stubbs! Come here, I say!" */

#include "windows.h"


HMENU GetSubMenu(HMENU, int){
    //assert(!"GetSubMenu shouldn't be called.");
    return 0;
}

int   GetTextExtent(HDC, const char *,   long) {
    //assert(!"GetTextExtent shouldn't be called.");
    return 0;
}

void  CheckMenuItem(HMENU, int, int) {
    
}

int GetMenuItemID(HMENU, int){
    return 0;
}

int SetScrollRange(HWND, int, int, int, bool){
    return 0;
}

BOOL IsDialogMessage(HWND, MSG*){
    return FALSE;
}


int GetMenuItemCount(HMENU) {
    return 0;
}

BOOL IsDialogMsg(HWND, MSG*) {
    return FALSE;
}

void InsertMenu(HMENU, int, int, int, const char *) {
    
}
void ModifyMenu(HMENU, int, int, int, const char *) {
    
}

void SetScrollPos(HWND, int, int, int) {
    
}

void LoadString(HINSTANCE, int, char *, int) {
    
}

HINSTANCE GetModuleHandle(const char *) {
    return NULL;
}

void SendDlgItemMessage(HWND, int, int, int, int) {
    
}
BOOL IsIconic(HWND) {
    return FALSE;
}
void DrawIcon(HDC, int, int, HICON) {
    
}
HDC BeginPaint(HWND, PAINTSTRUCT*){
    return NULL;
}
void EndPaint(HWND, PAINTSTRUCT*) {
    
}
HWND CreateDialog(HINSTANCE, const char *, HWND, DLGPROC){
    return NULL;
}


HMENU GetMenu(HWND) {
    return NULL;
}


HICON LoadIcon(HINSTANCE, char const*) {
    return NULL;
}

HPEN CreatePen(int, int, int) {
    return NULL;
}


void SetCapture(HWND) {
    
}

void ReleaseCapture() {
    
}

void MoveWindow(HWND, int, int, int, int, bool) {
    
}

void UpdateWindow(HWND) {
    
}

HBRUSH CreateSolidBrush(COLORREF) {
    return NULL;
}

void SetFocus(HWND){
    
}

void ClientToScreen(HWND,POINT*p) {
    
    
}

int GetScrollPos(HWND hWnd, int cmd) {
    return 0;
}


HCURSOR SetCursor(HCURSOR) {
    return NULL;
}
HCURSOR LoadCursor(HINSTANCE, int) {
    return NULL;
}

