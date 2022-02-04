#ifndef _NX_TLE_TOOLBAR_H__
#define _NX_TLE_TOOLBAR_H__

int HandleToolbarNotification (WPARAM wParam, LPARAM lParam);
HWND CreateOurToolbar (HWND hWnd, HINSTANCE app_instance);
void AutoResizeToolbar(HWND hWnd);
void AssertToolbarCheckState (HWND hWnd, int cmd);
void SetToolbarCheckState (HWND hWnd, int cmd, BOOL way);
BOOL GetToolbarCheckState (HWND hWnd, int cmd);
void EnableToolButton (HWND hWnd, int cmd, int val);

#endif
