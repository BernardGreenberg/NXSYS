#ifndef _NX_PANEL_SWITCH_H__
#define _NX_PANEL_SWITCH_H__

#ifdef TLEDIT
class Relay;
#ifndef _FILE_DEFINED
#include <stdio.h>
#define _FILE_DEFINED
#endif
#endif

#include <string>

class PanelSwitch : public GraphicObject {
  public:
    char NumStr[8];
    int NumStrLen;
    int XlkgNo;
    BOOL State;

    PanelSwitch (int xlkg_no, WP_cord p_wpx, WP_cord p_wpy, const char * relay_nom);
    void SetXlkgNo(int xno);


    virtual void Display (HDC hdc);
    virtual ObjId TypeID();
    virtual int ObjIDp(long);
#ifndef TLEDIT
    Relay * Rly;
    virtual void Hit (int mb);
#else
    std::string RelayNomenclature;
    virtual void EditClick(int x, int y);
    virtual ~PanelSwitch();
    virtual BOOL_DLG_PROC_QUAL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
    virtual int Dump (ObjectWriter& W);
#endif
};
void InitPanelSwitchData();

#endif
