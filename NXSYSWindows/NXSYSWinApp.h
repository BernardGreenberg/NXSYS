#pragma once

#include <string>


void EndChooseTrack();
void SetViewportDimsFromWindow(HWND hWnd);
void NXSYS_Command(unsigned int cmd);
void AllAbove();
void CheckMainMenuItem(int item, BOOL enabled);

extern BOOL RightButtonMenu;
extern int ChooseTrackActive;
extern std::string LayoutFileName;
extern BOOL EnableAutoOperation;
extern BOOL GotLayout;
extern const char* MainWindow_Class;

BOOL IsMenuDlgMessage(MSG* m); /*rightly win-only dynmenu */
void EnableAutomaticOperation(BOOL);

int StartUpNXSYS(HINSTANCE hInstance, HWND window, const char* initial_layout_name, const char* initial_demo_file,
    int nCmdShow);


#define NULL0(stl) ( (stl.length() == 0) ? nullptr : stl.c_str())