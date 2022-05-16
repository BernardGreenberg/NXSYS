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
#include "STLExtensions.h"
#include "WinApiSTL.h"
#include "undo.h"

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

REGISTER_NXTYPE(TypeId::TEXT, CmText, IDD_EDIT_TEXT, CreateTextString, NULL);

void TextString::EditClick (int x, int y) {
    Dragon.ClickOn (G_mainwindow, this, "Text string", x, y);
}

/* ------------------------------------------------------------*/

static void DumpString (const char * s, GraphicObject::ObjectWriter& W) {
    W.putc ('"');
    for (const char * p = s; *p != '\0'; p++) {
	char c = *p;
	if (c == '\r') W.puts("\\r");
	else if (c == '\n') W.puts("\\n");
	else if (c == '\t') W.puts("\\t");
	else if (strchr ("\\\"", c))
	    W.putf("\\%c",c);
        else W.putc(c);
    }
    W.putc ('"');
}

int TextString::Dump (ObjectWriter& W) {
    W.puts("  (TEXT\t");
    DumpString(S.String.c_str(), W);
    W.putf("\n\t\t%4ld\t%4ld", wp_x, wp_y);
    LOGFONT& lf = RedeemLogfont();
    if (lf.lfFaceName[0] != '\0')
	W.putf("\n\tFACE\t\"%s\"", lf.lfFaceName);
    if (lf.lfHeight != 0)
	W.putf("\n\tHEIGHT\t%d", lf.lfHeight);
    if (lf.lfWidth != 0)
	W.putf("\n\tWIDTH\t%d\n", lf.lfWidth);
    if (lf.lfWeight != 0) {
	if  (lf.lfWeight == FW_NORMAL)
	    W.puts("\n\tWEIGHT\tNORMAL");
	else if (lf.lfWeight == FW_BOLD)
	    W.puts("\n\tWEIGHT\tBOLD");
        else if (lf.lfWeight == FW_BOLD)
	    W.putf("\n\tWEIGHT\t%d", lf.lfWeight);
    }
    if (lf.lfItalic)
	W.puts("\n\tITALIC\tT");

    if (S.ColorGiven)
        W.putf("\n\tCOLOR\t(%d %d %d)",
               GetRValue (S.Color), GetGValue (S.Color), GetBValue (S.Color));

    W.puts(")\n");
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




BOOL_DLG_PROC_QUAL TextString::DlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
#ifdef NXSYSMac
    BOOL es;
#else
    static CHOOSEFONT Cf{};	/* says should be static, beats me */
#endif
    WP_cord new_wp_x, new_wp_y; /* must be outside "switch" stmt */

    switch (message) {
	case WM_INITDIALOG:
            SetDlgItemTextS (hDlg, IDC_TEXT_TEXT, S.String);
#ifdef NXSYSMac
            SetDlgItemTextS (hDlg, IDC_ETEXT_SAMPLE, S.String);
            SetDlgItemInt(hDlg, IDC_ETEXT_WPX, (int)wp_x, TRUE); // upd win dlg some day
            SetDlgItemInt(hDlg, IDC_ETEXT_WPY, (int)wp_y, TRUE);
#endif
	    memcpy (&TempLogfont, &RedeemLogfont(), sizeof(LOGFONT));
	    TempColorGiven = S.ColorGiven;
	    TempColor = S.ColorGiven ? S.Color : TrackDftCol;
	    SetTSDlgState (hDlg);
            CacheInitSnapshot();
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
		    
                    S.String = GetDlgItemText (hDlg, IDC_TEXT_TEXT);

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

		    S.Color = TempColor;
		    S.ColorGiven = TempColorGiven;
                    Invalidate();

                    if (wp_x != new_wp_x || wp_y != new_wp_y) {
                        MoveWP(new_wp_x, new_wp_y);
                        Invalidate();
                    }

                    Undo::RecordChangedProps(this, StealPropCache());
		    EndDialog (hDlg, TRUE);
		    return TRUE;

		case IDCANCEL:
                    DiscardPropCache();
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
    HWND hDlg = GetParent(hWnd);

    switch (msg) {
	case WM_LBUTTONDOWN:
	    GetDlgElementsToLogfont(hDlg);
	    SetTSDlgState (hDlg);
	    InvalidateRect (hWnd, NULL, TRUE);
	    return 0L;

	case WM_PAINT:
        {
            std::string s = GetDlgItemText (hDlg, IDC_TEXT_TEXT);
	    if (s.c_str()[strcspn (s.c_str(), " \t")] == '\0')
                s = "TEXT Sample";
	    GetClientRect (hWnd, &r);
	    BeginPaint (hWnd, &PaintStruct);
	    SelectObject (PaintStruct.hdc,
			  HFont ? HFont : (HFONT)GetStockObject (SYSTEM_FONT));
	    SetBkColor (PaintStruct.hdc, RGB(0,0,0));
	    SetTextColor (PaintStruct.hdc,
			  TempColorGiven ? TempColor : TrackDftCol);
	    DrawText (PaintStruct.hdc, s.c_str(), -1, &r,
		      DT_LEFT | DT_SINGLELINE |DT_NOCLIP |
		      DT_CENTER | DT_VCENTER);
	    EndPaint (hWnd, &PaintStruct);
	    return 0L;
        }

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

