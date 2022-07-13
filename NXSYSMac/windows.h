//
//  windows.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/15/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

/*  This header file has to be included by all portable modules instead of real windows.h
    It must not exist in the real Windows build, hence the check below.
 */

#ifndef NXSYSMac
#error NXSYSMac is not defined!  "You're in the wrong place, my friend, you'd better leave."
#endif

#ifndef NXSYSMac_windows_h
#define NXSYSMac_windows_h

#ifndef NULL
#define NULL 0
#endif

#define XTG 1   //"extended track graphics"
#define NXV2 1  //this is an implementation of NXSYS, Version 2

#ifdef TLEDIT
#define PRODUCT_NAME "TLEdit"
//#define MACDONT
//typedef signed char BOOL;
#else
#define PRODUCT_NAME "NXSYS/Mac"
#endif

#ifndef REALLY_NXSYS
#ifndef TLEDIT
#define REALLY_NXSYS 1   /* this means "not TLEDIT" */
#endif
#endif

struct RECT {
    int left, right, top, bottom;
};

struct POINT {
    int x;
    int y;
};

#include "WinMouseDefs.h"


#define LOWORD(x)  ((x) & 0xFFFF)
#define HIWORD(x) (((x) >> 16) & 0xFFFF)
#define RGB(x,y,z) (COLORREF)(((x) << 16) | ((y) << 8) | (z))
#define MAXPATH 512
#define MAX_PATH 512

#define WNDPROC_DCL int
#define DLGPROC_DCL BOOL
#define PASCAL
#define _export
#define CALLBACK

#include "MessageBox.h"

typedef void* HWND;
typedef unsigned WORD;
typedef unsigned DWORD;
typedef unsigned long ULONG;
typedef WORD *PWORD;
#ifndef MACDONT  /* major conflict with Cocoa */
typedef bool BOOL;
#endif
struct __DC_;
typedef struct __DC_ * HDC;
typedef  void* HMENU;
typedef const char * LPCSTR;
typedef char * LPSTR;
typedef unsigned int UINT;
typedef unsigned long LRESULT;
typedef WORD *LPWORD;
typedef void * HKEY;
typedef void * HFONT;
typedef void * HBRUSH;
typedef void * HCURSOR;
typedef void * HPEN;
typedef void * HACCEL;
typedef void * HINSTANCE;
typedef void * HICON;
typedef void * HGDIOBJ;
typedef void * HACCEL;
typedef void * HANDLE;
typedef void MSG;
typedef int COLORREF;
typedef int WPARAM;
typedef long LPARAM;

typedef int DLGPROC(HWND, int, int, int);
typedef void TIMERPROC (HWND, UINT, UINT, DWORD);


struct LOGFONT {
    int lfHeight, lfWeight, lfWidth;
    char lfFaceName[32];
    BOOL lfItalic; // bold is part of weight.
};


struct PAINTSTRUCT {  // this should not get used.
    void * Voider;
    void * Voider2;
    RECT rcPaint;
};



HCURSOR SetCursor(HCURSOR);
HCURSOR LoadCursor(HINSTANCE, int);
HICON LoadIcon(HINSTANCE, int);
HICON LoadIcon(HINSTANCE, const char *);
HACCEL LoadAccelerators(HINSTANCE, int);
HACCEL LoadAccelerators(HINSTANCE, const char *);
HDC GetDC(HWND);
HDC ReleaseDC(HWND,HDC);
void DeleteMenu(HMENU, int, int);
void EnableMenuItem(HMENU, int, int);
void ClientToScreen(HWND,POINT*);
void ScreenToClient(HWND,POINT*);
void InvalidateRect(HWND, RECT*, int);
void SetFocus(HWND);
void ShowWindow(HWND, int);
void LineTo(HDC, int, int);
void MoveTo(HDC, int, int);
void MoveToEx(HDC, int, int, void*);
void SelectObject(HDC, void*);
void* GetStockObject(void*);
#ifndef MACDONT  /* conflict with Cocoa primitives */
void Polygon(HDC, POINT*, int);
#endif
void Ellipse(HDC, int left, int top, int right, int bottom);
int DrawText(HDC, const char *, unsigned long, RECT*, int);
void SetScrollPos(HWND, int, int, int);
void DebugBreak();
HWND GetDlgItem(HWND, int);
UINT GetDlgItemText(HWND, int, char *, int); // not const!
void SetDlgItemText(HWND, int, const char *);
int  GetDlgItemInt(HWND, int, BOOL*, BOOL);
void SetDlgItemInt(HWND, int, int, BOOL);
BOOL IsDialogMessage(HWND, MSG*);
void DestroyWindow(HWND);
int GetScrollPos(HWND, int);
int SetScrollRange(HWND, int, int, int, BOOL);
void SetWindowText(HWND, const char *);
void GetWindowRect(HWND, RECT*);
void GetClientRect(HWND, RECT*);
HWND GetDesktopWindow();
void EnableWindow(HWND, BOOL);
void MoveWindow(HWND, int, int, int, int, BOOL);
void FatalAppExit(int, const char *);
void SendDlgItemMessage(HWND, int, int, int, int);
BOOL IsIconic(HWND);
void DrawIcon(HDC, int, int, HICON);
HDC BeginPaint(HWND, PAINTSTRUCT*);
void EndPaint(HWND, PAINTSTRUCT*);
HWND CreateDialog(HINSTANCE, const char *, HWND, DLGPROC);
void FillRect(HDC, RECT*, HBRUSH);
HPEN CreatePen(int, int, int);
HBRUSH CreateSolidBrush(int);
void UpdateWindow(HWND);
HFONT CreateFontIndirect(LOGFONT *);
COLORREF GetTextColor(HDC);
void SetTextColor(HDC, COLORREF);
void SetBkColor(HDC, COLORREF);
void SetBkMode(HDC, int);
int  GetBkMode(HDC);
HMENU GetMenu(HWND);
HMENU GetSubMenu(HMENU, int);
int   GetTextExtent(HDC, const char *,   long);
void  CheckMenuItem(HMENU, int, int);
int GetMenuItemCount(HMENU);
int GetMenuItemID(HMENU, int);
void InsertMenu(HMENU, int, int, int, const char *);
void ModifyMenu(HMENU, int, int, int, const char *);
void DeleteObject(void*);
void SetCapture(HWND);
void ReleaseCapture();
void RegCloseKey(void*); // shouldn't really implement this
void KillTimer(HWND, HANDLE);
HANDLE SetTimer (HWND,  WPARAM, LPARAM, TIMERPROC*);
void LoadString(HINSTANCE, int, char *, int);
HINSTANCE GetModuleHandle(const char *);
void Rectangle(HDC, int, int, int, int);
HFONT CreateFontIndirect(LOGFONT*);
long GetTickCount();
void MacReleaseGDIOBJs();
int MacWindowsContextMenu(HWND, int resource_id, void* NXgo);
int MessageBox(void *, const char *, const char *, int);
void CheckRadioButton(HWND dlg, int first_in_group, int last_in_group, int selected);
void PostQuitMessage(DWORD);
void EndDialog(HWND, BOOL);
// not real windows
void SetDlgItemCheckState (HWND hDlg, UINT id, BOOL state);
BOOL GetDlgItemCheckState (HWND hDlg, UINT id);

#ifndef MACDONT
#define TRUE true
#define FALSE false
#endif

#define MAXEXT 64
#define IDC_WAIT 5
#define IDC_ARROW 6
#define IDC_SIZEWE 7

#define MF_BYCOMMAND 1
#define MF_BYPOSITION 2
#define MF_GRAYED 0x0100
#define MF_ENABLED 0x0200
#define MF_CHECKED 0x0400
#define MF_UNCHECKED 0x0800
#define MF_DISABLED 0x1000

#define DT_TOP  1
#define DT_BOTTOM 2
#define DT_SINGLELINE 4
#define DT_NOCLIP 8
#define DT_CALCRECT 16
#define DT_LEFT 32
#define DT_RIGHT 64
#define DT_CENTER 128
#define DT_VCENTER 256

#define SB_PAGEUP       0x0001
#define SB_PAGEDOWN     0x0002
#define SB_PAGERIGHT    0x0004
#define SB_PAGELEFT     0x0008
#define SB_LINELEFT     0x0010
#define SB_LINERIGHT    0x0020
#define SB_TOP          0x0040
#define SB_BOTTOM       0x0080
#define SB_LINEUP       0x0100
#define SB_LINEDOWN     0x0200
#define SB_HORZ         0x0400
#define SB_VERT         0x0800
#define SB_THUMBPOSITION 0x1000
#define SB_CTL          0x2000
#define SB_THUMBTRACK   0x4000

#define WM_CHAR 1004
#define WM_INITDIALOG 1504
#define WM_COMMAND 1505
#define WM_VSCROLL 1600
#define WM_HSCROLL 1601
#define WM_NOTIFY 1861 /* Tagore */
#define WM_PAINT 1900
#define WM_SETTEXT 1901
#define WM_CLOSE 1950
#define WM_INITMENU 1951
#define WM_SIZE 1953
// in mouse defs
#define WM_SETFONT 1955
#define WM_SHOWWINDOW 1956
#define WM_USER 2000


#define SW_SHOW 10000
#define SW_HIDE 10001
#define SW_SHOWNOACTIVATE 10002
#define SW_SHOWNORMAL 10003
#define SW_SHOWMINIMIZED 10004
#define SW_MINIMIZE 10005
#define SW_RESTORE 10006
#define SW_SHOWMAXIMIZED 10007

#define BM_SETCHECK 0x10000
#define BM_GETCHECK 0x10001

#define PS_SOLID 0x20001
#define PS_NULL 0x20000

#define FW_BOLD 4
#define FW_NORMAL 2

#define BRUSH_PEN_FIRST 1000
#define BLACK_BRUSH (HBRUSH)(void*)1001
#define WHITE_BRUSH (HBRUSH)(void*)1002
#define LTGRAY_BRUSH (HBRUSH)(void*)1003
#define BLACK_PEN (HPEN)(void*)1004
#define NULL_PEN (HPEN)(void*)1005

#define BRUSH_PEN_END 1006

#define SYSTEM_FONT (HFONT)(void*)2005
#define UFONT_BASE 3000  // actual handle value. 0->small are hbrush/hpen table indices.
#define UFONT_END  3512

#define OPAQUE 0x4000
#define TRANSPARENT 0x8000


int fnsplit(const char *, char *, char*, char*, char *);
extern int errno;

#endif
