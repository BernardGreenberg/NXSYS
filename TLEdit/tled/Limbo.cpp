//
//  Limbo.cpp
//  TLEdit
//
//  Created by Bernard Greenberg on 5/22/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#include <unordered_set>
#include "Limbo.h"

using GOptr = GraphicObject*;

std::unordered_set<GOptr> Limbo;

void ClearLimbo() {
    Limbo.clear();
}
void GraphicObject::ConsignToLimbo() {
    assert (Limbo.count(this) == 0);
    BeforeInterment();
    Limbo.insert(this);
}


GOptr ResurrectFromLimbo(GOptr g) {
    assert(Limbo.count(g));
    Limbo.erase(g);
    g->AfterResurrection();
    g->MakeSelfVisible();
    return g;
}
