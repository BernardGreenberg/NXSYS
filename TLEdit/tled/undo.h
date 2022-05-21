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
#include <vector>
#include "ijid.h"

class TrackSeg;
using TSSet = std::unordered_set<TrackSeg *>;
using IJID = long;
namespace Undo {

void RecordGOCreation(GraphicObject* g);
void RecordGOCut(GraphicObject* g);
void RecordGOMoveStart(GraphicObject* g);
void RecordGOMoveComplete(GraphicObject* g);
void RecordChangedProps(GraphicObject* g, PropCellBase* pcp);
void RecordIrreversibleAct(const char * description);
void RecordWildfireTCSpread(TSSet&, IJID old_tcid, IJID new_tcid);
void RecordJointCreation(TrackJoint* tj, WPPOINT seg_id);
void RecordSegmentCut(TrackSeg* ts);
void RecordSegmentCreation(TrackSeg* ts);
void RecordShiftLayout(int deltax, int deltay);
void RecordSetViewOrigin(WPPOINT old, WPPOINT nieuw);
struct JointCutSnapInfo* SnapshotJointPreCut(TrackJoint* tj);
void RecordJointCutComplete(struct JointCutSnapInfo* jcsip);
void RecordJointMerge(TrackJoint* consumer, TrackJoint* movee, std::vector<TrackJoint*>& opposing_joints);

bool IsUndoPossible();
bool IsRedoPossible();
void Undo();
void Redo();

}

#endif

