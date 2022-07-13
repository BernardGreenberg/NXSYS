//
//  Limbo.cpp
//  TLEdit
//
//  Created by Bernard Greenberg on 5/22/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#include <unordered_set>
#include <cassert>
#include "nxgo.h"
#include "xtgtrack.h"
#include "Limbo.h"
#include "objreg.h"


using GOptr = GraphicObject*;

std::unordered_set<GOptr> Limbo;

void ClearLimbo() {
    for (auto g : Limbo)
        g->PurgeFromLimbo();
    Limbo.clear();
}

void GraphicObject::PurgeFromLimbo() {
    delete this;
}

void TrackSeg::PurgeFromLimbo() {
    Ends[0].Joint = nullptr;  // The joints are going to get purged independently
    Ends[1].Joint = nullptr;
    GraphicObject::PurgeFromLimbo();
}
#if DEBUG
static const char * nxtypename(GOptr g) {
    return NXObjectTypeName(g->TypeID());
}
#endif

void GraphicObject::ConsignToLimbo() {
#if DEBUG
    printf ("Consign   %s %p\n", nxtypename(this), this);
#endif
    assert (Limbo.count(this) == 0 && "Object already in Limbo");
    assert (TypeID() == TypeId::TRACKSEG || TypeID() == TypeId::JOINT);
    BeforeInterment();
    Limbo.insert(this);
}


GOptr ResurrectFromLimbo(GOptr g, TypeId expected_type) {
    assert(g && "Null passed into Resurrect");
#if DEBUG
    printf ("Resurrect %s %p\n", nxtypename(g), g);
#endif
    assert(Limbo.count(g) && "Object not found in Limbo");
    assert(g->TypeID() == expected_type);
    Limbo.erase(g);
    g->AfterResurrection();
    g->MakeSelfVisible();
    return g;
}

void TrackJoint::BeforeInterment() {
    TSA[0] = TSA[1] = TSA[2] = nullptr;
    TSCount = 0;
    GraphicObject::BeforeInterment();
    if (Lab)
        Lab->BeforeInterment();
}

void TrackJoint::AfterResurrection() {
    GraphicObject::AfterResurrection();
    if (Lab) {
        Lab->AfterResurrection();
        Lab->MakeSelfVisible();
    }
}
