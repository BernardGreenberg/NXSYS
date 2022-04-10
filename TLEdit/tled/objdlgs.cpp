#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>

#include "compat32.h"
#include "nxgo.h"
#include "xtgtrack.h"
#include "tledit.h"
#include "assignid.h"

#include "tlecmds.h"
#include "resource.h"
#include "objreg.h"
#include <string>
#include <utility>
#include "STLExtensions.h"
#include "WinApiSTL.h"

#include "SwitchConsistency.h"

#define CIRCUIT_SHOW_MS 2000

#ifdef NXSYSMac
void Sleep(long);
void MacDialogDispatcher(UINT did, GraphicObject *Object);
#endif

static UINT SwitchIsIDs [3] = {IDC_SWITCH_IS_SINGLETON, IDC_SWITCH_IS_A, IDC_SWITCH_IS_B};

UINT TrackJoint::DlgId () {
    return  EditAsSwitchP() ? IDD_SWITCH_ATTR : IDD_JOINT;
    
}

UINT TrackSeg::DlgId () {return IDD_SEG_ATTRIBUTES;}


void uerr (HWND hDlg, const char * control_string, ... ){
    va_list arg_ptr;
    va_start (arg_ptr, control_string);
    std::string msg = FormatStringVA(control_string, arg_ptr);
    MessageBox (hDlg, msg.c_str(), app_name, MB_OK| MB_ICONEXCLAMATION);
}

#ifndef NXSYSMac // do natively
void SetDlgItemCheckState (HWND hDlg, UINT id, BOOL state) {
    SendMessage (GetDlgItem (hDlg, id), BM_SETCHECK, (WPARAM) state, 0L);
}

BOOL GetDlgItemCheckState (HWND hDlg, UINT id) {
    return (BOOL)
	    SendMessage (GetDlgItem (hDlg, id), BM_GETCHECK, 0, 0L);
}

#endif

BOOL_DLG_PROC_QUAL TrackJoint::SwitchDlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    BOOL es;

    switch (message) {
	case WM_INITDIALOG:
	    SetDlgItemInt (hDlg, IDC_SWITCH_EDIT, (int)Nomenclature, FALSE);
	    SetDlgItemCheckState (hDlg, SwitchIsIDs[SwitchAB0], TRUE);
	    return TRUE;
	case WM_COMMAND:
	    switch (wParam) {
		case IDOK:
		{
		    int AB0 = 0;
		    int nchange = 0;
		    for (int i = 0; i < 3; i++)
			if (GetDlgItemCheckState (hDlg, SwitchIsIDs[i]))
			    AB0 = i;
		    if (AB0 != SwitchAB0)
			nchange = 1;
		    long newnom = GetDlgItemInt (hDlg, IDC_SWITCH_EDIT, &es, FALSE);
		    if (!es) {
			uerr (hDlg, "Bad number in Switch ID.");
			return TRUE;
		    }
		    if (newnom != Nomenclature) {
                        std::string complaint;
                        if (!SwitchConsistencyDefineCheck(newnom, AB0, complaint)) {
                            uerr(hDlg, complaint.c_str());
                            return TRUE;
                            
                        }
                        SwitchConsistencyUndefine(Nomenclature, SwitchAB0);
                        SwitchConsistencyDefine(newnom, AB0, complaint);
			Nomenclature = newnom;
			nchange = 1;
		    }	
		    SwitchAB0 = AB0;
		    if (nchange)
			PositionLabel();
		    EndDialog (hDlg, TRUE);
		    return TRUE;
		}
		case IDC_SWITCH_HILITE_NORMAL:
		    if (!Organized)
			Organize();
		    TSA[(int)TSAX::NORMAL]->Select();
		    break;
		case IDC_SWITCH_SWAP_NORMAL:
		    if (!Organized)
			Organize();

		    std::swap (TSA[(int)TSAX::NORMAL], TSA[(int)TSAX::REVERSE]);
		    BufferModified = TRUE;
		    TSA[(int)TSAX::NORMAL]->Select();
		    break;
		case IDCANCEL:
		    EndDialog (hDlg, FALSE);
		    return TRUE;
		case IDC_SWITCH_EDIT_JOINT_ATTRIBUTES:
                    EditAsJointInProgress = true;
                    EditProperties();  /* quel crocque ...*/
                    EditAsJointInProgress = false;
                    return TRUE;
		default:
		    return FALSE;
	    }
	default:
	    return FALSE;
    }

}

BOOL_DLG_PROC_QUAL TrackJoint::DlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL es;

    if (EditAsSwitchP())
	return SwitchDlgProc (hDlg, message, wParam, lParam);

    switch (message) {
	case WM_INITDIALOG:
	    SetDlgItemCheckState (hDlg, IDC_JOINT_INSULATED, Insulated);
	    SetDlgItemInt (hDlg, IDC_JOINT_STATION_ID, (int)Nomenclature, FALSE);
	    SetDlgItemInt (hDlg, IDC_JOINT_WPX, (int)wp_x, FALSE);
	    SetDlgItemInt (hDlg, IDC_JOINT_WPY, (int)wp_y, FALSE);
            
            /* Can't set these for a switch. Use Switch dlg. */
            if (EditAsJointInProgress) {
               EnableWindow(GetDlgItem(hDlg, IDC_JOINT_INSULATED), false);
               EnableWindow(GetDlgItem(hDlg, IDC_JOINT_STATION_ID), false);
            }
	    return TRUE;

	case WM_COMMAND:
	    switch (wParam) {
		case IDOK:
		{
		    int new_ins = GetDlgItemCheckState (hDlg, IDC_JOINT_INSULATED);
		    if (new_ins == 0 && SignalCount() > 0) {
			uerr (hDlg, "Can't make non-insulated as signals are at this joint.",
			      "TLEDIT Joint properties");
			return TRUE;
		    }
		}
		
		{
		    long newnom = GetDlgItemInt (hDlg, IDC_JOINT_STATION_ID, &es, FALSE);
                    if (!es) {
                        uerr (hDlg, "Bad number in Joint ID.");
                        return TRUE;
                    }
		    if (newnom != Nomenclature) {
                        /* 3-23-2022 -- ok to say 0 to delete nomenclature */
                        if (newnom == 0){
                            DeAssignID ((int)Nomenclature);
                            Nomenclature = 0;
                            delete Lab;
                            Lab = NULL;
                        }
                        else {
                            if (!CheckID((int) newnom)) {
                                uerr(hDlg, "ID %ld is bad or already in use.", newnom);
                                return TRUE;
                            }
                            DeAssignID ((int)Nomenclature);
                            Nomenclature = newnom;
                            MarkIDAssign ((int)Nomenclature);
                            delete Lab;
                            Lab = NULL;
                            PositionLabel();
                        }
		    }
		}

		{
		    WP_cord new_wp_x = GetDlgItemInt (hDlg, IDC_JOINT_WPX, &es, FALSE);
		    if (!es) {
			uerr (hDlg, "Bad number in Panel X coordinate.");
			return TRUE;
		    }
		    WP_cord new_wp_y = GetDlgItemInt (hDlg, IDC_JOINT_WPY, &es, FALSE);
		    if (!es) {
			uerr (hDlg, "Bad number in Panel Y coordinate.");
			return TRUE;
		    }
		    if (wp_x != new_wp_x || wp_y != new_wp_y)
			MoveToNewWPpos(new_wp_x, new_wp_y);
		}
		Insulated = GetDlgItemCheckState (hDlg, IDC_JOINT_INSULATED);
		BufferModified = TRUE;
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
}

BOOL_DLG_PROC_QUAL TrackSeg::DlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL es;

    switch (message) {
	case WM_INITDIALOG:
	    if (Circuit)
		SetDlgItemInt (hDlg, IDC_EDIT_SEG_TC, (int)(Circuit->StationNo), FALSE);
	    else
		SetDlgItemText (hDlg, IDC_EDIT_SEG_TC, "");
	    SetDlgItemCheckState (hDlg, IDC_EDIT_SEG_SPREAD,
				  !Circuit || Circuit->MultipleSegmentsP());
		    return TRUE;
	case WM_COMMAND:
#if 0
	    if (Circuit) {
		if (LOWORD(wParam) == IDC_EDIT_SEG_SHOW_TC) {
		    if (HIWORD(wParam) == BN_PUSHED)
			Circuit->SetOccupied(TRUE);
		    else if (HIWORD(wParam) == BN_UNPUSHED)
			Circuit->SetOccupied(FALSE);
		    UpdateWindow(G_mainwindow);
		    return TRUE;
		}
	    }
#endif
	    switch (wParam) {
		case IDCANCEL:
		    EndDialog (hDlg, FALSE);
		    return TRUE;
		case IDOK:
		{
		    long newid;
                    std::string text = GetDlgItemText (hDlg, IDC_EDIT_SEG_TC);
                    if (text.empty())
			newid = 0;
		    else {
			newid = GetDlgItemInt (hDlg, IDC_EDIT_SEG_TC, &es ,FALSE);
			if (!es) {
			    uerr(hDlg, "Bad track circuit ##: %s", text.c_str());
			    return TRUE;
			}
		    }
		    long oldid = Circuit ? Circuit->StationNo : 0L;
		    if (oldid != newid) {
			SetTrackCircuit
				(newid,
				 GetDlgItemCheckState
				 (hDlg, IDC_EDIT_SEG_SPREAD))
				->SetRouted(TRUE);
			BufferModified = TRUE;
		    }
		    SelectMsg();
		    EndDialog (hDlg, TRUE);
		    return TRUE;
		}
		case IDC_EDIT_SEG_SHOW_TC:

                    if (Circuit) {
#ifdef NXSYSMac
                        Circuit->SetOccupied((lParam == 0) ? TRUE : FALSE );
#else // unsurprisingly, doesn't fly at all on the Mac.  Sleep blocks updates.
      //Mac not unscathed on this account - see caller.
                        Circuit->SetOccupied(TRUE);
                        UpdateWindow(G_mainwindow);
                        Sleep(CIRCUIT_SHOW_MS);
			Circuit->SetOccupied(FALSE);
                        UpdateWindow(G_mainwindow);
#endif
		    }
		    return TRUE;
		default:
		    return FALSE;
	    }
	default:
	    return FALSE;
    }
}

#ifdef _WIN32
static DLGPROC_DCL GeneralDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    if (message == WM_INITDIALOG)
	SetWindowLongPtr (hDlg, GWLP_USERDATA, lParam);
    LONG_PTR l = GetWindowLongPtr (hDlg, GWLP_USERDATA);
    if (l)
	return (BOOL)((GraphicObject *) l)->DlgProc(hDlg, message, wParam, lParam);
    else
	return FALSE;
}
#endif


void GraphicObject::EditProperties() {
    if (UINT id = DlgId())
#ifdef NXSYSMac
        MacDialogDispatcher(id, this);
#else
        DialogBoxParam (app_instance, MAKEINTRESOURCE(id), G_mainwindow,
                        (DLGPROC) GeneralDlgProc, (LPARAM) this);
#endif
}

UINT GraphicObject::DlgId () {
    return  FindDialogIdFromObjClassRegistry (TypeID());
}

BOOL_DLG_PROC_QUAL GraphicObject::DlgProc (HWND, UINT, WPARAM, LPARAM) {
    return FALSE;
}

