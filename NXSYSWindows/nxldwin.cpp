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
		default:;
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


int RelayShowString (const char *rnm) {

    if (!S_RelayGraphicsWindow && !CreateRelayGraphicsWindow())
	return 0;
    if (!DrawRelayFromName ((const char *)rnm))
	return 0;
    PlaceDrawing(S_RelayGraphicsWindow);
    ShowWindow (S_RelayGraphicsWindow, SW_SHOWNORMAL);
    return 1;
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


static Relay * GetSelRelayFromListDlg (HWND hDlg) {
    int selx = SendDlgItemMessage (hDlg, IDC_RLYQUERY_LIST, LB_GETCURSEL,0,0);
    if (selx < 0)
	return NULL;
    return (Relay*)SendDlgItemMessage
	    (hDlg, IDC_RLYQUERY_LIST, LB_GETITEMDATA, selx,0);
}


static DLGPROC_DCL RlystateDlgProc (HWND hDlg, unsigned message, WPARAM wParam, LPARAM lParam)
{
    char buf [100];
    Relay * r;
    switch (message) {
	case WM_INITDIALOG:
	{
	    r = (Relay *) lParam;
dorelay:
	    SetWindowLong (hDlg, DWL_USER, (LONG)r);
	    EnableWindow (GetDlgItem (hDlg, IDC_DRAW_RELAY),
			  !(r->Flags & LF_CCExp));
	    wsprintf (buf, "Relay %s", RlysymPRep (r->RelaySym));
	    SetDlgItemText (hDlg, IDC_RLYQUERY_NAME, buf);
	    wsprintf (buf, "State is %s", r->State ? "PICKED" : "DROPPED");
	    SetDlgItemText (hDlg, IDC_RLYQUERY_STATE, buf);
	    wsprintf (buf, "%d dependent%s:",
		      r->NDependents, (r->NDependents) == 1 ? "" : "s");
	    SetDlgItemText (hDlg, IDC_RLYQUERY_NDEPS, buf);
	    SendDlgItemMessage (hDlg, IDC_RLYQUERY_LIST, LB_RESETCONTENT,0,0);
	    for (int j = 0; j < r->NDependents; j++) {
		Relay * dep = r->Dependents[j];
		wsprintf (buf, "%s\t%d", RlysymPRep(dep->RelaySym), dep->State);
		int index
			=  SendDlgItemMessage
			   (hDlg, IDC_RLYQUERY_LIST, LB_ADDSTRING, 0, (LPARAM)(LPSTR)buf);
		SendDlgItemMessage
			(hDlg, IDC_RLYQUERY_LIST, LB_SETITEMDATA,
			 index, (LPARAM)dep);
	    }
	    return TRUE;
	}
	case WM_COMMAND:
	    if (wParam == IDOK|| wParam == IDCANCEL) {
		EndDialog (hDlg, TRUE);
		return TRUE;
	    }
	    else if (wParam == IDC_DRAW_RELAY) {
		Relay * r = (Relay*)GetWindowLong (hDlg, DWL_USER);
		RelayShowString(RlysymPRep (r->RelaySym));
	    }
	    else if (NOTIFY_CODE(wParam,lParam) == LBN_DBLCLK) {
		r = GetSelRelayFromListDlg (hDlg);
		goto dorelay;
	    }
	    else return FALSE;
	default:
	    return FALSE;
    }
}


static void ShowStateRelay (Relay * rly) {
    DialogBoxParam (app_instance,
		    MAKEINTRESOURCE(IDD_RELAYSTATE),
		    G_mainwindow, DLGPROC(RlystateDlgProc), (LPARAM)rly);
}


void AskForAndShowStateRelay (HWND win) {
    char rnm [32];
    if (RlyDialog (win, app_instance, rnm)) {
	Sexpr s = RlysymFromStringNocreate (rnm);
	if (s == NIL || s.u.r->rly==NULL) {
	    usermsg ("No such relay: %s", rnm);
	    return;
	}
	ShowStateRelay (s.u.r->rly);
    }
}

void AskForAndDrawRelay (HWND win) {
    char rnm[32];
    if (RlyDialog (win, app_instance, rnm))
	RelayShowString (rnm);
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
    Relay ** Array;
    int N;
    Relay * Result;
    char Title[64];
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
	    for (int i = 0; i < S->N; i++) {
		Relay * r = S->Array[i];
		const char * rp = RlysymPRep(r->RelaySym);
		while (isdigit((unsigned char)*rp)) rp++;
		int ix = SendMessage (lb, LB_ADDSTRING, 0,(LPARAM)rp);
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
    
    S.N = get_relay_array_for_object_number (object_number, NULL);
    if (S.N == 0) {
	usermsg ("No relays found for %s %d.", classdesc, object_number);
	return NULL;
    }
    S.Array = new Relay*[S.N];
    get_relay_array_for_object_number (object_number, S.Array);
    
    DialogBoxParam (app_instance,
		    MAKEINTRESOURCE(IDD_OBJECT_RELAY_LIST),
		    G_mainwindow, RelayListDlgProc, (LPARAM) &S);
    delete [] S.Array;
    return S.Result;
}


int ShowStateRelaysForObject (int object_number, const char * classdesc) {
    Relay * r = ListRelaysForObjectDialog ("Show State", classdesc, object_number);
    if (!r)
	return 0;
    ShowStateRelay (r);
    return 1;
}

int DrawRelaysForObject (int object_number, const char * classdesc) {
    Relay * r = ListRelaysForObjectDialog ("Draw", classdesc, object_number);
    if (!r)
	return 0;
    RelayShowString(RlysymPRep (r->RelaySym));
    return 1;
}
