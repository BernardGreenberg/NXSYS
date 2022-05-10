//
//  undo.h
//  TLEdit
//
//  Created by Bernard Greenberg on 5/10/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#pragma once

#include "nxgo.h"

namespace Undo {

void RecordGOCreation(GraphicObject* g);
void RecordGODeletion(GraphicObject* g);
bool IsUndoPossible();
bool IsRedoPossible();
void Undo();
void Redo();

}

