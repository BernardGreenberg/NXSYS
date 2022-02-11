#include "windows.h"
#include <string.h>
#include <stdio.h>
#include "nxgo.h"
#include "tledit.h"
#include "dragger.h"
#include "text.h"
#include "resource.h"
#include "compat32.h"
#include <stdlib.h>
#ifndef NXSYSMac
#include <commdlg.h>
#endif
#include "brushpen.h"
#include "tlpropdlg.h"
#include "objreg.h"

#define TEXT_DUMP_PRIORITY 1000
#ifdef NXSYSMac
#define APIENTRY
#define GetRValue(x)((x >> 16 & 0xFF)) // I think
#define GetGValue(x)((x >> 8 & 0xFF))
#define GetBValue(x)((x >> 0 & 0xFF))
#endif

static Dragger Dragon;
TextString::~TextString() {}

static GraphicObject* CreateTextString (int wpx, int wpy) {
    if (Dragon.Movingp()) /* don't create obj if moving */
	return NULL;
    LOGFONT lf;
    memset (&lf, 0, sizeof(lf));
    TextString * txs = new TextString ("Text string", &lf, wpx, wpy, 0, FALSE);
    return Dragon.StartMoving (txs, "New text string", G_mainwindow);
}

REGISTER_NXTYPE(ID_TEXT, CmText, IDD_EDIT_TEXT, CreateTextString, NULL);

void TextString::EditClick (int x, int y) {
    Dragon.ClickOn (G_mainwindow, this, "Text string", x, y);
}

/* ------------------------------------------------------------*/

static void DumpString (char * s, FILE * f) {
    putc ('"', f);
    for (char * p = s; *p != '\0'; p++) {
	char c = *p;
	if (c == '\r') fprintf (f, "\\r");
	else if (c == '\n') fprintf (f, "\\n");
	else if (c == '\t') fprintf (f, "\\t");
	else if (strchr ("\\\"", c))
	    fprintf (f, "\\%c",c);
	else putc (c, f);
    }
    putc ('"', f);
}

int TextString::Dump (FILE * f) {
    if (f == NULL)
	return TEXT_DUMP_PRIORITY;
    fprintf (f, "  (TEXT\t");
    DumpString (String, f);
    fprintf (f,"\n\t\t%4ld\t%4ld", wp_x, wp_y);
    if (pOriginalLogfont->lfFaceName[0] != '\0')
	fprintf (f, "\n\tFACE\t\"%s\"", pOriginalLogfont->lfFaceName);
    if (pOriginalLogfont->lfHeight != 0)
	fprintf (f, "\n\tHEIGHT\t%d", pOriginalLogfont->lfHeight);
    if (pOriginalLogfont->lfWidth != 0)
	fprintf (f, "\n\tWIDTH\t%d\n", pOriginalLogfont->lfWidth);
    if (pOriginalLogfont->lfWeight != 0) {
	if  (pOriginalLogfont->lfWeight == FW_NORMAL)
	    fprintf (f, "\n\tWEIGHT\tNORMAL");
	else if (pOriginalLogfont->lfWeight == FW_BOLD)
	    fprintf (f, "\n\tWEIGHT\tBOLD");
	else if (pOriginalLogfont->lfWeight == FW_BOLD)
	    fprintf (f, "\n\tWEIGHT\t%d", pOriginalLogfont->lfWeight);
    }
    if (pOriginalLogfont->lfItalic)
	fprintf (f, "\n\tITALIC\tT");

    if (ColorGiven)
	fprintf (f, "\n\tCOLOR\t(%d %d %d)",
		 GetRValue (Color), GetGValue (Color), GetBValue (Color));

    fprintf (f, ")\n");
    return TEXT_DUMP_PRIORITY;
}
/* ------------------------------------------------------------*/

#ifndef NXSYSMac
static COLORREF CustColors[16];
#endif
static COLORREF TempColor;

static LOGFONT TempLogfont;
static BOOL TempColorGiven;
static HFONT HFont = NULL;

static void ShowItem (HWND hDlg, UINT ID, UINT ID2, BOOL show) {
    ShowWindow (GetDlgItem (hDlg, ID), show? SW_SHOW : SW_HIDE);
    if (ID2 != 0)
	ShowWindow (GetDlgItem (hDlg, ID2), show? SW_SHOW : SW_HIDE);
}

static void SetTSDlgState (HWND hDlg) {

    SetDlgItemCheckState (hDlg, IDC_ETEXT_COLOR_CUSTOM, TempColorGiven);
    SetDlgItemCheckState (hDlg, IDC_ETEXT_COLOR_DEFAULT, !TempColorGiven);
#ifndef NXSYSMac  // italic stuff not worth it for now, bold is uclear.
    ShowItem (hDlg, IDC_ETEXT_COLOR_SELECT, 0, TempColorGiven);
    SetDlgItemCheckState (hDlg, IDC_ITALIC, TempLogfont.lfItalic);
#endif
    if (TempLogfont.lfFaceName[0]) {
	SetDlgItemCheckState (hDlg, IDC_ETEXT_FACE_DEFAULT, FALSE);
	SetDlgItemCheckState (hDlg, IDC_ETEXT_FACE_CUSTOM, TRUE);
	SetDlgItemText (hDlg, IDC_ETEXT_FACE_SELECTION, TempLogfont.lfFaceName);
#ifndef NXSYSMac  // no more 'select' button on Mac, 'custom' does it.
        ShowItem (hDlg, IDC_ETEXT_FACE_SELECT, 0, TRUE);
#endif
    }
    else {
	SetDlgItemCheckState (hDlg, IDC_ETEXT_FACE_DEFAULT, TRUE);
	SetDlgItemCheckState (hDlg, IDC_ETEXT_FACE_CUSTOM, FALSE);
	SetDlgItemText (hDlg, IDC_ETEXT_FACE_SELECTION, "");
#ifndef NXSYSMac // happens at once on "Custom"
        ShowItem (hDlg, IDC_ETEXT_FACE_SELECT, 0, FALSE);

#endif
    }

    if (TempLogfont.lfHeight == 0) {
	SetDlgItemCheckState (hDlg, IDC_ETEXT_HEIGHT_DEFAULT, TRUE);
	SetDlgItemCheckState (hDlg, IDC_ETEXT_HEIGHT_CUSTOM, FALSE);

	ShowItem (hDlg, IDC_ETEXT_HEIGHT_EDIT, IDC_ETEXT_HEIGHT_SPIN, FALSE);
    }
    else {
	SetDlgItemCheckState (hDlg, IDC_ETEXT_HEIGHT_DEFAULT, FALSE);
	SetDlgItemCheckState (hDlg, IDC_ETEXT_HEIGHT_CUSTOM, TRUE);
	ShowItem (hDlg, IDC_ETEXT_HEIGHT_EDIT, IDC_ETEXT_HEIGHT_SPIN, TRUE);
	SetDlgItemInt (hDlg, IDC_ETEXT_HEIGHT_EDIT,
		       TempLogfont.lfHeight, TRUE);
    }
#ifndef NXSYSMac  // Mac doesn't do "width" as far as I know.
    if (TempLogfont.lfWidth == 0) {
	SetDlgItemCheckState (hDlg, IDC_ETEXT_WIDTH_DEFAULT, TRUE);
	SetDlgItemCheckState (hDlg, IDC_ETEXT_WIDTH_CUSTOM, FALSE);
	ShowItem (hDlg, IDC_ETEXT_WIDTH_EDIT, IDC_ETEXT_WIDTH_SPIN, FALSE);
    }
    else {
	SetDlgItemCheckState (hDlg, IDC_ETEXT_WIDTH_DEFAULT, FALSE);
	SetDlgItemCheckState (hDlg, IDC_ETEXT_WIDTH_CUSTOM, TRUE);
	ShowItem (hDlg, IDC_ETEXT_WIDTH_EDIT, IDC_ETEXT_WIDTH_SPIN, TRUE);
	SetDlgItemInt (hDlg, IDC_ETEXT_WIDTH_EDIT, TempLogfont.lfWidth, FALSE);
    }
#endif
    {
        
        BOOL bDEFAULT = TempLogfont.lfWeight == 0;
        BOOL bBOLD = TempLogfont.lfWeight == FW_BOLD;
        BOOL bNORMAL = TempLogfont.lfWeight == FW_NORMAL;

        SetDlgItemCheckState (hDlg, IDC_ETEXT_WEIGHT_BOLD, bBOLD);
        SetDlgItemCheckState (hDlg, IDC_ETEXT_WEIGHT_NORMAL, bNORMAL);
	SetDlgItemCheckState (hDlg, IDC_ETEXT_WEIGHT_DEFAULT, bDEFAULT);
#ifndef NXSYSMac
        BOOL bCUSTOM = (!(bBOLD || bNORMAL || bDEFAULT));
        SetDlgItemCheckState (hDlg, IDC_ETEXT_WEIGHT_CUSTOM, bCUSTOM);
	ShowItem (hDlg,  IDC_ETEXT_WEIGHT_EDIT, IDC_ETEXT_WEIGHT_SPIN, bCUSTOM);
	if (bCUSTOM)
	    SetDlgItemInt (hDlg, IDC_ETEXT_WEIGHT_EDIT,
			   TempLogfont.lfWeight, FALSE);
#endif

    }
#ifndef NXSYSMac // don't have a private view that needs private invalidation. OS/X manages it.
    InvalidateRect (GetDlgItem (hDlg, IDC_ETEXT_SAMPLE), NULL, TRUE);
    if (HFont)
	DeleteObject(HFont);
#endif
    LOGFONT lf2;
    memcpy (&lf2, &TempLogfont, sizeof(LOGFONT));
    if (lf2.lfWeight == 0) lf2.lfWeight = NXSYS_DEFAULT_TEXT_WEIGHT;
    if (lf2.lfHeight == 0) lf2.lfHeight = NXSYS_DEFAULT_TEXT_HEIGHT;
    HFont = CreateFontIndirect (&lf2);
}

static void GetDlgElementsToLogfont (HWND hDlg) {
    BOOL es;
#ifndef NXSYSMac // non habemus italicum
    TempLogfont.lfItalic  = GetDlgItemCheckState (hDlg,IDC_ITALIC);
#endif
    if (GetDlgItemCheckState (hDlg, IDC_ETEXT_HEIGHT_CUSTOM))
	TempLogfont.lfHeight
		= (int)GetDlgItemInt(hDlg, IDC_ETEXT_HEIGHT_EDIT, &es, TRUE);
    else
	TempLogfont.lfHeight = 0;
#ifdef NXSYSMac
    TempLogfont.lfWidth = 0;
#else
    if (GetDlgItemCheckState (hDlg, IDC_ETEXT_WIDTH_CUSTOM))
	TempLogfont.lfWidth
		= (int)GetDlgItemInt(hDlg, IDC_ETEXT_WIDTH_EDIT, &es, TRUE);
    else
	TempLogfont.lfWidth = 0;

    if (GetDlgItemCheckState (hDlg, IDC_ETEXT_WEIGHT_CUSTOM))
	TempLogfont.lfWeight
		= (int)GetDlgItemInt(hDlg, IDC_ETEXT_WEIGHT_EDIT, &es, TRUE);
    else
#endif
        if (GetDlgItemCheckState (hDlg, IDC_ETEXT_WEIGHT_NORMAL))
	TempLogfont.lfWeight = FW_NORMAL;
    else if (GetDlgItemCheckState (hDlg, IDC_ETEXT_WEIGHT_BOLD))
	TempLogfont.lfWeight = FW_BOLD;
    else
	TempLogfont.lfWeight = 0;
}




BOOL TextString::DlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
#ifdef NXSYSMac
    BOOL es;
#else
    static CHOOSEFONT Cf;	/* says should be static, beats me */
#endif
    WP_cord new_wp_x = wp_x, new_wp_y = wp_y;
    char buf[200];

    switch (message) {
	case WM_INITDIALOG:
            SetDlgItemText (hDlg, IDC_TEXT_TEXT, String);
#ifdef NXSYSMac
            SetDlgItemText (hDlg, IDC_ETEXT_SAMPLE, String);
            SetDlgItemInt(hDlg, IDC_ETEXT_WPX, (int)wp_x, TRUE); // upd win dlg some day
            SetDlgItemInt(hDlg, IDC_ETEXT_WPY, (int)wp_y, TRUE);
#endif
	    memcpy (&TempLogfont, pOriginalLogfont, sizeof(LOGFONT));
	    TempColorGiven = ColorGiven;
	    TempColor = ColorGiven ? Color : TrackDftCol;
	    SetTSDlgState (hDlg);
	    return TRUE;
	case WM_COMMAND:
	    switch (wParam) {
		case IDOK:
#ifdef NXSYSMac
                    new_wp_x = GetDlgItemInt (hDlg, IDC_ETEXT_WPX, &es, FALSE);
                    if (!es) {
                        uerr (hDlg, "Bad number in Panel X coordinate.");
                        return TRUE;
                    }
                    new_wp_y = GetDlgItemInt (hDlg, IDC_ETEXT_WPY, &es, FALSE);
                    if (!es) {
                        uerr (hDlg, "Bad number in Panel Y coordinate.");
                        return TRUE;
                    }
#endif
		    Invalidate();
		    GetDlgItemText (hDlg, IDC_TEXT_TEXT, buf, sizeof(buf)-1);
		    free (String);
		    String = strdup(buf);
		    StringLength = strlen(String);

		    GetDlgElementsToLogfont(hDlg);
#ifdef NXSYSMac
                    /* grab face name from the Mac TextField serving as idc_etext_face_selection*/
                    if (GetDlgItemCheckState(hDlg, IDC_ETEXT_FACE_CUSTOM)) {
                        GetDlgItemText(hDlg,
                                       IDC_ETEXT_FACE_SELECTION,
                                       TempLogfont.lfFaceName,
                                       sizeof(TempLogfont.lfFaceName));
                    }
#endif
                    SetNewLogfont (&TempLogfont);

                    ComputeVisibleLast();

		    Color = TempColor;
		    ColorGiven = TempColorGiven;
                    Invalidate();

                    if (wp_x != new_wp_x || wp_y != new_wp_y) {
                        MoveWP(new_wp_x, new_wp_y);
                        Invalidate();
                    }

		    BufferModified = TRUE;
		    EndDialog (hDlg, TRUE);
		    return TRUE;

		case IDCANCEL:
		    EndDialog (hDlg, FALSE);
		    return TRUE;

		case IDC_ETEXT_FACE_DEFAULT:
		    TempLogfont.lfFaceName[0] = '\0';
		    goto restate;
	    
#ifndef NXSYSMac  //The "Custom" radiobutton is handled by an IB outlet.
		case IDC_ETEXT_FACE_CUSTOM:
		    ShowItem (hDlg, IDC_ETEXT_FACE_SELECT, 0, TRUE);
		    break;
#endif
		case IDC_ETEXT_HEIGHT_DEFAULT:
		    TempLogfont.lfHeight = 0;
		    ShowItem (hDlg, IDC_ETEXT_HEIGHT_EDIT,
			      IDC_ETEXT_HEIGHT_SPIN, FALSE);
		    goto restate;

		case IDC_ETEXT_HEIGHT_CUSTOM:
		    TempLogfont.lfHeight = NXSYS_DEFAULT_TEXT_HEIGHT;
		    ShowItem (hDlg, IDC_ETEXT_HEIGHT_EDIT,
			      IDC_ETEXT_HEIGHT_SPIN, TRUE);
		    goto restate;

		case IDC_ETEXT_WIDTH_DEFAULT:
		    TempLogfont.lfWidth = 0;
		    ShowItem (hDlg, IDC_ETEXT_WIDTH_EDIT,
			      IDC_ETEXT_WIDTH_SPIN, FALSE);
		    goto restate;

		case IDC_ETEXT_WIDTH_CUSTOM:
		    ShowItem (hDlg, IDC_ETEXT_WIDTH_EDIT,
			      IDC_ETEXT_WIDTH_SPIN, TRUE);
		    break;
#ifndef NXSYSMac  // The Face-Select button is gone on the mac; the Custom
                    //radio brings up the Mac fontmgr dialog.  All the CF stuff
                    // is Windows-specific and not worth simulting.
		case IDC_ETEXT_FACE_SELECT:
		    Cf.lStructSize = sizeof(CHOOSEFONT);
		    Cf.hwndOwner = hDlg;
		    Cf.lpLogFont = &TempLogfont;
		    Cf.Flags = CF_SCREENFONTS;
		    Cf.Flags |= CF_INITTOLOGFONTSTRUCT;
		    Cf.nFontType = SCREEN_FONTTYPE;
		    if (ChooseFont (&Cf))
			goto restate;
		    break;
#endif
		case IDC_ETEXT_WEIGHT_DEFAULT:
		    TempLogfont.lfWeight = 0;
		    goto restate;
		case IDC_ETEXT_WEIGHT_NORMAL:
		    TempLogfont.lfWeight = FW_NORMAL;
		    goto restate;
		case IDC_ETEXT_WEIGHT_BOLD:
		    TempLogfont.lfWeight = FW_BOLD;
		    goto restate;
		case IDC_ETEXT_WEIGHT_CUSTOM:
		    TempLogfont.lfWeight = FW_NORMAL+1;
		    goto restate;

		case IDC_ETEXT_COLOR_CUSTOM:
		    TempColorGiven = TRUE;
		    goto restate;

		case IDC_ETEXT_COLOR_DEFAULT:
		    TempColorGiven = FALSE;
		    goto restate;


#ifdef NXSYSMac 
                case IDC_MAC_GET_COLOR:
                    *(int*)lParam = TempColor;
                    break;
               
                case IDC_MAC_SEND_COLOR:
                    // rex cludgeorum hic ...
                    // overload this message cross-platform to give the mac
                    // side a way to feed the color back into the windows side.
                    TempColorGiven = TRUE;
                    TempColor = (COLORREF)lParam;
                    break;  //quelle clouge!
#else

                case IDC_ETEXT_COLOR_SELECT:
                {
		    CHOOSECOLOR Cc;
		    memset (&Cc, 0, sizeof(Cc));
		    Cc.lStructSize = sizeof(Cc);
		    Cc.hwndOwner = hDlg;
		    Cc.rgbResult = TempColor;
		    Cc.lpCustColors = CustColors;
		    Cc.Flags = CC_RGBINIT;
		    if (ChooseColor (&Cc)) {
			TempColor = Cc.rgbResult;
			TempColorGiven = TRUE;
			InvalidateRect (GetDlgItem (hDlg, IDC_ETEXT_SAMPLE),
					NULL, TRUE);
		    }
		    break;
		}
#endif
		case IDC_ITALIC:
		    TempLogfont.lfItalic = GetDlgItemCheckState
					   (hDlg, IDC_ITALIC);
		    goto restate;

		default:
		    return FALSE;
	    }
	    return TRUE;

	default:
	    return FALSE;
    }
restate:
    SetTSDlgState (hDlg);
    return TRUE;
}


#ifndef NXSYSMac  // don't need no sample pane class; NSTextField is completely adequate.

static LRESULT APIENTRY SamplePaneWndProc
   (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    PAINTSTRUCT PaintStruct;
    RECT r;
    char buf[200];
    HWND hDlg = GetParent(hWnd);

    switch (msg) {
	case WM_LBUTTONDOWN:
	    GetDlgElementsToLogfont(hDlg);
	    SetTSDlgState (hDlg);
	    InvalidateRect (hWnd, NULL, TRUE);
	    return 0L;

	case WM_PAINT:
	    GetDlgItemText (hDlg, IDC_TEXT_TEXT, buf, sizeof(buf)-1);
	    if (buf[strcspn (buf, " \t")] == '\0')
		strcpy (buf, "TEXT Sample!");
	    GetClientRect (hWnd, &r);
	    BeginPaint (hWnd, &PaintStruct);
	    SelectObject (PaintStruct.hdc,
			  HFont ? HFont : (HFONT)GetStockObject (SYSTEM_FONT));
	    SetBkColor (PaintStruct.hdc, RGB(0,0,0));
	    SetTextColor (PaintStruct.hdc,
			  TempColorGiven ? TempColor : TrackDftCol);
	    DrawText (PaintStruct.hdc, buf, -1, &r,
		      DT_LEFT | DT_SINGLELINE |DT_NOCLIP |
		      DT_CENTER | DT_VCENTER);
	    EndPaint (hWnd, &PaintStruct);
	    return 0L;

	default:
	    return DefWindowProc (hWnd, msg, wParam, lParam);
    }
}

BOOL RegisterTextSampleClass (HINSTANCE hInstance) {
   
   WNDCLASS klass;

   memset (&klass, 0, sizeof(klass));
   klass.style          = 0;
   klass.lpfnWndProc    = (WNDPROC) SamplePaneWndProc;
   klass.hInstance      = hInstance;
   klass.hIcon          = NULL;
   klass.hCursor        = LoadCursor (NULL, IDC_ARROW);
   klass.hbrBackground  = (HBRUSH)GetStockObject (BLACK_BRUSH);
   klass.lpszMenuName   = NULL;
   klass.lpszClassName  = "TLEdit:TextSample";

   return RegisterClass (&klass);
}
#endif
