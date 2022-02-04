#include <windows.h>
#include <commdlg.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <ctype.h>

#include <compat32.h>
#include <nxgo.h>
#include <tledit.h>
#include <dialogs.h>

#include "tlecmds.h"
#include "resource.h"


static char *GFilter = "Interpreted Xlkgs\0*.TRK\0All Files (*.*)\0*.*\0\0";

BOOL FileOpenDlg (HWND hwnd, LPSTR lpstrFileName, LPSTR lpstrTitleName,
		  int bufl, int rsw) {

     OPENFILENAME ofn ;
     char ext [MAXEXT];
     memset (&ofn, 0, sizeof (OPENFILENAME));
     fnsplit(lpstrFileName, NULL, NULL, NULL, ext);
     strlwr(ext);

     ofn.nFilterIndex      = 1 ;
     ofn.lStructSize       = sizeof (OPENFILENAME) ;
     ofn.hwndOwner         = hwnd ;
     ofn.hInstance         = NULL ;
     ofn.lpstrFilter	   = GFilter;
     ofn.lpstrCustomFilter = NULL ;
     ofn.nMaxCustFilter    = 0 ;
     ofn.lpstrFile         = lpstrFileName;
     ofn.nMaxFile          = bufl;
     ofn.lpstrFileTitle    = lpstrTitleName;
     ofn.nMaxFileTitle     = 256;
     ofn.lpstrInitialDir   = NULL ;
     ofn.lpstrTitle        = rsw ? 
			     "Track definition file to be edited" :
			     "Track definition file to be written";
     ofn.Flags             = (rsw  ? OFN_FILEMUSTEXIST : OFN_OVERWRITEPROMPT)
			     | OFN_HIDEREADONLY;
     ofn.nFileOffset       = 0;
     ofn.nFileExtension    = 0 ;
     ofn.lpstrDefExt       = "" ;
     ofn.lCustData         = 0L ;
     ofn.lpfnHook          = NULL ;
     ofn.lpTemplateName    = NULL ;
     ofn.hwndOwner         = hwnd ;

     return rsw ? GetOpenFileName (&ofn) : GetSaveFileName (&ofn);
}

int S_x, S_y;

DLGPROC_DCL ShiftDlgProc (HWND hDlg, unsigned message, WPARAM wParam, LPARAM lParam)
{
    BOOL ok;

    switch (message) {
	case WM_INITDIALOG:
	{
	    SetDlgItemText (hDlg, IDC_SHIFT_X, "0");
	    SetDlgItemText (hDlg, IDC_SHIFT_Y, "0");
	    return TRUE;
	}
	case WM_COMMAND:
	    if (wParam == IDCANCEL) {
		EndDialog (hDlg, FALSE);
		return TRUE;
	    }
	    else if (wParam == IDOK) {
		S_x = (int)GetDlgItemInt(hDlg, IDC_SHIFT_X, &ok, TRUE);
		if (ok)
		    S_y = (int)GetDlgItemInt(hDlg, IDC_SHIFT_Y, &ok, TRUE);
		if (!ok) {
		    MessageBox (hDlg, "Bad number.  Try again.",
				"Layout Shift Dialog",
				MB_OK|MB_ICONEXCLAMATION);
		    return TRUE;
		}
		EndDialog (hDlg, TRUE);
		return TRUE;
	    }
	    else return FALSE;
	default:
	    return FALSE;
    }
}


BOOL DoShiftLayoutDlg (int &x, int&y) {
    if (DialogBox (app_instance,
		      MAKEINTRESOURCE(IDD_SHIFT),
		      G_mainwindow,
		      DLGPROC(ShiftDlgProc))) {
	x = S_x;
	y = S_y;
	return TRUE;
    }
    return FALSE;
}




double VirtualScreenScale = 1.0;


DLGPROC_DCL ScaleDlgProc (HWND hDlg, unsigned message, WPARAM wParam, LPARAM lParam)
{
    char buf[30];

    switch (message) {
	case WM_INITDIALOG:
	{
	    if ((((int)(VirtualScreenScale*1000.0))%1000) == 0)
		sprintf (buf, "%d", (int) VirtualScreenScale);
	    else	
		sprintf (buf, "%g", VirtualScreenScale);
	    SetDlgItemText (hDlg, IDC_SCALE_EDIT, buf);
	    return TRUE;
	}
	case WM_COMMAND:
	    switch (wParam) {
		case IDCANCEL:
		    EndDialog (hDlg, FALSE);
		    return TRUE;
		case IDOK:
		    GetDlgItemText (hDlg, IDC_SCALE_EDIT, buf, sizeof(buf)-1);
		    if (sscanf (buf, "%lf", &VirtualScreenScale) == 1) {
fx:			NXGO_SetDisplayScale (VirtualScreenScale);
			EndDialog (hDlg, TRUE);
			return TRUE;
		    }
		    else
			MessageBox (hDlg, "Bad number.  Try again.",
				    "Display Scale Dialog",
				    MB_OK|MB_ICONEXCLAMATION);
		    return TRUE;
		case IDC_SCALE_P5:
		    VirtualScreenScale = .5;
		    goto fx;
		case IDC_SCALE_P7:
		    VirtualScreenScale = .7;
		    goto fx;
		case IDC_SCALE_P8:
		    VirtualScreenScale = .8;
		    goto fx;
		case IDC_SCALE_1:
		    VirtualScreenScale = 1.0;
		    goto fx;
		default:
		    return FALSE;
	    }
	default:
	    return FALSE;
    }
}

BOOL ScaleDialog () {
    return DialogBox (app_instance,
		      MAKEINTRESOURCE(IDD_SCALE),
		      G_mainwindow,
		      DLGPROC(ScaleDlgProc));
}

