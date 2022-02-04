//
//  plight_apponly.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/13/19.  Recordaremur.
//  Split off from plight.cpp -- this file is NOT USED in TLEdit.
//  Copyright © 2019 BernardGreenberg. All rights reserved.
//
#ifdef TLEDIT
#error Should not be compiling plight_apponly.cpp in TLEdit
#endif

#include <string>
#include <map>

#include "STLExtensions.h"
#include "windows.h"
#include "nxgo.h"
#include "brushpen.h"
#include "rlyapi.h"
#include "plight.h"

static std::map<COLORREF, HBRUSH> RandomBrushes;
static std::map<std::string, HBRUSH*> BrushesByColorLetter
{
    {"R", &GKRedBrush},
    {"G", &GKGreenBrush},
    {"Y", &GKYellowBrush},
    {"W", &GKWhiteBrush}
};

// TLEdit has its own version.
void PanelLight::Display (HDC hdc) {
    for (auto& aspect : Aspects)
        if (aspect.active) {
            Paint(hdc, aspect.Brush);
            return;
        }
    /* none selected, draw off-state grey */
    Paint (hdc, GKOffBrush);
}

/* Callback from the relay system when a given color (aspect) changes state */
void PanelLightAspect::Reporter (BOOL state, void * v) {
    ((PanelLightAspect *)v)->Report((bool)state);
}

void PanelLightAspect::Report (bool state) {
    if (state)  // if we're turning "off", don't clear what might be on.
        PLight->ClearAllActives();
    active = state;
    PLight->Invalidate();
}

void PanelLight::ClearAllActives() {
    for (auto& aspect : Aspects)
        aspect.active = false;
}


//  This is the true and only constructor of PanelLightAspect's in true-NXSYS.
// TLEdit has its own version of this with no brushes or relay pointers.
PanelLightAspect::PanelLightAspect(COLORREF color_ref, const char * color_name, const char * relay_nomenclature, PanelLight* pl) :
PLight(pl), active(false), relay_type_name(relay_nomenclature)
{
    if (color_name != nullptr) {
        std::string uname = stoupper(color_name);
        if (BrushesByColorLetter.count(uname) != 0)
            Brush = *BrushesByColorLetter[uname];
        else
            Brush = GKOffBrush;
    }
    else {
        if (RandomBrushes.count(color_ref) == 0)
            RandomBrushes[color_ref] = CreateSBrush(color_ref);
        Brush = RandomBrushes[color_ref];
    }
    
}


// This ancient mechanism is actually superior to the entire circus of C++11 Move-constructors etc...
void PanelLightAspect::ProcessLoadComplete() {
    ReporterRelay = CreateAndSetReporter (PLight->XlkgNo, relay_type_name.c_str(), PanelLightAspect::Reporter, this);
}

void PanelLight::ProcessLoadComplete() {
    for (auto& aspect: Aspects)
        aspect.ProcessLoadComplete();
}
