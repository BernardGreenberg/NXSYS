//
//  Limbo.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 5/22/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#ifndef Limbo_h
#define Limbo_h

#include "nxgo.h"

void ConsignToLimbo(GraphicObject* g);
GraphicObject* ResurrectFromLimbo(GraphicObject * g, TypeId expected_type);
void ClearLimbo();

#endif /* Limbo_h */
