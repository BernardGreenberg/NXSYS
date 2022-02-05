#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "lisp.h"
#include "stdio.h"
#include "relays.h"
#include "nxgo.h"
#include "objid.h"
#include "brushpen.h"
#include "nxsysapp.h"
#include "compat32.h"
#include "dynmenu.h"
#include "rlyapi.h"

/* Relay-operated Windows menus - a clever hack by BSG, 21 February 1997 */

/* TBD
   Why don't we need message forwarding?
   */

static BOOL DynMenusEnabled = TRUE;
static Relay * AZ = NULL;

#define CONTROL_ID_BASE 1000
/*
    (MENU 
0          223MENU		 	 ;relay symbol
1          (AND !4242NS !4242T)          ;expression
2	   "Southbound Route"            ;title
3	   (AUTODISMISS)                 ;options
4	   (("Brighton"   223MPBAPB 223MPBAK)  ;string, button, light
	    ("Fourth Ave" 223MPBBPB 223MPBBK)
	    ("West End"   223MPBCPB 223MPBCK)
	    ("Sea Beach"  223MPBDPB 223MPBDK)
	    ("Cancel"     223MPBEPB)))  ;no light

*/

class DynMenu;

class DynMenuEntry {
public:
    class DynMenu * Menu;		/* so callbacks can find */
    char * String;
    UINT   ControlId;
    Relay* PushButtonRelay;
    ReportingRelay* LampRelay;
    void Push();
    void BeforeShow();
    void LampReport (BOOL state);
    ~DynMenuEntry();
};

class DynMenu {
public:
    int nEntries;
    HWND Dlg;
    DynMenuEntry *Entries;
    char * Title;
    BOOL Up;
    long Nomenclature;
    Relay * MenuRelay;
    Relay * MNZ;
    GraphicObject * TiedObject;
    LRESULT DlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    void RelayFcn (BOOL state);
    void SetLights();
    ~DynMenu();
};

#define MAX_MENUS 30
static DynMenu * Menus[MAX_MENUS];
static int NMenus = 0;


static int nCopyAnsiToWideChar (LPWORD lpWCStr, LPSTR lpAnsiIn)
{
  int nChar = 0;

  do {
    *lpWCStr++ = (WORD) *lpAnsiIn;
    nChar++;
  } while (*lpAnsiIn++);

  return nChar;
}

static LPWORD lpwAlign ( LPWORD lpIn)
{
  ULONG ul;

  ul = (ULONG) lpIn;
  ul +=3;
  ul >>=2;
  ul <<=2;
  return (LPWORD) ul;
}

void DynMenuEntry::Push () {
	PulseToRelay (PushButtonRelay);
}

void DynMenuEntry::BeforeShow () {
    if (LampRelay) {
	BOOL state = (BOOL) (LampRelay->State & 1);
	HWND control = GetDlgItem(Menu->Dlg, ControlId);

	if (state) {
	    SetFocus(control);
	    CheckRadioButton (Menu->Dlg,
			      CONTROL_ID_BASE,
			      CONTROL_ID_BASE+Menu->nEntries - 1,
			      ControlId);
	}
    }
}
    
void DynMenu::SetLights () {
    int i;
    for (i = 0; i < nEntries; i++)
	Entries[i].BeforeShow();
}

void DynMenuEntry::LampReport (BOOL state) {
    if (Menu->Dlg) 
	SendDlgItemMessage(Menu->Dlg, ControlId, BM_SETCHECK, state, 0L);
}

void DynMenu::RelayFcn (BOOL state) {
    if (state) {
	RECT r;
	if (TiedObject && TiedObject->Visible) {
	    GetWindowRect (Dlg, &r);
	    int width = r.right-r.left;
	    int height = r.bottom - r.top;
	    POINT Origin = {0, 0};
	    ClientToScreen (G_mainwindow, &Origin);
	    int x = TiedObject->sc_x - 3*GU2 + Origin.x;
	    int y = TiedObject->sc_y + 4*GU2 + Origin.y;
	    MoveWindow (Dlg, x, y, width, height, FALSE);
	}
	SetLights();
    }
    ShowWindow (Dlg, state ? SW_SHOW : SW_HIDE);
}


LRESULT DynMenu::DlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    switch (message) {

	case WM_INITDIALOG:
	    SetLights();
	    Up = TRUE;
	    return FALSE;
	    
	case WM_SHOWWINDOW:
	    if (wParam)
		SetLights();
	    Up = wParam;
	    return TRUE;

	case WM_COMMAND:
	    switch (wParam) {
		case IDCANCEL:
		    Up = FALSE;
		    ShowWindow (hDlg, SW_HIDE);
		    return TRUE;
		default:
		    if (!Up)
			return FALSE;
		    if (wParam >= CONTROL_ID_BASE &&
			wParam < (UINT)(CONTROL_ID_BASE + nEntries)) {
			Entries[wParam - CONTROL_ID_BASE].Push();
			return TRUE;
		    }
		    return FALSE;
	    }
	default:
	    return FALSE;
    }
}


static DLGPROC_DCL DlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_INITDIALOG) {
	SetWindowLong (hDlg, DWL_USER, (long) lParam);
    }
    return ((DynMenu *) GetWindowLong (hDlg, DWL_USER))->DlgProc(hDlg, message, wParam, lParam);
}


static void CreateTheMenu (DynMenu* md) {
  WORD  *p, *pdlgtemplate;
  int   nchar;
  DWORD lStyle;
  int bno, ypos;

  /* allocate some memory to play with  */
  pdlgtemplate = p = (PWORD) LocalAlloc (LPTR, 2048);

  /* start to fill in the dlgtemplate information.  addressing by WORDs */
  lStyle = DS_MODALFRAME | DS_3DLOOK | DS_CENTERMOUSE | WS_POPUP | WS_CAPTION;

  *p++ = LOWORD (lStyle);
  *p++ = HIWORD (lStyle);
  *p++ = 0;          // LOWORD (lExtendedStyle)
  *p++ = 0;          // HIWORD (lExtendedStyle)
  *p++ = md->nEntries;   // NumberOfItems
  *p++ = 10*NMenus;         // x
  *p++ = 10*NMenus;         // y
  *p++ = 100;        // cx
  *p++ = 10+md->nEntries*20;
  *p++ = 0;          // Menu
  *p++ = 0;          // Class

  /* copy the title of the dialog */
  nchar = nCopyAnsiToWideChar (p, md->Title);
  p += nchar;

  /* add in the wPointSize and szFontName here iff the DS_SETFONT bit on */

  ypos = 10;
  for (bno = 0; bno < md->nEntries; bno++) {

      /* make sure each item starts on a DWORD boundary */
      p = lpwAlign (p);

      lStyle = BS_RADIOBUTTON | WS_VISIBLE | WS_CHILD;
      if (bno == 0)
	  lStyle |= WS_GROUP;

      *p++ = LOWORD (lStyle);
      *p++ = HIWORD (lStyle);
      *p++ = 0;          // LOWORD (lExtendedStyle)
      *p++ = 0;          // HIWORD (lExtendedStyle)
      *p++ = 10;         // x
      *p++ = ypos;       // y
      *p++ = 145;        // cx
      *p++ = 10;         // cy
      *p++ = md->Entries[bno].ControlId;

      ypos += 20;
      /* fill in class i.d. Button in this case */
      *p++ = (WORD)0xffff;
      *p++ = (WORD)0x0080;

      /* copy the text of the item */
      nchar = nCopyAnsiToWideChar (p, md->Entries[bno].String);
      p += nchar;

      *p++ = 0;  // advance pointer over nExtraStuff WORD
  }

  md->Dlg = CreateDialogIndirectParam
	    (app_instance, (LPDLGTEMPLATE) pdlgtemplate,
	     G_mainwindow, (DLGPROC) DlgProc, (LPARAM)(void*) md);

  LocalFree (LocalHandle (pdlgtemplate));

}

static void MenuRelayFcn (BOOL state, void * v) {
    if (DynMenusEnabled || !state)
	((DynMenu *) v)->RelayFcn(state);
}

static void LampRelayFcn (BOOL state, void * v) {
    ((DynMenuEntry*) v)->LampReport (state);
}

int DefineMenuFromLisp (Sexpr s) {
    if (ListLen (s) < 5) {
	LispBarf (1, "Fewer than five arguments in MENU form", s);
	return 0;
    }
    if (CAR(s).type != L_RLYSYM) {
	LispBarf (1, "Relay symbol in MENU not a relay symbol", CAR(s));
	return 0;
    }
    Sexpr rlysym = CAR(s);
    SPop(s);
    if (NMenus >= MAX_MENUS) {
	LispBarf (1, "Too many relay-operated MENUs.", rlysym);
	return 0;
    }

    ReportingRelay * rr = CreateReportingRelay(rlysym);
    Sexpr expr = Lisp_Cons (CAR(s), NIL);
    if (!DefineRelayFromLisp2 (rlysym, expr))
	return 0;			/* compilation error */
    SPop(s);
    if (CAR(s).type != L_STRING) {
	LispBarf (1, "Title string in MENU not a string", CAR(s));
	return 0;
    }
    char * title = _strdup (CAR(s).u.s);
    SPop(s);
    /* ignore options for now */
    SPop(s);
    if (CAR(s).type != L_CONS) {
	LispBarf (1, "Button list in MENU not a list", CAR(s));
	return 0;
    }
    Sexpr items = CAR(s);
    int nitems = ListLen (items);
    if (nitems < 1){
	LispBarf (1, "No entries in Menu", rlysym);
	return 0;
    }
    
    long Nom = rlysym.u.r->n;
    DynMenu * Menu = new DynMenu;
    Menu->nEntries = nitems;
    Menu->Dlg = NULL;
    Menu->Entries = new DynMenuEntry[nitems];
    memset (Menu->Entries, 0, sizeof(DynMenuEntry)*nitems); /* for destr. */
    Menu->Title = title;
    Menu->Up = FALSE;
    Menu->Nomenclature = Nom;
    Menu->TiedObject = FindHitObject (Nom, ID_SIGNAL);
    Menu->MenuRelay = rr;
    Menu->MNZ = GetRelay2NoCreate (Nom, "MNZ");
    for (int entno = 0; entno < nitems; entno++, SPop (items)) {
	Sexpr item = CAR(items);
	DynMenuEntry * Entry = &Menu->Entries[entno];
	Entry->Menu = Menu;
	Entry->ControlId = CONTROL_ID_BASE + entno;
	if (CAR(item).type != L_STRING) {
	    LispBarf (1, "Item label not a string in MENU", rlysym);
	    /* free crap */
	    return 0;
	}
	Entry->String = _strdup (CAR(item).u.s);
	SPop(item);
	if (CAR(item).type != L_RLYSYM) {
	    LispBarf (1, "PB relay sym not a relay sym in MENU", rlysym);
	    return 0;
	}
	Entry->PushButtonRelay = CreateRelay (CAR(item));
	SPop(item);
	if (item.type == L_CONS) {
	    if (CAR(item).type != L_RLYSYM) {
		LispBarf (1, "Lamp relay sym not a relay sym in MENU", rlysym);
		return 0;
	    }
	    Entry->LampRelay = CreateReportingRelay (CAR(item));
	    Entry->LampRelay->SetReporter (LampRelayFcn, Entry);
	}
	else Entry->LampRelay = NULL;
    }
    rr->SetReporter (MenuRelayFcn, Menu);
    CreateTheMenu (Menu);
    Menus[NMenus++] = Menu;
    return 1;
}


void DestroyDynMenus() {
    for (int i = 0; i < NMenus; i++)
	delete Menus[i];
    NMenus = 0;
}

DynMenuEntry::~DynMenuEntry() {
    if (String)
	free (String);
}

DynMenu::~DynMenu () {
    if (Dlg)
	DestroyWindow (Dlg);
    if (Entries)
	delete [] Entries;
    if (Title)
	free(Title);
}

BOOL IsMenuDlgMessage (MSG*m) {
    for (int i = 0;i < NMenus; i++){
	if (IsDialogMessage(Menus[i]->Dlg, m))
	    return TRUE;
    }
    return FALSE;
}

void EnableDynMenus (BOOL onoff) {
    DynMenusEnabled = onoff;
}

BOOL AutoControlRelayExists () {
    AZ = GetRelay2NoCreate (0, "AZ");
    return (AZ != NULL);
}

void EnableAutomaticOperation (BOOL onoff) {
    if (AZ)
	ReportToRelay (AZ, onoff);
}

void TrySignalIDBox (long nomenclature) {
    for (int i = 0;i < NMenus; i++) {
	DynMenu * dmp = Menus[i];
	if (dmp->Nomenclature == nomenclature) {
	    if (dmp->MNZ)
		PulseToRelay (dmp->MNZ);
	    break;
	}
    }
}

    
