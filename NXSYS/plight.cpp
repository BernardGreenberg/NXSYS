#include "windows.h"
// Panel Lights are V2 only
#include "nxgo.h"  //wp_limits stuff

#include "plight.h"
#include "brushpen.h"
#include "objid.h"

/* Common stuff to main app and TLEdit   -- 13 Aug 2019 */
void PanelLight::SetRadius (int r) {
    Radius = r;
    wp_limits.left   = -Radius-1;
    wp_limits.right  = +Radius+1;
    wp_limits.top =    -Radius-1;
    wp_limits.bottom = +Radius+1;
}

PanelLight::PanelLight (int xno, int radius, WP_cord p_wpx, WP_cord p_wpy,
			   const char* ) :
   XlkgNo(xno)
{
    wp_x = p_wpx;
    wp_y = p_wpy;
    SetRadius (radius);
}

// Used in both main app and TLEdit
void PanelLight::Paint(HDC hdc, HBRUSH brush) {
    int rad = (int)(Radius * NXGO_Scale + .8);
    SelectObject (hdc, GetStockObject (NULL_PEN));
    SelectObject (hdc, brush);
    Ellipse(hdc, sc_x - rad, sc_y - rad, sc_x + rad, sc_y + rad);
}


ObjId PanelLight::TypeID() {
    return ObjId::PANELLIGHT;
}

int PanelLight::IsNomenclature(long id) {
    return XlkgNo == id;
}

//  These get called from both the track loader and TLEdit.
void PanelLight::AddAspect (COLORREF color, const char *relay_name) {
    Aspects.emplace_back(color, nullptr, relay_name, this);   // construct PanelLightAspect in place!
}

void PanelLight::AddAspect (const char* color_name, const char * relay_name) {
    Aspects.emplace_back((COLORREF) 0, color_name, relay_name, this);  //construct PanelLightAspect in place!
}


