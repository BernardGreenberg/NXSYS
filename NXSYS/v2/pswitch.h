#ifndef _NX_PANEL_SWITCH_H__
#define _NX_PANEL_SWITCH_H__

#include "PropCell.h"
#include <string>

class PanelSwitch : public GraphicObject, public PropEditor<PanelSwitch> {
  public:
    char NumStr[8];
    int NumStrLen;
    int XlkgNo;
    BOOL State;

    PanelSwitch (int xlkg_no, WP_cord p_wpx, WP_cord p_wpy, const char * relay_nom);
    void SetXlkgNo(int xno);

    virtual void Display (HDC hdc);
    virtual TypeId TypeID();
    virtual bool IsNomenclature(long);
#ifndef TLEDIT
    class Relay * Rly;
    virtual void Hit (int mb);
#else
    class PropCell : public PropCellPCRTP<PropCell, PanelSwitch> {
        int XlkgNo;
        std::string RelayNomenclature;
    public:
        void Snapshot_(PanelSwitch* p) {
            SnapWPpos(p);
            XlkgNo = p->XlkgNo;
            RelayNomenclature = p->RelayNomenclature;
        }
        void Restore_(PanelSwitch* p) {
            RestoreWPpos(p);
            p->SetXlkgNo(XlkgNo);
            p->RelayNomenclature = RelayNomenclature;
        }
    };
    
    std::string RelayNomenclature;
    virtual void EditClick(int x, int y);
    virtual ~PanelSwitch();
    virtual BOOL_DLG_PROC_QUAL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
    virtual int Dump (ObjectWriter& W);
#endif
};
void InitPanelSwitchData();

#endif
