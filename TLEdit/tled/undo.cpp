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
#include "LayoutModified.h"
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
    CutJoint, ShiftLayout, SetViewOrigin, IrreversibleAct, Wildfire
};

static std::unordered_map<RecType, string> RecTypeNames {
    {RecType::CreateGO, "create"},
    {RecType::CutGO, "cut"},
    {RecType::PropChange, "property change"},
    {RecType::MoveGO, "move"},
    {RecType::CreateJoint, "create"},
    {RecType::CreateSegment, "create"},
    {RecType::CutSegment, "cut"},
    {RecType::CutJoint, "cut"},
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
    WildfireRecord (const WPVEC& wpvec, IJID oldtc, IJID newtc) :
    Segvec(wpvec), old_tcid(oldtc), new_tcid(newtc) {};

    WPVEC Segvec;
    IJID old_tcid;
    IJID new_tcid;
};


struct Coords {
    Coords() {}
    Coords(WP_cord x, WP_cord y) : wp_x(x), wp_y(y) {}
    Coords(WPPOINT wp) : Coords(wp.x, wp.y) {}
    Coords(GOptr g) : Coords(g->WPPoint()) {}
    operator WPPOINT const () {return WPPOINT(wp_x, wp_y);}
    Coords operator - () const {
        return Coords(-wp_x, -wp_y);
    }

    bool operator == (const Coords& other) const {
        return wp_x == other.wp_x && wp_y == other.wp_y;
    }
    bool operator != (const Coords& other) const {
        return !(*this == other);
    }
    WP_cord wp_x, wp_y;
};

static GOptr FindObjByLoc(TypeId type, const Coords& wp, bool nf_ok =false);

struct UndoRecord {
    /* The theory of this is that we can't store object pointers in the stacks,
     because "equivalent" objects are going to be created and destroyed. Thus, the
     means of identifying an object is the pair of its location and type. For all
     objects except track segs, that's well-defined. For track-segs, it's their midpoint.
     After a lot of thinking about the fact that no forward actions escape the stack,
     you will conclude that these locations remain valid if you ever reach them by
     undoing (the only way), no matter what else has been done. */
    
    /* That works like a charm, except when IJ's in the middle of a double-crossover coincide. */

    UndoRecord() {}
    UndoRecord(RecType rtype) : rec_type(rtype) {}
   
    /* ye nieuww movector ... */
    UndoRecord(UndoRecord&& other) {
        rec_type = other.rec_type;
        recreate_form = other.recreate_form;
        obj_type = other.obj_type;
        coords = other.coords;
        coords_old = other.coords_old;
        seg_id = other.seg_id;
        Nomenclature = other.Nomenclature;

        orig_props = std::move(other.orig_props);
        changed_props = std::move(other.changed_props);
        wf_objptr = std::move(other.wf_objptr);
    }

    /* POD (plain old data) */
    RecType rec_type;
    string recreate_form; /* Lisp form to recreate */
    TypeId obj_type = TypeId::NONE;
    IJID Nomenclature = 0;   // only used sometimes.

    /* Little structures. */
    Coords coords {0,0};
    Coords coords_old {-1,-1};
    WPPOINT seg_id{0,0};

    /* Self-important pointers */
    std::unique_ptr<PropCellBase> orig_props;
    std::unique_ptr<PropCellBase> changed_props;
    std::unique_ptr<WildfireRecord> wf_objptr;

    void TypeCoords(GOptr g) {
        coords = Coords(g);
        obj_type = g->TypeID();
    }
    
    GOptr Find() {
        return FindObjByLoc(obj_type, coords);
    }
    GOptr FindOld() {
        return FindObjByLoc(obj_type, coords_old);
    }

    string DescribeAction(string tag) {
        if (rec_type == RecType::IrreversibleAct)
            return "Can't " + tag + ": " + recreate_form;
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

static GOptr FindObjByLoc(TypeId type, const Coords& C, bool nf_ok) {
    GOptr g = FindObjectByTypeAndWPpos(type, C.wp_x, C.wp_y);
    assert(g || nf_ok);
    return g;
}

static GOptr FindObjByLoc(TypeId type, const WPPOINT& wp) {
    return FindObjByLoc(type, Coords(wp));
}

static Coords GOMovCoords {0, 0};

static void MoveGO (GOptr g, const Coords& C) {
    g->MoveWP(C.wp_x, C.wp_y);
}

static void MarkForwardAction() {
    RedoStack.clear();
    compute_menu_state();
}
    

static void PlacemForward(UndoRecord& ur) {
    UndoStack.emplace_back(std::move(ur));
    MarkForwardAction();
}

void RecordGOCreation(GraphicObject* g){
    UndoRecord R (RecType::CreateGO);
    R.TypeCoords(g);
    R.recreate_form = StringImageObject(g);
    if (g->TypeID() == TypeId::JOINT)
        R.rec_type = RecType::CreateJoint;
    PlacemForward(R);
}

void RecordGOCut(GraphicObject* g) {
    UndoRecord R(RecType::CutGO);
    R.recreate_form = StringImageObject(g);
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
    R.coords_old =  pre_change_props->WpPoint();
    R.orig_props.reset(pre_change_props);
    R.changed_props.reset(pre_change_props->SnapshotNow(g));
    PlacemForward(R);
}

void RecordWildfireTCSpread(std::unordered_set<TrackSeg *>& segs,
                            IJID old_tcid, IJID new_tcid) {
    WPVEC seg_points;
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
    R.Nomenclature = tj->Nomenclature;
    PlacemForward(R);
}

void RecordSegmentCut (TrackSeg* ts) {
    UndoRecord R(RecType::CutSegment);
    R.obj_type = TypeId::TRACKSEG;
    R.coords_old = Coords(ts->Ends[0].Joint);
    R.coords     = Coords(ts->Ends[1].Joint);
    if (ts->Circuit)
        R.Nomenclature = ts->Circuit->StationNo;
    else
        R.Nomenclature = 0;
    PlacemForward(R);
}

void RecordSegmentCreation (TrackSeg* ts) {
    UndoRecord R(RecType::CreateSegment);
    R.obj_type = TypeId::TRACKSEG;
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
    R.recreate_form = description;
    PlacemForward(R);
}

void RecordSetViewOrigin(WPPOINT old, WPPOINT nieuw) {
    UndoRecord R(RecType::SetViewOrigin);
    R.coords = nieuw;
    R.coords_old = old;
    PlacemForward(R);
}

static void undo_guts (vector<UndoRecord>& Stack, RecType rt, UndoRecord& R) {
    switch(rt) {
        case RecType::CreateGO:
            delete R.Find();
            break;
            
        case RecType::CutGO:
        {
            GOptr obj = ProcessNonGraphObjectCreateFormString(R.recreate_form.c_str());
            obj->MakeSelfVisible();
            obj->Select();
            break;
        }

        case RecType::CreateJoint:
            ((TrackJoint*)R.Find())->Cut_();
            break;
            
        case RecType::CutJoint:
        {
            auto tj = (TrackJoint*)ProcessNonGraphObjectCreateFormString(R.recreate_form.c_str());
            Coords dest_loc(tj);
            auto seg = (TrackSeg*)FindObjByLoc(TypeId::TRACKSEG, R.coords_old);
            seg->Split(R.coords_old.wp_x, R.coords_old.wp_y, tj);
            tj->MoveToNewWPpos(dest_loc.wp_x, dest_loc.wp_y);
            break;
        }
            
        case RecType::MoveGO:
        {
            GOptr g = R.Find();
            auto [x,y] = R.coords_old;
            if (R.obj_type == TypeId::JOINT)
                ((TrackJoint*)g)->MoveToNewWPpos(x, y);
            else
                MoveGO(g, R.coords_old);
            break;
        }
            
        case RecType::PropChange:
        {
            StatusMessage("");  //clear left-over object descriptions.
            GOptr g = R.Find();
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
                tj1 = new TrackJoint(R.coords);
            if (tj2 == nullptr)
                tj2 = new TrackJoint(R.coords_old);
            auto seg = new TrackSeg(R.coords, R.coords_old);
            seg->SetTrackCircuit(R.Nomenclature);
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
            AssignFixOrigin(R.coords_old);
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
                    R.recreate_form.c_str());
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
            GOptr g = ProcessNonGraphObjectCreateFormString(R.recreate_form.c_str());
            g->MakeSelfVisible();
            g->Select();
            break;
        }

        case RecType::CreateJoint:
        {
            auto seg = (TrackSeg*) FindObjByLoc(TypeId::TRACKSEG, R.seg_id);
            auto tj = new TrackJoint(R.coords);
            seg->Split(R.coords.wp_x, R.coords.wp_y, tj);
            tj->Select();
            break;
        }
        
        case RecType::CutJoint:
            ((TrackJoint*)R.Find())->Cut_();
             break;

        case RecType::CutGO:
            delete R.Find();
            break;
            
        case RecType::MoveGO:
        {
            GOptr g = R.FindOld();
            auto [x, y] = R.coords;
            if (R.obj_type == TypeId::JOINT)
                ((TrackJoint*)g)->MoveToNewWPpos(x, y);
            else
                MoveGO(g, R.coords);
            break;
        }
            
        case RecType::PropChange:
        {
            GOptr g = R.FindOld();
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
            AssignFixOrigin(R.coords);
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
    Coords OthersLoc[2];
    Coords OriginalLoc;
    Coords OthersAverage() {
        return Coords((OthersLoc[0].wp_x + OthersLoc[1].wp_x)/2, (OthersLoc[0].wp_y + OthersLoc[1].wp_y)/2);
    }
};


/* Joint cut system very hairy */
JointCutSnapInfo* SnapshotJointPreCut(TrackJoint* tj) {
    auto J = new JointCutSnapInfo;
    J->RecreateInfo = StringImageObject(tj);
    J->OriginalLoc = Coords(tj);
    TSEX myx0 = tj->TSA[0]->FindEndIndex(tj);
    assert (myx0 != TSEX::NOTFOUND);
    J->OthersLoc[0] = Coords(tj->TSA[0]->GetOtherEnd(myx0).Joint);
    if (tj->TSCount == 1)
        J->OthersLoc[1] = J->OthersLoc[0];
    else {
        TSEX myx1 = tj->TSA[1]->FindEndIndex(tj);
        assert (myx1 != TSEX::NOTFOUND);
        J->OthersLoc[1] = Coords(tj->TSA[1]->GetOtherEnd(myx1).Joint);
    }
    return J;
}

void RecordJointCutComplete(JointCutSnapInfo* J) {
    UndoRecord R(RecType::CutJoint);
    R.obj_type = TypeId::JOINT;
    R.recreate_form = J->RecreateInfo;
    R.coords = J->OriginalLoc;
    R.coords_old = J->OthersAverage();
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
/* Global functions */
void ClearLayoutModified() {
    Undo::RedoStack.clear();
    Undo::UndoStack.clear();
}

bool IsLayoutModified() {
    return !Undo::UndoStack.empty();
}

