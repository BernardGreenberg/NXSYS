#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include "tlecmds.h"
#include "tletoolb.h"

/* stolen from RJ, to which I have unlimited source rights */

struct _ButtonData {
    int Bmx;
    int Style;
    int Command;
    char * String;
} ButtonData []
   = {
    {0,TBSTYLE_BUTTON, CmQuit, "Exit Layout editor"},
    {13,TBSTYLE_BUTTON,CmOpen, "Open a layout file"},
    {12,TBSTYLE_BUTTON,CmSave, "Save layout to file"},

    {0,TBSTYLE_SEP, 0, NULL},

    {8,TBSTYLE_CHECK, CmToggleNSJoints, "Show/Don't show nonselected joints"},
    {19,TBSTYLE_CHECK, CmToggleExitLights, "Show/Don't show entrance/exit lights"},

    {0,TBSTYLE_SEP, 0, NULL},
    {9,TBSTYLE_BUTTON, CmCut, "Cut selected object"},
    {0,TBSTYLE_SEP, 0, NULL},
    {14, TBSTYLE_BUTTON, CmEditProperties, "Edit Properties of selected object"},
    {0,TBSTYLE_SEP, 0, NULL},
    {3,TBSTYLE_BUTTON, CmIJ, "Insulate selected joint"},
    {18, TBSTYLE_BUTTON, CmFlipNum, "Flip position of switch/joint ##"},
//    {5,TBSTYLE_BUTTON, CmSwitch, "Create a switch or crossover"},
    {16, TBSTYLE_BUTTON, CmSignalUpRight, "Create a signal facing up/right"},
    {17, TBSTYLE_BUTTON, CmSignalDownLeft, "Create a signal facing down/left"},
    {20, TBSTYLE_BUTTON, CmCreateExitLight, "Create or select exit light at this signal"},
    
    {0,TBSTYLE_SEP, 0, NULL},

    {21, TBSTYLE_BUTTON, CmAuxKey, "Create an auxiliary switch key."},
#ifndef NOTRAFLEV
    {23, TBSTYLE_BUTTON, CmTrafficLever, "Create a traffic lever."},
    {24, TBSTYLE_BUTTON, CmPanelLight, "Create a general panel light."},
    {25, TBSTYLE_BUTTON, CmPanelSwitch, "Create a general panel switch."},
#endif
    {0,TBSTYLE_SEP, 0, NULL},
    {22, TBSTYLE_BUTTON, CmText, "Create a text item."},
    {0,TBSTYLE_SEP, 0, NULL},

    {7,TBSTYLE_BUTTON, CmHelp, "Pop up a help text"},
};

#define BUTTON_DATA_COUNT (sizeof (ButtonData)/sizeof(ButtonData[0]))  
    
int HandleToolbarNotification (WPARAM wParam, LPARAM lParam) { 

    if (((LPNMHDR) lParam)->code == TTN_NEEDTEXT) {
	    LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT) lParam; 
	    int idButton = lpttt->hdr.idFrom; 
	    if (idButton != 0)
		for (int i = 0; i < BUTTON_DATA_COUNT;i++)
		    if (ButtonData[i].Command == idButton) {
			lpttt->lpszText = ButtonData[i].String;
			return 1;
		    }
    } 
    return 0;
}


HWND CreateOurToolbar (HWND hWnd, HINSTANCE app_instance) {
    InitCommonControls();
    
    TBBUTTON B [BUTTON_DATA_COUNT];
    memset (B, 0, sizeof(B));

    int bmx = 0;
    for (int i = 0; i < BUTTON_DATA_COUNT; i++) {
	B[i].fsStyle = ButtonData[i].Style;
	if (B[i].fsStyle != TBSTYLE_SEP) {
	    B[i].fsState = TBSTATE_ENABLED;
	    B[i].idCommand = ButtonData[i].Command;
	    B[i].iBitmap = ButtonData[i].Bmx;
	    if (ButtonData[i].Bmx > bmx)
		bmx = ButtonData[i].Bmx;
	}
    }

    HWND tb = CreateToolbarEx
	      ( hWnd,
		WS_CHILD | TBSTYLE_TOOLTIPS,
		10901,
		bmx+1,
		app_instance,
		IDB_MAIN_TOOLS,
		B,
		BUTTON_DATA_COUNT,
		16,16, 16,16,
		sizeof(TBBUTTON));

//    if (tb == NULL)
//	usermsg ("Can't create toolbar - error 0x%X", GetLastError());
    return tb;
}

void AutoResizeToolbar(HWND hWnd) {
    if (hWnd)
	SendMessage (hWnd, TB_AUTOSIZE, 0, 0);
}

void AssertToolbarCheckState (HWND hWnd, int cmd) {
    if (hWnd)
	SendMessage (hWnd, TB_CHECKBUTTON, (WPARAM) cmd,
		     (LPARAM) MAKELONG (1, 0));
}

void SetToolbarCheckState (HWND hWnd, int cmd, BOOL way) {
    if (hWnd)
	SendMessage (hWnd, TB_CHECKBUTTON, (WPARAM) cmd,
		     (LPARAM) MAKELONG (way, 0));
}

BOOL GetToolbarCheckState (HWND hWnd, int cmd) {
    if (hWnd)
	return (BOOL)
		SendMessage (hWnd, TB_ISBUTTONCHECKED, (WPARAM) cmd,
			     (LPARAM) 0);
    else return FALSE;
}

void EnableToolButton (HWND hWnd, int cmd, int val) {
    if (hWnd)
	SendMessage (hWnd, TB_ENABLEBUTTON, (WPARAM) cmd,
		     (LPARAM) MAKELONG (val, 0));
}

