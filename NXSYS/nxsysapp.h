#ifndef _NXSYS_APP_H__
#define _NXSYS_APP_H__
#ifndef NXSYSMac

#include <nxproduct.h>
#endif

extern HWND G_mainwindow;
extern HINSTANCE app_instance;
extern HFONT Fnt, LargeFnt;
extern char app_name[];
extern WORD WindowsMessageLoop (HWND window, HACCEL hAccel, UINT Quitmsg = 0);
void NBDSetWindowText (HWND window, const char* text);

BOOL GetLayout (const char * name, BOOL reset_viewport);
void DeInstallLayout();
void NxsysAppAbort (int reserved, const char* message);
int  ContextMenu (int resource_id);
void CleanUpNXSYS(); // for Mac impl.

HKEY GetAppKey(LPCSTR subk);
DWORD GetDWORDRegval (HKEY key, LPCSTR vname, DWORD default_value);
DWORD PutDWORDRegval (HKEY key, LPCSTR vname, DWORD value);

#define WM_NXSYS_CLOSE_TRACE_WINDOW (WM_APP+1)

#endif
