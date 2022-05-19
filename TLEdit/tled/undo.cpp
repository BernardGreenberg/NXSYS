//
//  undo.cpp
//  TLEdit
//
//  Created by Bernard Greenberg on 5/10/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//
#include "typeid.h"
#include "nxgo.h"
#include "tledit.h"
#include "objreg.h"
#include "undo.h"
#include "xtgtrack.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cstdarg>
#include <cassert>
#include <memory>

using std::vector, std::string;
using GOptr = GraphicObject*;
using WPVEC = vector<WPPOINT>;
GOptr ProcessNonGraphObjectCreateFormString(const char * s);

void SetUndoRedoMenu(const char * undo, const char * redo);

namespace Undo {

enum class RecType {CreateGO, CutGO, MoveGO, PropChange, CreateSegment, CutSegment, CreateJoint,
    DeleteJoint, ShiftLayout, SetViewOrigin, IrreversibleAct, Wildfire
};

static std::unordered_map<RecType, string> RecTypeNames {
    {RecType::CreateGO, "create"},
    {RecType::CutGO, "cut"},
    {RecType::PropChange, "property change"},
    {RecType::MoveGO, "move"},
    {RecType::CreateJoint, "create"},
    {RecType::CreateSegment, "create"},
    {RecType::CutSegment, "cut"},
    {RecType::Wildfire, "wildfire spread track circuit"},
    {RecType::ShiftLayout, "shift layout"},
    {RecType::SetViewOrigin, "set view origin"},
    {RecType::IrreversibleAct, "currently irreversible act"}
};

static std::unordered_set<RecType> OpLessOps {
    RecType::ShiftLayout,
    RecType::SetViewOrigin,
    RecType::IrreversibleAct,
    RecType::Wildfire};

PropCellBase::~PropCellBase () {
    //even though destructor is marked pure, this must be provided!!!
}

struct WildfireRecord {
    WildfireRecord (const WPVEC& wpvec, int oldtc, int newtc) :
    Segvec(wpvec), old_tcid(oldtc), new_tcid(newtc) {};

    WPVEC Segvec;
    int old_tcid;
    int new_tcid;
};


struct Coords {
    Coords() {}
    Coords(WP_cord x, WP_cord y) {
        wp_x = x;
        wp_y = y;
    }
    Coords(GOptr g) {
        WPPOINT wpp = g->WPPoint();
        wp_x = wpp.x;
        wp_y = wpp.y;
    }
    Coords(WPPOINT wp) {
        wp_x = wp.x;
        wp_y = wp.y;
    }
    bool operator == (const Coords& other) const {
        return wp_x == other.wp_x && wp_y == other.wp_y;
    }
    bool operator != (const Coords& other) const {
        return !(*this == other);
    }
    WP_cord wp_x, wp_y;
};

struct UndoRecord {
    UndoRecord() {}
    UndoRecord(RecType rtype) : rec_type(rtype) {}
   
    /* ye nieuww movector ... */
    UndoRecord(UndoRecord&& other) {
        rec_type = other.rec_type;
        image = other.image;
        obj_type = other.obj_type;
        coords = other.coords;
        coords_old = other.coords_old;
        seg_id = other.seg_id;

        orig_props = std::move(other.orig_props);
        changed_props = std::move(other.changed_props);
        wf_objptr = std::move(other.wf_objptr);
    }

    RecType rec_type;
    string image; /* generally Lisp form to recreate */
    TypeId obj_type = TypeId::NONE;
    
    Coords coords {0,0};
    Coords coords_old {-1,-1};
    WPPOINT seg_id{0,0};

    std::unique_ptr<PropCellBase> orig_props;
    std::unique_ptr<PropCellBase> changed_props;
    std::unique_ptr<WildfireRecord> wf_objptr;

    void TypeCoords(GOptr g) {
        coords = Coords(g);
        obj_type = g->TypeID();
    }

    string DescribeAction(string tag) {
        if (rec_type == RecType::IrreversibleAct)
            return "Can't " + tag + ": " + image;
        else if (OpLessOps.count(rec_type))
            return tag + " " + RecTypeNames[rec_type];
        else
            return tag + " " + RecTypeNames[rec_type] + " " + NXObjectTypeName(obj_type);
    }

};

static vector <UndoRecord> UndoStack;
static vector <UndoRecord> RedoStack;

class StringOutputWriter : public GraphicObject::ObjectWriter {
    string S;
public:
    void putc (char c) {S += c;}
    void puts (const char * s) {S += s;}
    void putf (const char * ctl, ...) {
        va_list args1, args2;
        va_start(args1, ctl);
        vector<char> result(1+vsnprintf(nullptr, 0, ctl, args1));
        va_end(args1);
        va_start(args2, ctl);
        vsnprintf(result.data(), result.size(), ctl, args2);
        va_end(args2);
        S += string(result.data(), result.size()-1);
    }
    string get() {return S;}
};

bool IsUndoPossible() {
    return !UndoStack.empty();
}

bool IsRedoPossible() {
    return !RedoStack.empty();
}

static void compute_menu_state() {
    static string undo_str, redo_str;
    undo_str = redo_str = "";
    const char * undo_avl = nullptr;
    const char * redo_avl = nullptr;
    if (IsUndoPossible()) {
        undo_str = UndoStack.back().DescribeAction("Undo");
        undo_avl = undo_str.c_str();
    }
    if (IsRedoPossible()) {
        redo_str = RedoStack.back().DescribeAction("Redo");
        redo_avl = redo_str.c_str();
    }
    SetUndoRedoMenu(undo_avl, redo_avl);
}

static string StringImageObject(GOptr o) {
    StringOutputWriter W;
    o->Dump(W);
    return W.get();
}

static GOptr FindObjByLoc(TypeId type, const Coords& C, bool nf_ok = false) {
    GOptr g = FindObjectByTypeAndWPpos(type, C.wp_x, C.wp_y);
    assert(g || nf_ok);
    return g;
}

static GOptr FindObjByLoc(TypeId type, const WPPOINT& wp) {
    return FindObjByLoc(type, Coords(wp.x, wp.y));
}

static Coords GOMovCoords {0, 0};

static void MoveGO (GOptr g, const Coords& C) {
    g->MoveWP(C.wp_x, C.wp_y);
}

static void MarkForwardAction() {
    RedoStack.clear();
    compute_menu_state();
    BufferModified = TRUE; /* when this sys is complete, won't need BufferModified*/
}
    
static void PlacemForward(UndoRecord& ur) {
    UndoStack.emplace_back(std::move(ur));
    MarkForwardAction();
}

void RecordGOCreation(GraphicObject* g){
    UndoRecord R (RecType::CreateGO);
    R.TypeCoords(g);
    R.image = StringImageObject(g);
    if (g->TypeID() == TypeId::JOINT)
        R.rec_type = RecType::CreateJoint;
    PlacemForward(R);
}

void RecordGOCut(GraphicObject* g) {
    UndoRecord R(RecType::CutGO);
    R.image = StringImageObject(g);
    R.TypeCoords(g);
    PlacemForward(R);
}

void RecordGOMoveStart(GraphicObject* g) {
    GOMovCoords = Coords(g);
}

void RecordGOMoveComplete(GraphicObject* g) {
    Coords uploc(g);
    if (uploc != GOMovCoords) {
        UndoRecord R(RecType::MoveGO);
        R.coords_old = GOMovCoords;
        R.TypeCoords(g);
        PlacemForward(R);
    }
}

void RecordChangedProps(GraphicObject* g, PropCellBase* pre_change_props) {
    UndoRecord R(RecType::PropChange);
    R.TypeCoords(g);
    R.orig_props.reset(pre_change_props);
    R.changed_props.reset(pre_change_props->SnapshotNow(g));
    PlacemForward(R);
}

void RecordWildfireTCSpread(std::unordered_set<TrackSeg *>& segs,
                            int old_tcid, int new_tcid) {
    vector<WPPOINT>seg_points;
    for (auto seg : segs)
        seg_points.push_back(seg->WPPoint());
    UndoRecord R(RecType::Wildfire);
    R.wf_objptr.reset(new WildfireRecord(seg_points, old_tcid, new_tcid));
    R.obj_type = TypeId::NONE;
    PlacemForward(R);
}

void RecordJointCreation(TrackJoint* tj, WPPOINT seg_id) {
    UndoRecord R(RecType::CreateJoint);
    R.TypeCoords(tj);
    R.seg_id = seg_id;
    PlacemForward(R);
}

void RecordSegmentCut (TrackSeg* ts) {
    UndoRecord R(RecType::CutSegment);
    R.coords_old = Coords(ts->Ends[0].Joint);
    R.coords     = Coords(ts->Ends[1].Joint);
    PlacemForward(R);
}

void RecordSegmentCreation (TrackSeg* ts) {
    UndoRecord R(RecType::CreateSegment);
    R.coords_old = Coords(ts->Ends[0].Joint);
    R.coords     = Coords(ts->Ends[1].Joint);
    PlacemForward(R);
}

void RecordShiftLayout (int delta_x, int delta_y) {
    UndoRecord R(RecType::ShiftLayout);
    R.coords = Coords(delta_x, delta_y);
    PlacemForward(R);
}

void RecordIrreversibleAct(const char * description) {
    UndoRecord R(RecType::IrreversibleAct);
    R.image = description;
    PlacemForward(R);
}

void RecordSetViewOrigin(WPPOINT old, WPPOINT nieuw) {
    UndoRecord R(RecType::SetViewOrigin);
    R.coords = Coords(nieuw.x, nieuw.y);
    R.coords_old = Coords(old.x, old.y);
    PlacemForward(R);
}

static void undo_guts (vector<UndoRecord>& Stack, RecType rt, UndoRecord& R) {
    switch(rt) {
        case RecType::CreateGO:
            /* Can't store real object pointer in undo stack -- the object can be deleted and recreated
             (but at the same position) before the undo element is used. */

            delete FindObjByLoc(R.obj_type, R.coords);
            break;
            
        case RecType::CutGO:
        {
            GOptr obj = ProcessNonGraphObjectCreateFormString(R.image.c_str());
            obj->MakeSelfVisible();
            obj->Select();
            break;
        }

        case RecType::CreateJoint:
            ((TrackJoint*)FindObjByLoc(R.obj_type, R.coords))->Cut();
            break;
            
        case RecType::DeleteJoint:
        {
            auto tj = (TrackJoint*)ProcessNonGraphObjectCreateFormString(R.image.c_str());
            Coords dest_loc(tj);
            int tscount = (R.coords == R.coords_old) ? 1 : 2;
            if (tscount == 2) {
                Coords midpoint((R.coords.wp_x + R.coords_old.wp_x)/2, (R.coords.wp_y + R.coords_old.wp_y)/2);
                auto seg = (TrackSeg*)FindObjByLoc(TypeId::TRACKSEG, midpoint);
                seg->Split(midpoint.wp_x, midpoint.wp_y, tj);
                
            }
            tj->MoveToNewWPpos(dest_loc.wp_x, dest_loc.wp_y);
            break;
        }
            
        case RecType::MoveGO:
        {
            GOptr g = FindObjByLoc(R.obj_type, R.coords);
            auto [x,y] = R.coords_old;
            if (R.obj_type == TypeId::JOINT)
                ((TrackJoint*)g)->MoveToNewWPpos(x, y);
            else
                MoveGO(g, R.coords_old);
            break;
        }
            
        case RecType::PropChange:
        {
            GOptr g = FindObjByLoc(R.obj_type, R.coords);
            R.orig_props->Restore(g);
            g->Invalidate();
            break;
        }

        case RecType::Wildfire:
            for (auto [x,y] : R.wf_objptr->Segvec) {
                auto seg = static_cast<TrackSeg*>(FindObjectByTypeAndWPpos(TypeId::TRACKSEG, x, y));
                assert(seg);
                seg->SetTrackCircuit(R.wf_objptr->old_tcid);
            }
            StatusMessage("");  //Clear out remains of wildfire's message
            break;
            
        case RecType::CutSegment:
        {
            auto tj1 = (TrackJoint*)FindObjByLoc(TypeId::JOINT, R.coords, true);
            auto tj2 = (TrackJoint*)FindObjByLoc(TypeId::JOINT, R.coords_old, true);
            if (tj1 == nullptr)
                tj1 = new TrackJoint(R.coords.wp_x, R.coords.wp_y);
            if (tj2 == nullptr)
                tj2 = new TrackJoint(R.coords_old.wp_x, R.coords_old.wp_y);
            auto seg = new TrackSeg(R.coords.wp_x, R.coords.wp_y, R.coords_old.wp_x, R.coords_old.wp_y);
            tj1->AddBranch(seg);
            tj2->AddBranch(seg);
            seg->Ends[0].Joint = tj1;
            seg->Ends[1].Joint = tj2;
            seg->Select();
            break;
        }
            
        case RecType::CreateSegment:
        {
            auto tj1 = (TrackJoint*)FindObjByLoc(TypeId::JOINT, R.coords);
            auto tj2 = (TrackJoint*)FindObjByLoc(TypeId::JOINT, R.coords_old);
            auto [x1, y1] = tj1->WPPoint();
            auto [x2, y2] = tj2->WPPoint();
            auto seg = (TrackSeg*)FindObjByLoc(TypeId::TRACKSEG,
                                    Coords((x1 + x2)/2, (y1 + y2)/2));
            tj1->Select(); // tj1 may vanish!
            seg->Cut_();
            break;
        }
            
        case RecType::ShiftLayout:
            ShiftLayout_(-(int)R.coords.wp_x, -(int)R.coords.wp_y);
            break;
            
        case RecType::SetViewOrigin:
            AssignFixOrigin(WPPOINT((int)R.coords_old.wp_x, (int)R.coords_old.wp_y));
            break;

        default:
            break;
    }
    Stack.emplace_back(std::move(R));
}

void Undo() {
    if (!IsUndoPossible())
        return;
    UndoRecord& R = UndoStack.back();
    RecType rt = R.rec_type;
    switch(rt) {
        case RecType::IrreversibleAct:
        {
            usererr("Currently irreversible act (%s) may not be undone, nor any earlier acts.",
                    R.image.c_str());
            return;  // Don't pop
        }
        default:
            undo_guts(RedoStack, rt, R);
    }
    UndoStack.pop_back();
    compute_menu_state();
}

void Redo() {
    if (!IsRedoPossible())
        return;
    UndoRecord& R = RedoStack.back();
    RecType rt = R.rec_type;
    bool already_emplaced = false;
    switch(rt) {
        case RecType::CreateGO:
        {
            GOptr g = ProcessNonGraphObjectCreateFormString(R.image.c_str());
            g->MakeSelfVisible();
            g->Select();
            break;
        }

        case RecType::CreateJoint:
        {
            auto seg = (TrackSeg*) FindObjByLoc(TypeId::TRACKSEG, R.seg_id);
            auto [x, y] = R.coords;
            auto tj = new TrackJoint(x, y);
            seg->Split(x, y, tj);
            tj->Select();
            break;
        }

        case RecType::CutGO:
            delete FindObjByLoc(R.obj_type, R.coords);
            break;
            
        case RecType::MoveGO:
        {
            GOptr g = FindObjByLoc(R.obj_type, R.coords);
            auto [x, y] = R.coords_old;
            if (R.obj_type == TypeId::JOINT)
                ((TrackJoint*)g)->MoveToNewWPpos(x, y);
            else
                MoveGO(g, R.coords_old);
            break;
        }
            
        case RecType::PropChange:
        {
            GOptr g = FindObjByLoc(R.obj_type, R.coords);
            R.changed_props->Restore(g);
            g->Invalidate();
            break;
        }

        case RecType::Wildfire:
            for (auto [x, y] : R.wf_objptr->Segvec) {
                auto seg = static_cast<TrackSeg*>(FindObjectByTypeAndWPpos(TypeId::TRACKSEG, x, y));
                assert(seg);
                seg->SetTrackCircuit(R.wf_objptr->new_tcid);
            }
            break;
            
        case RecType::ShiftLayout:
            ShiftLayout_((int)R.coords.wp_x, (int)R.coords.wp_y);
            break;
            
        case RecType::CutSegment:
            undo_guts(UndoStack, RecType::CreateSegment, R);
            already_emplaced = true;
            break;
            
        case RecType::CreateSegment:
            undo_guts(UndoStack, RecType::CutSegment, R);
            already_emplaced = true;
            break;

        case RecType::SetViewOrigin:
            AssignFixOrigin(WPPOINT((int)R.coords.wp_x, (int)R.coords.wp_y));
            break;
    
        default:
            break;
    }
    if (!already_emplaced)
        UndoStack.emplace_back(std::move(R));
    RedoStack.pop_back();
    compute_menu_state();
}

struct JointCutSnapInfo {
    string RecreateInfo;
    int TSCount;
    Coords OthersLoc[2];
};


/* Joint cut system very hairy */
JointCutSnapInfo* SnapshotJointPreCut(TrackJoint* tj) {
    auto J = new JointCutSnapInfo;
    J->RecreateInfo = StringImageObject(tj);
    J->TSCount = tj->TSCount;
    TSEX myx0 = tj->TSA[0]->FindEndIndex(tj);
    assert (myx0 != TSEX::NOTFOUND);
    TrackJoint * tj_other_0 = tj->TSA[0]->GetOtherEnd(myx0).Joint;
    J->OthersLoc[0] = Coords(tj_other_0);
    if (tj->TSCount == 1)
        J->OthersLoc[1] = J->OthersLoc[0];
    else {
        TSEX myx1 = tj->TSA[1]->FindEndIndex(tj);
        assert (myx1 != TSEX::NOTFOUND);
        TrackJoint * tj_other_1 = tj->TSA[1]->GetOtherEnd(myx1).Joint;
        J->OthersLoc[1] = Coords(tj_other_1);
    }
    return J;
}

void RecordJointCutComplete(JointCutSnapInfo* J) {
    UndoRecord R(RecType::DeleteJoint);
    R.image = J->RecreateInfo;
    R.obj_type = TypeId::JOINT;
    R.coords_old = J->OthersLoc[0];
    if (J->TSCount == 1)
        R.coords = R.coords_old;
    else
        R.coords = J->OthersLoc[1];
    delete J;
    PlacemForward(R);
}




}


int TrackJoint::Dump(ObjectWriter &W) {
    W.puts("(JOINT ");
    W.putf("%6ld      %4d %4d", Nomenclature, wp_x, wp_y);
    if (Insulated)
        W.puts(" INSULATED");
    if (NumFlip)
        W.puts(" NUMFLIP");
    W.putc(')');
    
    return -1;  /* sort prio not meaningful in undo-sys call */
}


