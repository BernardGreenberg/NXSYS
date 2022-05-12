#ifndef _NX_SWITCH_KEY_H__
#define _NX_SWITCH_KEY_H__

#ifdef TLEDIT
#include <stdio.h>
#endif

#include "RelayMovingPointer.h"

#ifdef TLEDIT
class SwitchKey : public GraphicObject {
#else
class SwitchKey : public RelayMovingPointer<SwitchKey>, public GraphicObject {
#endif

  public:
    char NumStr[8];
    int NumStrLen;
    int XlkgNo;
#ifdef TLEDIT
    SwitchKey (int xlkg_no, WP_cord p_wpx, WP_cord p_wpy);
#else
    void AssociateTurnout (Turnout * t);
    SwitchKey (Turnout * t, WP_cord p_wpx, WP_cord p_wpy);
    Turnout * Turn;
#endif
    BOOL Normal, Reverse, HitWasControl, LSState;
    BOOL XIntersectP (WP_cord wp1, WP_cord wp2);
    void LSReport (BOOL state);
    void ClearAux();
    void SetTurnSwkeyFlags();
    void SetXlkgNo(int xno);
    BOOL Press (BOOL reverse, BOOL lock);

    virtual void Display (HDC hdc);
    virtual ObjId TypeID();
    virtual int IsNomenclature(long);
#ifndef TLEDIT
    virtual void Hit (int mb);
    virtual void UnHit();
    static void LSReporter (BOOL state, void * v);
#endif

#ifdef TLEDIT
    virtual BOOL_DLG_PROC_QUAL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
    virtual void EditClick(int x, int y);
    virtual ~SwitchKey();
    virtual int Dump (ObjectWriter& W);
#else
    virtual void EditContextMenu (HMENU m);
#endif

};

void InitSwitchKeyData();

#endif

