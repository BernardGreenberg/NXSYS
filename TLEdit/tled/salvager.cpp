//
//  salvager.cpp
//  TLEdit
//
//  Created by Bernard Greenberg on 3/18/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#include <unordered_set>
#include <string>
#include <cstdarg>

using std::string, std::unordered_set, std::vector;

#include "STLExtensions.h"

#include "salvager.hpp"
#include "xtgtrack.h"
#include "tledit.h"
#include "nxgo.h"
#include "typeid.h"



/* Recordamos de Multics ...*/


struct SalvInstance {
    string Message;
    unordered_set<GraphicObject*> AllObjects;
    SalvInstance(const char * msg) : Message(msg) {}
    void SalvageSeg(TrackSeg* ts);
    void SalvageJoint(TrackJoint* tj);
    void Salvage();
    void Error (TrackJoint* tj, const char* ctlstring, ...);
    void Error(TrackSeg* ts, const char * ctlstring, ...);
    void ErrorCommon(const string& description, const char* ctlstring, va_list va);
};

static string DescribeJoint (TrackJoint* tj) {
    return FormatString("Joint %d (AB0 %d) at %ldx%ld (%p)",
                        (int)tj->Nomenclature, tj->SwitchAB0, tj->wp_x, tj->wp_y, tj);
}

static string DescribeSegment (TrackSeg* ts) {
    return FormatString("Segment at %ldx%ld  %ldx%ld (%p)",
                        ts->Ends[0].wpx, ts->Ends[0].wpy, ts->Ends[1].wpx, ts->Ends[1].wpy, ts);
}

void SalvInstance::Error(TrackJoint* tj, const char * ctlstring, ...) {
    va_list va;
    va_start(va, ctlstring);
    ErrorCommon(DescribeJoint(tj), ctlstring, va);
    va_end(va);
}

void SalvInstance::Error(TrackSeg* ts, const char * ctlstring, ...) {
    va_list va;
    va_start(va, ctlstring);
    ErrorCommon(DescribeSegment(ts), ctlstring, va);
    va_end(va);
}
    
void SalvInstance::ErrorCommon(const string& description, const char* ctlstring, va_list va) {
    string message = "Error at " + Message + " in " + description + " " + FormatStringVA(ctlstring, va);
    MessageBox(nullptr, message.c_str(), "Structure Salvager Diagnostic", MB_OK);
#ifdef DEBUG
    (void)ctlstring;
#endif
}


void Salvager(const char* message) {
    SalvInstance(message).Salvage();
}

void SalvInstance::Salvage() {
    MapAllGraphicObjects([](GraphicObject* g, void* thus) {
        SalvInstance& SI = *(SalvInstance*) thus;
        SI.AllObjects.insert(g);
        return 0;
    }, this);
    
    MapFindGraphicObjectsOfType(TypeId::JOINT, [](GraphicObject *g, void* thus) {
        SalvInstance& SI = *(SalvInstance*) thus;
        SI.SalvageJoint((TrackJoint*)g);
        return 0;
    }, this);
    
    MapFindGraphicObjectsOfType(TypeId::TRACKSEG, [](GraphicObject *g, void* thus) {
        SalvInstance& SI = *(SalvInstance*) thus;
        SI.SalvageSeg((TrackSeg*)g);
        return 0;
    }, this);
}

void SalvInstance::SalvageJoint(TrackJoint* tj) {
    if (tj->Nomenclature < 0 || tj->Nomenclature > 40000)
        return Error(tj, "Bogus Nomenclature, likely garbage pointer");

    if (tj->TSCount < 0 || tj->TSCount > 3)
        return Error(tj, "Bogus branch count: %d", tj->TSCount);

    if (tj->TSCount == 3 && (tj->SwitchAB0 < 0 || tj->SwitchAB0 > 2))
        return Error(tj, "Bogus SwitchAB0 %d", tj->TSCount);

    for (int brx = 0; brx < tj->TSCount; brx ++) {
        TrackSeg* ts = tj->TSA[brx];
        if (ts == NULL)
            Error(tj, "Claims %d branches, but TSA[%d] is null.", tj->TSCount, brx);
        else {
            if (AllObjects.count((GraphicObject*)ts) == 0) {
                Error(ts, "found in joint %s at %d, not in global listing.",
                      DescribeJoint(tj).c_str(), brx);
            }
            else if (tj == ts->Ends[0].Joint) {
                continue;
            }
            else if (tj == ts->Ends[1].Joint) {
                continue;
            } else {
                Error(ts, "found in [%s] at %d, is not Ends.Joint of either of its ends.",
                      DescribeJoint(tj).c_str(), brx);
            }

        }
    }
}

void SalvInstance::SalvageSeg(TrackSeg* ts) {
    for (int ex = 0; ex < 2; ex++) {
        TrackSegEnd& E = ts->Ends[ex];
        if (E.Joint == nullptr)
            Error(ts, "Null pointer at Ends[%d].Joint", ex);
        else {
            TrackJoint& J = *E.Joint;
            if (J.TSCount < 0 || J.TSCount > 3 || J.Nomenclature < 0 || J.Nomenclature > 40000)
                Error(ts, "Probable garbage pointer at Ends[%d].Joint", ex);
            else
                if (AllObjects.count((GraphicObject*)(&J)) == 0) {
                    Error(ts, "Joint ptr %s in End[%d] not in global object table.",
                          DescribeJoint(&J).c_str(), ex);
                } else {
                    bool found = false;
                    for (int i = 0; i < J.TSCount; i++) {
                        if (J.TSA[i] == ts)
                            found = true;
                    }
                    if (!found)
                        Error (ts, "has %s as [%d], which does not know it as a branch.",   DescribeJoint(&J).c_str(), ex);
                }
        }
        if (ts->Ends[0].Joint == ts->Ends[1].Joint) {
            Error(ts, "has %s as both its ends.", DescribeJoint(ts->Ends[0].Joint).c_str());
        }
    }
}


