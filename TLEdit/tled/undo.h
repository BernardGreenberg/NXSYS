//
//  undo.h
//  TLEdit
//
//  Created by Bernard Greenberg on 5/10/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#pragma once

#include "nxgo.h"

#include "PropCell.h"

namespace Undo {

void RecordGOCreation(GraphicObject* g);
void RecordGOCut(GraphicObject* g);
void RecordGOMoveStart(GraphicObject* g);
void RecordGOMoveComplete(GraphicObject* g);
void RecordChangedProps(GraphicObject* g, PropCellBase* pcp);

bool IsUndoPossible();
bool IsRedoPossible();
void Undo();
void Redo();

}

