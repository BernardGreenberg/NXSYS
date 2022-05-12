#include "windows.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "nxgo.h"
#include "xtgtrack.h"
#include "tledit.h"
#include "plight.h"
#include "resource.h"
#include "tlpropdlg.h"
#include "objreg.h"
#include "brushpen.h"

#include "dragger.h"
#include "STLExtensions.h"
#include "WinApiSTL.h"

#include <string>

static Dragger Dragon;

PanelLight::~PanelLight() {
    /* empty right now */
}

static struct PLColorData {
    const char * color_letter;
    UINT    control_id;
} ColorData[]{
    {"R", IDC_PANELLIGHT_RED},
    {"Y", IDC_PANELLIGHT_YELLOW},
    {"G", IDC_PANELLIGHT_GREEN},
    {"W", IDC_PANELLIGHT_WHITE}
};


static GraphicObject* CreatePanelLight (int wpx, int wpy) {
    if (Dragon.Movingp())
	return NULL;
    return Dragon.StartMoving (new PanelLight (0, 10, wpx, wpy, "New"), "New Panel Light", G_mainwindow);
}

void null_f() {};

REGISTER_NXTYPE(TypeId::PANELLIGHT, CmPanelLight, IDD_PANELLIGHT, CreatePanelLight, null_f);

static const std::string GetPLDlgString (HWND hDlg, UINT id) {
    return stoupper(GetDlgItemText(hDlg, id));
}


void PanelLight::EditClick (int x, int y) {
    char d[30];
    sprintf (d, "Panel Light %d", XlkgNo);
    Dragon.ClickOn (G_mainwindow, this, d, x, y);
}

int PanelLight::Dump (ObjectWriter& W) {
    W.putf("  (PANELLIGHT %d %d %d\t%s\t%4ld %4ld",
           1, XlkgNo, Radius, "\"\"", wp_x, wp_y);
    for (auto& aspect : Aspects)
        W.putf("(%s %s)", aspect.Colorstring.c_str(), aspect.RelayName.c_str());
    W.putf(")\n");

    return 520;				/* dump order */
}

// Special version for TLEdit
PanelLightAspect::PanelLightAspect(COLORREF color, const char * color_name, const char * relay_name, PanelLight* pl) {
    
    PLight = pl;
    Color = color;                    //selects slot in TLEDIT
    Colorstring = color_name ? color_name : "";
    RelayName = relay_name;           //edited in TLEDIT
}

//special version for TLEdit
void PanelLight::Display (HDC hdc) {
    Paint(hdc, Selected ? GKGreenBrush : GKOffBrush);
}

BOOL_DLG_PROC_QUAL PanelLight::DlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    BOOL es;
    switch (message) {
	case WM_INITDIALOG:
	{
	    SetDlgItemInt  (hDlg, IDC_PANELLIGHT_LEVER, XlkgNo, FALSE);
	    SetDlgItemInt  (hDlg, IDC_PANELLIGHT_WPX, (int)wp_x, FALSE);
	    SetDlgItemInt  (hDlg, IDC_PANELLIGHT_WPY, (int)wp_y, FALSE);
	    SetDlgItemText (hDlg, IDC_PANELLIGHT_STRING, "");
	    SetDlgItemInt  (hDlg, IDC_PANELLIGHT_RADIUS, Radius, FALSE);

	    SetDlgItemText (hDlg, IDC_PANELLIGHT_RED, "");
	    SetDlgItemText (hDlg, IDC_PANELLIGHT_YELLOW, "");
	    SetDlgItemText (hDlg, IDC_PANELLIGHT_GREEN, "");
	    SetDlgItemText (hDlg, IDC_PANELLIGHT_WHITE, "");

            for (auto& aspect : Aspects) {
                for (auto& colord : ColorData)
		    if (aspect.Colorstring == colord.color_letter)
                        SetDlgItemTextS (hDlg, colord.control_id, aspect.RelayName);
	    }
	    return TRUE;
	}
	case WM_COMMAND:
	    switch (wParam) {
		case IDOK:
		{
		    long newnom = GetDlgItemInt (hDlg, IDC_PANELLIGHT_LEVER, &es, FALSE);
		    if (!es) {
			uerr (hDlg, "Bad lever number.");
			return TRUE;
		    }

		    int newrad  = GetDlgItemInt (hDlg, IDC_PANELLIGHT_RADIUS, &es, FALSE);
		    if (!es || newrad <= 3) {
			uerr (hDlg, "Bad radius.");
			return TRUE;
		    }
                    
                    for (auto& colord : ColorData) {
                        std::string data = GetPLDlgString(hDlg, colord.control_id);
                        if (!data.empty() && strchr("0123456789", data[0])) {
                            uerr(hDlg, "Bad relay nomenclature (no number!)");
                            return TRUE;
                        }
                    }

		    if (!InstallDlgLights (hDlg))
			return TRUE;

		    if (newnom != XlkgNo) {
                        XlkgNo =(int)newnom;
			StatusMessage ("Panel light/%ld", newnom);
			Invalidate();
			BufferModified = TRUE;
		    }

		    if (newrad != Radius) {
			SetRadius(newrad);
			Invalidate();
			ComputeVisibleLast();
			BufferModified = TRUE;
		    }
		}


		{
		    WP_cord new_wp_x = GetDlgItemInt (hDlg, IDC_PANELLIGHT_WPX, &es, FALSE);
		    if (!es) {
			uerr (hDlg, "Bad number in Panel X coordinate.");
			return TRUE;
		    }
		    WP_cord new_wp_y = GetDlgItemInt (hDlg, IDC_PANELLIGHT_WPY, &es, FALSE);
		    if (!es) {
			uerr (hDlg, "Bad number in Panel Y coordinate.");
			return TRUE;
		    }
		    if (wp_x != new_wp_x || wp_y != new_wp_y) {
			MoveWP(new_wp_x, new_wp_y);
			BufferModified = TRUE;
		    }
		}
		EndDialog (hDlg, TRUE);
		return TRUE;

		case IDCANCEL:
		    EndDialog (hDlg, FALSE);
		    return TRUE;
		default:
		    return FALSE;
	    }
	default:
	    return FALSE;
    }
    return FALSE;
}


bool PanelLight::InstallCheckCorrespondence(HWND hDlg) {
    for (auto& aspect : Aspects) {
        for (auto& colord : ColorData)
	    if (aspect.Colorstring != colord.color_letter) {
		if (GetPLDlgString(hDlg, colord.control_id) == aspect.RelayName)
                    break; // next_aspect
		else
                    return false;
            }
    }
    
    for (auto& colord : ColorData) {
	if (!GetPLDlgString (hDlg, colord.control_id).empty()) {
            bool found = false;
            for (auto& aspect : Aspects) {
                if (aspect.Colorstring == colord.color_letter) {
                    found = true;
                    break;
                }
            }
            if (!found)
                return false;
	}
    }
    return true;
}

    
BOOL PanelLight::InstallDlgLights (HWND hDlg) {

    /* don't have to free anything any more */
    
    if (InstallCheckCorrespondence(hDlg))
        return TRUE;

    Aspects.clear();

    for (auto& colord : ColorData) {
        std::string relay_name = GetPLDlgString (hDlg, colord.control_id);
	if (!relay_name.empty())
            AddAspect (colord.color_letter, relay_name.c_str());
    }

    BufferModified = TRUE;
    return TRUE;
}
