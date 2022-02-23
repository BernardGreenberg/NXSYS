#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <commdlg.h>
#include "ldraw.h"
#include <time.h>
#include "rlyindex.h"
#include "compat32.h"

extern HWND G_mainwindow;
extern HINSTANCE app_instance;
extern char app_name[];
static const char * Title = NULL;
static int xPage, yPage, BotLoc;
static int Printing = 0;
static int PageNo;
static int FirstPageno, LastPageno;
static FARPROC lpfnAbortProc = NULL;
static HDC Hpdc = NULL;
static HFONT Pfnt = NULL, Pfnt2 = NULL;
static HPEN HPen = NULL;
static BOOL bUserAbort;
static HWND hDlgPrint = NULL;
static char datime[30];
static char Indexing = 0;

#ifdef TESTTR0
void TR0(int n) {
char buf[20]; sprintf(buf, "TRACE %d", n);
	    MessageBox (0, buf, "TRACE", MB_OK);}

#endif

DLGPROC_DCL PrintDlgProc (HWND hDlg, UINT message, WPARAM, LPARAM)
     {/* aus dem livre de m. PETZOLD, le XVte Kapitel  */
     switch (message)
          {
          case WM_INITDIALOG:

               EnableWindow (GetParent (hDlg), FALSE) ;
               return TRUE ;

          case WM_COMMAND:
               bUserAbort = TRUE ;
               EnableWindow (GetParent (hDlg), TRUE) ;
               hDlgPrint = 0 ;
               DestroyWindow (hDlg) ;
               return TRUE ;
          }
     return FALSE ;
     }          

#ifdef WIN32
BOOL PASCAL AbortProc (HDC, short)
#else
BOOL CALLBACK _export AbortProc (HDC, int)
#endif     
{
     MSG msg ;

     while (!bUserAbort && PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
          {
          if (!hDlgPrint || !IsDialogMessage (hDlgPrint, &msg))
               {
               TranslateMessage (&msg) ;
               DispatchMessage (&msg) ;
               }
          }
     return !bUserAbort ;
     }


int StartPrintRelays (const char * title, int indexing) {

    CleanUpDrawing();

    time_t timer;
    struct tm *tblock;
    timer = time(NULL);
    tblock = localtime (&timer);
    strcpy (datime, asctime(tblock));
    *strchr (datime, '\n') = '\0';

    static PRINTDLG pd;
    memset (&pd, 0, sizeof(pd));
    pd.lStructSize = sizeof (PRINTDLG);
    pd.hwndOwner = G_mainwindow;
    pd.Flags = PD_ALLPAGES | PD_RETURNDC;
    pd.nFromPage = 1;
    pd.nToPage = 999;
    pd.nMinPage = 1;
    pd.nMaxPage = 999;
    bUserAbort = FALSE;
    int v = PrintDlg (&pd);
    if (!v)
	return 0;
    Hpdc = pd.hDC;
    FirstPageno = pd.nFromPage;
    LastPageno = pd.nToPage;
    Indexing = indexing;

    xPage = GetDeviceCaps (Hpdc, HORZRES);
    yPage = GetDeviceCaps (Hpdc, VERTRES);
    LOGFONT lf;
    int minm = yPage;
    if (xPage < minm)
	minm = xPage;

    HPen = CreatePen (PS_SOLID, GetDeviceCaps (Hpdc, LOGPIXELSY)/200,
		      RGB(0,0,0));
    SelectObject(Hpdc, HPen);
    memset (&lf, 0, sizeof(LOGFONT));
    lf.lfHeight = minm/70;
    strcpy (lf.lfFaceName, "Helvetica");
    Pfnt = CreateFontIndirect (&lf);
    memset (&lf, 0, sizeof(LOGFONT));
    lf.lfHeight = minm/40;
    BotLoc = yPage - lf.lfHeight/5;
    strcpy (lf.lfFaceName, "Helvetica");
    Pfnt2 = CreateFontIndirect (&lf);
    SelectObject (Hpdc, Pfnt);
    DrawingSetPageSize (xPage, BotLoc, minm, LDRAW_NO_STATEREPORT);
    Printing = 1;
    Title = title;
    if (lpfnAbortProc == NULL)
	lpfnAbortProc = MakeProcInstance ((FARPROC) AbortProc, app_instance);
    Printing = 1;
    PageNo = 1;

#ifndef WIN32
    /* Beats the -hell- out of me why this doesn't work in win3.1 */
    if (LOBYTE(LOWORD(GetVersion())) != 3)
#endif
	SetAbortProc (Hpdc, (ABORTPROC) lpfnAbortProc);

    DOCINFO DocInfo;
    DocInfo.cbSize = sizeof(DocInfo);
    DocInfo.lpszOutput = NULL;
#ifdef WIN32
    DocInfo.lpszDocName = title;
    DocInfo.lpszDatatype = "NX Interlocking Prints";
    DocInfo.fwType = 0;
#else
    DocInfo.lpszDocName = "Interlocking";
#endif

    if (StartDoc (Hpdc, &DocInfo) < 0) {
	CleanUpPrinting();
	return 0;
    }

    hDlgPrint = CreateDialog  /* autoexposes */
		(app_instance,"CancelPrintDlgBox",
		 G_mainwindow, (DLGPROC) PrintDlgProc);
    return 1;
}

int IndexMoreProc ();
void FramePage();

static void
DocEnd () {
#ifdef WIN32
	EndPage (Hpdc);
	EndDoc (Hpdc);
#else
	Escape (Hpdc, ENDDOC, 0, NULL, NULL);
#endif
}

int IndexPrinted = 0;

int FinishPrintRelays () {
    if (bUserAbort)
	DocEnd();
    else if (PageFrame()) {
	if (Indexing && PageNo <= LastPageno) {
	    IndexPrinted = 0;
#ifdef WIN32
	    StartPage(Hpdc);
#endif
	    
	    ProduceRelayIndex (Hpdc, IndexMoreProc,Pfnt,Pfnt2, xPage, BotLoc);
	    IndexPrinted = -1;
	    IndexMoreProc();
	}
	DocEnd();
    }
    DeleteDC (Hpdc);
    Printing=0;
    Hpdc = NULL;
    if (hDlgPrint) {
	EnableWindow (GetParent (hDlgPrint), TRUE);
	DestroyWindow (hDlgPrint);
	hDlgPrint = NULL;
    }
    return 1;
}

void CleanUpPrinting() {
    if (Hpdc != NULL)
	DeleteDC (Hpdc);
    Hpdc = NULL;
    if (Pfnt != NULL)
	DeleteObject (Pfnt);
    Pfnt = NULL;
    if (Pfnt2 != NULL)
	DeleteObject (Pfnt2);
    Pfnt2 = NULL;
    if (hDlgPrint != NULL)
	DestroyWindow (hDlgPrint);
    hDlgPrint = NULL;
    if (lpfnAbortProc != NULL)
	FreeProcInstance (lpfnAbortProc);
    if (HPen!= NULL)
	DeleteObject(HPen);
    HPen = NULL;
    lpfnAbortProc = NULL;

}


void FramePage() {

    SelectObject (Hpdc, Pfnt2);
    MoveTo (Hpdc, 0, 0);
    LineTo (Hpdc, xPage-1, 0);
    LineTo (Hpdc, xPage-1, yPage-1);
    LineTo (Hpdc, 0, yPage-1);
    LineTo (Hpdc, 0, 0);

    char buf [128];
    sprintf (buf, "%s    %s    Page %2d  ", Title, datime, PageNo);
    RECT tr;
    tr.top = tr.bottom = tr.left = tr.right= 0;
    DrawText (Hpdc, buf, (int)strlen(buf), &tr,
	      DT_TOP | DT_LEFT |DT_SINGLELINE|DT_CALCRECT);
    tr.top = BotLoc - tr.bottom - 3;
    tr.bottom = BotLoc - 3;
    tr.left = 0;
    tr.right = xPage - 3;
    DrawText (Hpdc, buf, (int)strlen(buf), &tr, DT_TOP|DT_RIGHT|DT_SINGLELINE);
    SelectObject (Hpdc, Pfnt);
}

static void
OurEndPage () {

#ifdef WIN32
    EndPage(Hpdc);
#else
    Escape (Hpdc, NEWFRAME, 0, NULL, NULL);
#endif
}


int PageFrame() {
    if (bUserAbort)
	return 0;

    if (Indexing)
	RecordIndexDrawnRelays (PageNo);

    if (PageNo >= FirstPageno && PageNo <= LastPageno) {
#ifdef WIN32
	StartPage(Hpdc);
#endif
	FramePage();
	RenderRelayPage (Hpdc);
	OurEndPage();
    }
    PageNo++;
    return 1;
}

int IndexMoreProc () {

    if (PageNo >= FirstPageno && PageNo <= LastPageno) {
	FramePage();
	OurEndPage();
#ifdef WIN32
	if (IndexPrinted != -1)
	    StartPage(Hpdc);
#endif
	IndexPrinted = 1;
    }
    PageNo++;
    return 1;
}
