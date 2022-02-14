#include <windows.h>
#include <stdio.h>
#include <memory.h>
#include <ctype.h>

#include "commands.h"
#include "resource.h"
#include "lisp.h"
#include "nxsysapp.h"
#include "compat32.h"
#include "ldraw.h"
#include "dialogs.h"
#include "relays.h"
#include "nxldwin.h"
#include "usermsg.h"
#include "rlymenu.h"

static HWND S_RelayGraphicsWindow;
static const char RelayGraphicsWindow_Class [] = PRODUCT_NAME ":Relayg";

static void PlaceDrawing (HWND window) {
    if (!PlaceRelayDrawing()) {
	ClearRelayGraphics();
	if (!PlaceRelayDrawing()) {
	    MessageBox (0, "Requested relay circuit too big for "
			"current size of relay graphics window.  Make it longer and "
			"maybe NARROWER.", PRODUCT_NAME " Relay Graphics",
			MB_OK | MB_ICONSTOP);
	    return;
	}
	InvalidateRect (window, NULL, 1);
	return;
    }
    InvalidateRect (window, NULL, 0);
}

static WNDPROC_DCL RelayGraphicsWindow_WndProc
   (HWND window, unsigned message, WPARAM wParam, LPARAM lParam){

    RECT rc;
    switch (message) {

	case WM_LBUTTONDOWN:
	    if (RelayGraphicsMouse (wParam, LOWORD (lParam), HIWORD (lParam)))
		PlaceDrawing (window);
	    break;
	  
	case WM_COMMAND:

	    switch (LOWORD(wParam)) {
		case CmTrClear:
		    ClearRelayGraphics();
		    InvalidateRect (window, NULL, 1);
		    break;
		case CmPauseDemo:
		    InvalidateRect (window, NULL, 0);
		    break;
		case CmTrHide:
		    ShowWindow (window, SW_HIDE);
		    break;
		case CmShowRelayCircuit:
		    AskForAndDrawRelay(window);
		    break;
		default: break;
	    }
	    return 0;

	case WM_CHAR:
	    if ((wParam & 0x7F) == 'a')
		InvalidateRect (window, NULL, 1);
	    break;

	case WM_SIZE:

	    GetWindowRect (window, &rc);
	    DrawingSetPageSize (rc.right-rc.left, rc.bottom-rc.top,
				rc.right-rc.left, 0);
	    InvalidateRect (window, NULL, 1);
	    break;

	case WM_PAINT:
	{
	    PAINTSTRUCT ps;
	    HDC dc = BeginPaint (window, &ps);
	    SelectObject (dc, Fnt);
	    RenderRelayPage (dc);
	    EndPaint (window, &ps);
	    break;
	}
	case WM_CLOSE:
	    ShowWindow (window, SW_HIDE);
	    break;
	default:
	    return DefWindowProc (window, message, wParam, lParam);
    }
    return 0;

}

int CreateRelayGraphicsWindow () {
    RECT rc;
    GetWindowRect (GetDesktopWindow(), &rc);
    int dtw = rc.right-rc.left;
    int dth = rc.bottom-rc.top;
    int main_width = (7*dtw)/8;
    S_RelayGraphicsWindow = CreateWindow
			    (RelayGraphicsWindow_Class,
			     PRODUCT_NAME " Relay Draftsman",
			     WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER |
			     WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX,
			     /* style */
			     dtw/16 + dtw/64,dth/2,main_width, (2*dth)/3,
			     G_mainwindow, NULL, app_instance, NULL);
    if (!S_RelayGraphicsWindow){
	MessageBox (0, "Can't create Relay Graphics Window.",
		    app_name, MB_OK | MB_ICONEXCLAMATION);
	return 0;
    }
    /* don't know why create doesn't get it ...*/
    GetWindowRect (S_RelayGraphicsWindow, &rc);
    DrawingSetPageSize (rc.right-rc.left, rc.bottom-rc.top,
			rc.right-rc.left, 0);
    InvalidateRect (S_RelayGraphicsWindow, NULL, 1);
    return 1;
}


void RelayShowString (const char *rnm) {

    if (!S_RelayGraphicsWindow && !CreateRelayGraphicsWindow())
	return ;
    if (!DrawRelayFromName (rnm))
	return ;
    PlaceDrawing(S_RelayGraphicsWindow);
    ShowWindow (S_RelayGraphicsWindow, SW_SHOWNORMAL);
}

void CheckRelayDisplay() {
    if (S_RelayGraphicsWindow != NULL) {
	InvalidateRect (S_RelayGraphicsWindow, NULL, 0);
    }
}

BOOL RegisterRelayLogicWindowClass (HINSTANCE hInstance) {
    WNDCLASS klass;
    memset (&klass, 0, sizeof(klass));

    klass.style          = CS_BYTEALIGNCLIENT | CS_CLASSDC;
    klass.lpfnWndProc    = (WNDPROC) RelayGraphicsWindow_WndProc;
    klass.cbClsExtra     = 0;
    klass.cbWndExtra     = 0;
    klass.hInstance      = hInstance;
    klass.hIcon          = LoadIcon (hInstance, "RELAYICON");
    klass.hCursor        = LoadCursor (NULL, IDC_ARROW);
    klass.hbrBackground  = (HBRUSH)GetStockObject (WHITE_BRUSH);
    klass.lpszMenuName   = MAKEINTRESOURCE(IDR_RELAYMENU);
    klass.lpszClassName  = RelayGraphicsWindow_Class;

    return RegisterClass (&klass);
}



void AskForAndDrawRelay (HWND win) {
    auto p = RlyDialog(win, app_instance);
    if (p.first)
	RelayShowString (p.second.c_str());
}

void DraftsbeingCleanupForSystem() {
    CleanUpDrawing();
}

void DraftsbeingCleanupForOneLayout () {
    CleanUpDrawing();
    if (S_RelayGraphicsWindow)
	ShowWindow (S_RelayGraphicsWindow, SW_HIDE);
}

struct ListRelayStruct {
    std::vector<Relay *>Array;
    const Relay * Result;
    char Title[64]{};
};

typedef struct ListRelayStruct *pLRS;

static DLGPROC_DCL RelayListDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    pLRS S = (pLRS) GetWindowLong (hDlg, DWL_USER);
    switch (message) {
	case WM_INITDIALOG:
	{
	    SetWindowLong(hDlg, DWL_USER, (LONG) lParam);
	    S = (pLRS)lParam;
	    SetWindowText (hDlg, S->Title);
	    HWND lb = GetDlgItem (hDlg, IDC_RELAY_LIST);
	    for (auto r : S->Array) {
		std::string rs = r->RelaySym.PRep();
		const char* rp = rs.c_str();
		while (isdigit((unsigned char)*rp)) rp++;
		int ix = SendMessage (lb, LB_ADDSTRING, 0,(LPARAM)rp); //hope he copies it!
		SendMessage (lb, LB_SETITEMDATA, ix, (LPARAM)r);
	    }
	    return TRUE;
	}
	case WM_COMMAND:
	    if (wParam == IDOK) {
		goto doit;
	    }
	    else if (wParam == IDCANCEL) {
		S->Result = NULL;
		EndDialog (hDlg, FALSE);
		return TRUE;
	    }
	    else if (NOTIFY_CODE(wParam,lParam) == LBN_DBLCLK) {
doit:
		int selx = SendDlgItemMessage
			   (hDlg, IDC_RELAY_LIST, LB_GETCURSEL,0,0);
		if (selx == LB_ERR) {
		    MessageBox (hDlg, "No item selected", PRODUCT_NAME,
				MB_OK | MB_ICONEXCLAMATION);
		    return TRUE;
		}
		S->Result = (Relay*)SendDlgItemMessage
			    (hDlg, IDC_RELAY_LIST, LB_GETITEMDATA, selx,0);
		EndDialog(hDlg,TRUE);
		return TRUE;
	    }	    
	    else return FALSE;
	default:
	    return FALSE;
    }
    return FALSE;
}


Relay * ListRelaysForObjectDialog (const char * funcdesc,
				   const char * classdesc,
				   int object_number) {

    ListRelayStruct S;
    sprintf (S.Title, "%s relay for %s %d",
	     funcdesc, classdesc, object_number);
    
    S.Array = get_relay_array_for_object_number (object_number);
    if (S.Array.size() == 0) {
	usermsg ("No relays found for %s %d.", classdesc, object_number);
	return NULL;
    }
    
    DialogBoxParam (app_instance,
		    MAKEINTRESOURCE(IDD_OBJECT_RELAY_LIST),
		    G_mainwindow, RelayListDlgProc, (LPARAM) &S);
   
    return const_cast<Relay*>(S.Result);
}

void ShowStateRelay(Relay*);
void ShowStateRelaysForObject (int object_number, const char * classdesc) {
    Relay * r = ListRelaysForObjectDialog ("Show State", classdesc, object_number);
    if (r)
    ShowStateRelay (r);
}
#if 0
int ShowStateRelaysForObject(int object_number, const char* classdesc) {
    Relay* r = ListRelaysForObjectDialog("Show State", classdesc, object_number);
    if (!r)
	return 0;
    ShowStateRelay(r);
    return 1;
}
int DrawRelaysForObject (int object_number, const char * classdesc) {
    Relay * r = ListRelaysForObjectDialog ("Draw", classdesc, object_number);
    if (!r)
	return 0;
    RelayShowString(r->RelaySym.PRep().c_str());
    return 1;
}
#endif

void DrawRelaysForObject(int object_number, const char* classdesc) {
    Relay* r = ListRelaysForObjectDialog("Draw", classdesc, object_number);
    if (!r)
	return;
    RelayShowString(r->RelaySym.PRep().c_str());
}