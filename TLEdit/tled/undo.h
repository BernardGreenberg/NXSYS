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
#include <unordered_map>
#include <vector>
#include "ijid.h"

class TrackSeg;
using SegmentGroupMap = std::unordered_map<TrackSeg*, IJID>; // pretty ugly duplication

namespace Undo {

void RecordGOCreation(GraphicObject* g);
void RecordGOCut(GraphicObject* g);
void RecordGOMoveStart(GraphicObject* g);
void RecordGOMoveComplete(GraphicObject* g);
void RecordChangedProps(GraphicObject* g, PropCellBase* pcp);
void RecordChangedJointProps(TrackJoint* tj, PropCellBase* pcp);
void RecordIrreversibleAct(const char * description);
void RecordWildfireTCSpread(SegmentGroupMap& SGM, IJID new_tcid);
void RecordJointCreation(TrackJoint* tj, TrackSeg* splitee, TrackSeg* new_seg);
void RecordSegmentCut(TrackSeg* ts);
void RecordSegmentCreation(TrackSeg* ts);
void RecordShiftLayout(int deltax, int deltay);
void RecordSetViewOrigin(WPPOINT old, WPPOINT nieuw);
struct JointCutSnapInfo* SnapshotJointPreCut(TrackJoint* tj);
void RecordJointCutComplete(struct JointCutSnapInfo* jcsip);
void RecordJointMoveComplete(TrackJoint* tj);
void RecordJointMerge(TrackJoint* consumer, TrackJoint* movee, std::vector<TrackJoint*>& opposing_joints);

bool IsUndoPossible();
bool IsRedoPossible();
void Undo();
void Redo();

}

#endif

