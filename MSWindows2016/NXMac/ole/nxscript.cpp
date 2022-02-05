#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "commands.h"
#include "compat32.h"
#include "demoapi.h"
#include "nxsysapp.h"
#include "usermsg.h"
#include "nxole.h"
#include "timers.h"
#include "trainapi.h"
#include <syserr32.h>
#include "nxautcli.h"
#include "nxhelp.h"

static HWND CmdDlg = NULL;

int ParseExecuteCommand (NXSYS &N, char * b);
int ReadParseExecuteCommand (NXSYS &N, FILE * f, int echo);

static NXSYS * ObjNXSYS = NULL;
static long TimeInterval = 0L;

/* our nasty replacement for NXSYSSleepMilliseconds in nxautcli.cpp */
void NXSYSSleepMilliseconds (long milliseconds) {
    TimeInterval = milliseconds;
}

static char T;
static int ScriptInProgress = 0, ScriptPaused = 0;
static FILE* ScriptFile = NULL;
static char ScriptPath[MAXPATH];
static BOOL ErSw = FALSE;

static void EndScript() {
    if (ScriptFile != NULL)
	fclose (ScriptFile);
    KillOneTimer (&T);
    delete ObjNXSYS;
    ObjNXSYS = NULL;
    ScriptFile = NULL;
    ScriptInProgress = 0;
    HideDemoWindow();
}

static void Ercom (BOOL is_error,
		   const char * text, va_list ap, UINT code, const char * title) {
    char buf [400];
    vsprintf (buf, text, ap);
    va_end (ap);
    int l = strlen(buf);
    if (l > 0 && buf[l-1] == '\n')
	buf[l-1] = '\0';		/* fprintf stuffies */
    if (CmdDlg) {
	HWND Tb = GetDlgItem (CmdDlg, IDC_CLI_STATUS);
	if (Tb) {
	    char buf2[400];
	    GetWindowText (Tb, buf2, sizeof(buf)-1);
	    if (buf2[0] != '\0')
		strcat (buf2, "\r\n");
	    strcat (buf2, buf);
	    SetWindowText (Tb, buf2);
	}
    }
    else {
	if (is_error) {
	    ErSw = 1;
	    strcat (buf, " - Terminating script.");
	}
	MessageBox (G_mainwindow, buf, title , MB_OK| code);
    }
}
    

void NXScriptInternalError (void *, const char * text, ...) {
    va_list ap;
    va_start (ap, text);
    Ercom (TRUE, text, ap, MB_ICONSTOP, "NXScript internal error");
}

void NXScriptUserError (void *, const char * text, ...) {
    va_list ap;
    va_start (ap, text);
    Ercom (TRUE, text, ap, MB_ICONEXCLAMATION, "NXScript user error");
}

void NXScriptUserPrintf (const char * text, ...) {
    va_list ap;
    va_start (ap, text);
    Ercom (FALSE, text, ap, MB_ICONEXCLAMATION, "NXScript user output");
}

static void ScriptImpulse (void*) {
    if (!ScriptInProgress)
	return;
    if (ScriptPaused) {
	if (ScriptPaused == 1) {
	    DemoSay("SPACE to resume, ESC to end demo.  Script paused.");
	    ScriptPaused = 2;
	}
	return;
    }

    for (;!ErSw;) {
	TimeInterval = 0L;
	if (!ReadParseExecuteCommand (*ObjNXSYS, ScriptFile, 0)) {
	    EndScript();
	    break;
	}
	if (TimeInterval) {
	    NXTimer (&T, ScriptImpulse, TimeInterval);
	    break;
	}
    }
    if (ErSw)
	EndScript();
}

/* External API hook-called by DemoPause */
void ScriptPause (int haltsw) {
    if (ScriptInProgress) {
	if (haltsw)
	    EndScript();
	else
	    if (ScriptPaused) {
		ScriptPaused = 0;
		ScriptImpulse(NULL);
	    }
	    else {
		ScriptPaused = 1;
		DemoSay("Script pausing...control the interlocking yourself now...");
		NXTimer (&T, ScriptImpulse, 1000);
	    }
    }
}

/* this is an external API */
void NXScript (const char * fname) {
    if (ScriptInProgress){
	usermsg ("Script already in progress!");
	return;
    }

    if (!EnsureOleInitialized(FALSE))
	return;

    strcpy (ScriptPath, fname);		/* for expand path */
    ScriptFile = fopen (fname, "r");
    if (ScriptFile == NULL) {
	usermsgstop ("Can't open demo file %s: %s.", fname, _strerror(NULL));
	return;
    }
    TrainMiscCtl (CmKillTrains);
    ScriptInProgress = 1;
    ScriptPaused = 0;
    ErSw = FALSE;
    DemoSay ("");
    if (ObjNXSYS == NULL)
	ObjNXSYS = new NXSYS;
    ObjNXSYS->Create();
    ScriptImpulse (NULL);
    return;
}

/* ----------------------------------------------------------- */


DLGPROC_DCL InteractorDlgProc
   (HWND hDlg, unsigned message, WPARAM wParam, LPARAM lParam)
{
    static BOOL DlgPlaceSet = FALSE;
    static int DlgX, DlgY;
    static NXSYS * nxp = NULL;
    RECT MainCli, DlgRect;

    switch (message) {
	case WM_INITDIALOG:
	{
	    GetClientRect(G_mainwindow,&MainCli);
	    GetWindowRect (hDlg, &DlgRect);
	    int wid = DlgRect.right - DlgRect.left;
	    int hgt = DlgRect.bottom - DlgRect.top;
	    if (!DlgPlaceSet) {
		DlgX = DlgRect.left;
		DlgY = (int)(.75*MainCli.bottom);
	    }
	    MoveWindow (hDlg, DlgX, DlgY, wid, hgt, FALSE);
	    if (ObjNXSYS == NULL)
		ObjNXSYS = new NXSYS;
	    ObjNXSYS->Create();
	    return TRUE;
	}
	case WM_KEYDOWN:

	    if (wParam == VK_F1) {
		WinHelp (G_mainwindow, HelpPath, HELP_CONTEXT, IDH_SCRIPT_LANG);
		return TRUE;
	    }
	    else return FALSE;
		
	case WM_COMMAND:
	    switch (wParam) {
		case IDCANCEL:
		    GetWindowRect (hDlg, &DlgRect);
		    DlgX = DlgRect.left;
		    DlgY = DlgRect.top;
		    DlgPlaceSet = TRUE;
		    DestroyWindow (hDlg);
		    CmdDlg = NULL;
		    return TRUE;
		case IDHELP:
		    WinHelp (G_mainwindow, HelpPath, HELP_CONTEXT, IDH_SCRIPT_LANG);
		    return TRUE;

		case IDOK:
		{
		    char buf[200];
		    HWND Edit= GetDlgItem (hDlg, IDC_CLI_CMD);
		    GetWindowText (Edit, buf,sizeof(buf)-1);
		    SetWindowText (GetDlgItem(hDlg, IDC_CLI_STATUS), "");
		    ParseExecuteCommand (*ObjNXSYS, buf);
		    SetFocus (Edit);
		    SendMessage (Edit, EM_SETSEL, (WPARAM) 0, (LPARAM)-1);
		    return TRUE;
		}
		default:
		    return FALSE;
	    }
	default:
	    return FALSE;
    }
}


void CommandLoopDlg () {

    if (CmdDlg) {
	SetFocus (CmdDlg);
	return;
    }

    if (!EnsureOleInitialized(FALSE))
	return;

    CmdDlg = CreateDialog
	     (app_instance, MAKEINTRESOURCE(IDD_INTERACTOR),
	      G_mainwindow,
	      (DLGPROC) InteractorDlgProc);
    if (!CmdDlg) {
	usermsgstop ("Can't create interactor dialog: %s", SysLastErrstr());
	return;
    }
}

BOOL IsCmdLoopDlgMessage (MSG * msgp) {
    return CmdDlg && IsDialogMessage (CmdDlg, msgp);
}
