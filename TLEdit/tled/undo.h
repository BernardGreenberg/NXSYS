//
//  undo.h
//  TLEdit
//
//  Created by Bernard Greenberg on 5/10/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#pragma once

#if TLEDIT

#include "nxgo.h"

#include "PropCell.h"
#include <unordered_set>

class TrackSeg;
using TSSet = std::unordered_set<TrackSeg *>;

namespace Undo {

void RecordGOCreation(GraphicObject* g);
void RecordGOCut(GraphicObject* g);
void RecordGOMoveStart(GraphicObject* g);
void RecordGOMoveComplete(GraphicObject* g);
void RecordChangedProps(GraphicObject* g, PropCellBase* pcp);
void RecordIrreversibleAct(const char * description);
void RecordWildfireTCSpread(TSSet&, int old_tcid, int new_tcid);
void RecordJointCreation(TrackJoint* tj, WPPOINT seg_id);
void RecordSegmentCut(TrackSeg* ts);
void RecordSegmentCreation(TrackSeg* ts);
void RecordShiftLayout(int deltax, int deltay);
void RecordViewportOrigin(WP_cord x, WP_cord y);

bool IsUndoPossible();
bool IsRedoPossible();
void Undo();
void Redo();

}

#endif

