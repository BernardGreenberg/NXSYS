#ifndef _NX_PANEL_LIGHT_H__
#define _NX_PANEL_LIGHT_H__

#if !TLEDIT
#include "relays.h"
#endif


class PanelLight;

#include <vector>
#include <string>

class PanelLightAspect
{
  public:
    PanelLightAspect(COLORREF, const char* color_name, const char * relay_nomenclature, PanelLight* pl);

    friend class PanelLight;

    PanelLight * PLight;

#ifdef TLEDIT
    std::string RelayName;
    COLORREF Color;
    std::string Colorstring;
#else
    static void Reporter(BOOL state, void*);
    ReportingRelay *ReporterRelay;   //no move any more
    void ProcessLoadComplete();
    void Report (bool state);
    HBRUSH Brush;
    bool active;
    std::string relay_type_name;
#endif
#if DEBUG
    std::string obj_str;
    const char * get_mover_objid() {
        return obj_str.c_str();
    }
#endif
};

class PanelLight : public GraphicObject {

  public:
    int XlkgNo;

    PanelLight (int xlkg_no, int radius, WP_cord p_wpx, WP_cord p_wpy, const char * string);
    int Radius;

    std::vector<PanelLightAspect> Aspects;
    
    void AddAspect (COLORREF, const char * name);
    void AddAspect (const char*, const char * name);
    void SetRadius(int rad);
    void Paint(HDC hdc, HBRUSH brush);
    virtual void Display (HDC hdc);
    virtual int TypeID(), ObjIDp(long);
#ifdef TLEDIT
    BOOL InstallDlgLights (HWND hDlg);
    bool InstallCheckCorrespondence(HWND hDlg);
    virtual void EditClick(int x, int y);
    virtual ~PanelLight();
    virtual BOOL_DLG_PROC_QUAL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
    virtual int Dump (ObjectWriter& W);
#else
    void ClearAllActives();
    void ProcessLoadComplete();
#endif

};

void InitPanelLightData();

#endif
