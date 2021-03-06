#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <regex>

#include "compat32.h"
#include "nxgo.h"
#include "xtgtrack.h"
#include "tledit.h"
#include "assignid.h"
#include "undo.h"

#include "tlecmds.h"
#include "resource.h"
#include "objreg.h"
#include <string>
#include <utility>
#include "STLExtensions.h"
#include "WinApiSTL.h"
#include "ValidatingValue.h"

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

bool GraphicObject::CheckGONumberReuse(HWND hDlg, long nomenclature) {
    GraphicObject* g = FindObjectByNomAndType(nomenclature, TypeID());
    if (!g)
        return true;   /* not yet established */
    if (g == this)
        return true;
    uerr(hDlg, "Lever number %ld is already in use by another %s.",
         nomenclature,
         NXObjectTypeName(TypeID()));
    return false;
}

static std::regex SwLeverPat("^(\\d{1,5})(A|B)?$", std::regex_constants::icase);
static ValidatingValue<std::pair<int, int>> ParseSwitchLeverField(const std::string s) {
    std::smatch sm;
    if (regex_match(s, sm, SwLeverPat)) {
        int swno = std::stoi(sm[1].str());
        if (sm[2].length()== 0)
            return std::pair<int,int>(swno, 0);
        else
            return std::pair<int, int>(swno, stoupper(sm[2].str()).c_str()[0] - 'A' + 1);
    }
    return {};
}

BOOL_DLG_PROC_QUAL TrackJoint::SwitchDlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    /* This really needs a dialog-instance object, as in both MFC and Cocoa, but ...*/
    static SwitchBranchSnapshot SSBS;
    switch (message) {
	case WM_INITDIALOG:
	    SetDlgItemInt (hDlg, IDC_SWITCH_EDIT, (int)Nomenclature, FALSE);
	    SetDlgItemCheckState (hDlg, SwitchIsIDs[SwitchAB0], TRUE);
            CacheInitSnapshot();
            SSBS.Init(TSA);  /* snapshot the branches */
	    return TRUE;
	case WM_COMMAND:
	    switch (LOWORD(wParam)) {
		case IDOK:
		{
		    int AB0 = 0;
		    int nchange = 0;
		    for (int i = 0; i < 3; i++)
			if (GetDlgItemCheckState (hDlg, SwitchIsIDs[i]))
			    AB0 = i;
		    if (AB0 != SwitchAB0)
			nchange = 1;
                    IJID newnom;
                    {
                        if (auto vv = ParseSwitchLeverField(GetDlgItemText(hDlg, IDC_SWITCH_EDIT)))
                            newnom = vv.value.first;
                        else {
                            uerr (hDlg, "Bad number in Switch ID.");
                            return TRUE;
                        }
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
                    
                    if (nchange || SSBS != SwitchBranchSnapshot (TSA)) {
			PositionLabel();
                        Undo::RecordChangedJointProps(this, StealPropCache());
                    }
                    else {
                        DiscardPropCache();
                    }
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
		    TSA[(int)TSAX::NORMAL]->Select();
		    break;
		case IDCANCEL:
                    RestorePropCache();
                    DiscardPropCache();
		    EndDialog (hDlg, FALSE);
		    return TRUE;
		case IDC_SWITCH_EDIT_JOINT_ATTRIBUTES:
                    EditAsJointInProgress = true;
                    EditProperties();  /* quel crocque ...*/
                    EditAsJointInProgress = false;
                    return TRUE;
                case IDC_SWITCH_EDIT:  /* poorly-named "lever number" */
                    if (auto vv = ParseSwitchLeverField(GetDlgItemText(hDlg, IDC_SWITCH_EDIT))) {
                        for (int i = 0; i < 3; i++)
                            SetDlgItemCheckState (hDlg, SwitchIsIDs[i], i == vv.value.second);
                    }
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
            } else {
                CacheInitSnapshot();
            }
	    return TRUE;

	case WM_COMMAND:
	    switch (wParam) {
		case IDOK:
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
                    decltype(Insulated) new_ins = GetDlgItemCheckState (hDlg, IDC_JOINT_INSULATED);
                    if (!new_ins) {   /* Attempting to merge segments  */
                        if (auto excuse = PrecludeUninsulation("uninsulate")) {
                            uerr(hDlg, excuse.value.c_str(), "TLEDIT Joint properties");
                            return TRUE;
                        }
                    }
		    decltype(Nomenclature) newnom = GetDlgItemInt (hDlg, IDC_JOINT_STATION_ID, &es, FALSE);
                    if (!es) {
                        uerr (hDlg, "Bad number in Joint ID.");
                        return TRUE;
                    }
                    
                    if (newnom != Nomenclature && newnom && !CheckID((int)newnom)) {
                        uerr(hDlg, "ID %ld is bad or already in use.", newnom);
                        return TRUE;
                    }

                    /* Commit Point */

		    if (newnom != Nomenclature) {
                        /* 3-23-2022 -- ok to say 0 to delete nomenclature */
                        if (newnom == 0){
                            DeAssignID ((int)Nomenclature);
                            Nomenclature = 0;
                            delete Lab;
                            Lab = NULL;
                        }
                        else {
                            DeAssignID ((int)Nomenclature);
                            Nomenclature = newnom;
                            MarkIDAssign ((int)Nomenclature);
                            delete Lab;
                            Lab = NULL;
                            PositionLabel();
                        }
		    }

		    if (wp_x != new_wp_x || wp_y != new_wp_y)
			MoveToNewWPpos(new_wp_x, new_wp_y);

                    Insulated = new_ins;
                    if (!EditAsJointInProgress)
                        Undo::RecordChangedJointProps(this, StealPropCache());
                    EndDialog (hDlg, TRUE);
                }
		return TRUE;

		case IDCANCEL:
                    if (!EditAsJointInProgress)
                        DiscardPropCache();
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
        {
	    if (Circuit)
		SetDlgItemInt (hDlg, IDC_EDIT_SEG_TC, (int)(Circuit->StationNo), FALSE);
	    else
		SetDlgItemText (hDlg, IDC_EDIT_SEG_TC, "");
            SegmentGroupMap SGM;
            CollectContacteesRecurse(SGM);
            int brethren = (int)SGM.size();
            if (brethren == 1)
                SetDlgItemText(hDlg, IDC_EDIT_SEG_NSEGS, "One track segment");
            else
                SetDlgItemTextS(hDlg, IDC_EDIT_SEG_NSEGS, std::to_string(brethren) + " segments in circuit");
        }
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
		    IJID newid;
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
                    SegmentGroupMap SGM;
                    CollectContacteesRecurse(SGM);
                    auto [real, imaginary] = TrackSeg::AnalyzeSegmentGroup(SGM);
                    if (real > 1) {
                        uerr(hDlg, "The group of segments reachable by non-insulated joints includes "
                             "more than one assigned track circuit. Create and/or insulate some "
                             "joints and try again.");
                        EndDialog(hDlg, FALSE);
                        return TRUE;
                    }
                    SetTrackCircuitWildfire(newid);
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

