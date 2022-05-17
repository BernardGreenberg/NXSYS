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
#include <cstdarg>
#include <cassert>
#include <memory>

using std::vector, std::string;
using GOptr = GraphicObject*;
using WPVEC = vector<WPPOINT>;
GOptr ProcessNonGraphObjectCreateFormString(const char * s);

void SetUndoRedoMenu(const char * undo, const char * redo);

namespace Undo {

enum class RecType {CreateGO, CutGO, MoveGO, PropChange, CreateSegment, CutSegment, CreateJoint, DeleteJoint,
    IrreversibleAct, Wildfire
};

std::unordered_map<RecType, string> RecTypeNames {
    {RecType::CreateGO, "create"},
    {RecType::CutGO, "cut"},
    {RecType::PropChange, "property change"},
    {RecType::MoveGO, "move"},
    {RecType::CreateJoint, "create"},
    {RecType::CreateSegment, "create"},
    {RecType::CutSegment, "cut"},
    {RecType::Wildfire, "wildfire spread track circuit"},
    {RecType::IrreversibleAct, "currently irreversible act"}
};

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
    Coords(WP_cord x, WP_cord y) {
        wp_x = x;
        wp_y = y;
    }
    Coords(GOptr g) {
        WPPOINT wpp = g->WPPoint();
        wp_x = wpp.x;
        wp_y = wpp.y;
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
    UndoRecord(RecType type, string img, GraphicObject* o) {
        rec_type = type;
        image = img;
        obj_type = o->TypeID();
        coords = Coords(o);
    }
    UndoRecord(RecType type, string img, TypeId obt) {
       rec_type = type;
       image = img;
       obj_type = obt;
    }
    UndoRecord(RecType type, GOptr g, Coords C) {
        coords_old = C;
        rec_type = type;
        obj_type = g->TypeID();
        coords = Coords(g);
    }
    UndoRecord(GOptr g, PropCellBase* pcp_orig, PropCellBase* pcp_changed) {
        coords = Coords(g);
        rec_type = RecType::PropChange;
        obj_type = g->TypeID();
        orig_props.reset(pcp_orig);
        changed_props.reset(pcp_changed);
    }
    UndoRecord(WildfireRecord* wfrp) {
        rec_type = RecType::Wildfire;
        wf_objptr.reset(wfrp);
        obj_type = TypeId::NONE;
    }
    UndoRecord(RecType type, TrackJoint* tj, WPPOINT a_seg_id) {
        rec_type =type;
        obj_type = tj->TypeID();
        coords = Coords(tj);
        seg_id = a_seg_id;
    }
    UndoRecord(RecType type, Coords C1, Coords C2) {
        assert(type == RecType::CreateSegment || type == RecType::CutSegment);
        rec_type = type;
        obj_type = TypeId::TRACKSEG;
        coords = C2;
        coords_old = C1;
    }

    RecType rec_type;
    string image;                    /* not valid or needed for create*/
    TypeId obj_type;
    
    Coords coords {0,0};
    Coords coords_old {-1,-1};
    std::unique_ptr<PropCellBase> orig_props;
    std::unique_ptr<PropCellBase> changed_props;
    std::unique_ptr<WildfireRecord> wf_objptr;
    WPPOINT seg_id{0,0};

    string DescribeAction(string tag) {
        if (rec_type == RecType::IrreversibleAct)
            return "Can't " + tag + ": " + image;
        else if (rec_type == RecType::Wildfire)
            return tag + " wildfire spread TC";
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

static void MoveGO (GOptr g, const Coords& C) {
    g->MoveWP(C.wp_x, C.wp_y);
}

static void MarkForwardAction() {
    RedoStack.clear();
    compute_menu_state();
    BufferModified = TRUE; /* when this sys is complete, won't need BufferModified*/
}
    
void RecordGOCreation(GraphicObject* g){
    UndoStack.emplace_back(RecType::CreateGO, "", g);
    MarkForwardAction();
}

void RecordGOCut(GraphicObject* g) {
    UndoStack.emplace_back(RecType::CutGO, StringImageObject(g), g->TypeID());
    MarkForwardAction();
}

static Coords GOMovCoords {0, 0};

void RecordGOMoveStart(GraphicObject* g) {
    GOMovCoords = Coords(g);
}

void RecordGOMoveComplete(GraphicObject* g) {
    Coords uploc(g);
    if (uploc != GOMovCoords) {
        UndoStack.emplace_back(RecType::MoveGO, g, GOMovCoords);
        MarkForwardAction();
    }
}

void RecordChangedProps(GraphicObject* g, PropCellBase* pre_change_props) {
    UndoStack.emplace_back(g, pre_change_props, pre_change_props->SnapshotNow(g));
    MarkForwardAction();
}

void RecordWildfireTCSpread(std::unordered_set<TrackSeg *>& segs,
                            int old_tcid, int new_tcid) {
    vector<WPPOINT>seg_points;
    for (auto seg : segs)
        seg_points.push_back(seg->WPPoint());
    UndoStack.emplace_back(new WildfireRecord(seg_points, old_tcid, new_tcid));
    MarkForwardAction();
}

void RecordJointCreation(TrackJoint* tj, WPPOINT seg_id) {
    UndoStack.emplace_back(RecType::CreateJoint, tj, seg_id);
    MarkForwardAction();
}

void RecordSegmentCut (TrackSeg* ts) {
    auto tj0 = ts->Ends[0].Joint;
    auto tj1 = ts->Ends[1].Joint;
    UndoStack.emplace_back(RecType::CutSegment, Coords(tj0), Coords(tj1));
    MarkForwardAction();
}

void RecordSegmentCreation (TrackSeg* ts) {
    auto tj0 = ts->Ends[0].Joint;
    auto tj1 = ts->Ends[1].Joint;
    UndoStack.emplace_back(RecType::CreateSegment, Coords(tj0), Coords(tj1));
    MarkForwardAction();
}

void RecordIrreversibleAct(const char * description) {
    UndoStack.emplace_back(RecType::IrreversibleAct, description, TypeId::NONE);
    MarkForwardAction();
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
            return;
        }
        case RecType::CreateGO:
        {
            /* Can't store real object pointer in undo stack -- the object can be deleted and recreated
             (but at the same position) before the undo element is used. */

            GOptr g = FindObjByLoc(R.obj_type, R.coords);
            RedoStack.emplace_back(rt, StringImageObject(g), R.obj_type);
            delete g;
            break;
        }
            
        case RecType::CutGO:
        {
            GOptr obj = ProcessNonGraphObjectCreateFormString(R.image.c_str());
            obj->MakeSelfVisible();
            obj->Select();
            RedoStack.emplace_back(rt, obj, Coords(obj));
            break;
        }

        case RecType::CreateJoint:
        {
            auto tj = (TrackJoint*)FindObjByLoc(R.obj_type, R.coords);
            RedoStack.emplace_back(R.rec_type, tj, R.seg_id);  //do this before obliterating
            tj->Cut_();
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
            RedoStack.emplace_back(rt, g, R.coords);
            break;
        }
            
        case RecType::PropChange:
        {
            GOptr g = FindObjByLoc(R.obj_type, R.coords);
            R.orig_props->Restore(g);
            RedoStack.emplace_back(g, R.orig_props.release(), R.changed_props.release());
            g->Invalidate();
            break;
        }

        case RecType::Wildfire:
        {
            int ct = 0;
            for (auto wp : R.wf_objptr->Segvec) {
                auto seg = static_cast<TrackSeg*>
                   (FindObjectByTypeAndWPpos(TypeId::TRACKSEG, wp.x, wp.y));
                assert(seg);
                if (ct++ == 0)
                    seg->SetTrackCircuit(R.wf_objptr->old_tcid, FALSE);
                else
                    seg->SetTrackCircuit(0, FALSE);
                seg->Invalidate();
            }
            RedoStack.emplace_back(R.wf_objptr.release());
            break;
        }
            
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
            RedoStack.emplace_back(R.rec_type, Coords(tj1), Coords(tj2));
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
            RedoStack.emplace_back(R.rec_type, R.coords, R.coords_old);
            tj1->Select(); // tj1 may vanish!
            seg->Cut_();
            break;
        }

        default:
            break;
            
    }
    UndoStack.pop_back();
    compute_menu_state();
}

void Redo() {
    if (!IsRedoPossible())
        return;
    UndoRecord& R = RedoStack.back();
    RecType rt = R.rec_type;
    switch(rt) {
        case RecType::CreateGO:
        {
            GOptr g = ProcessNonGraphObjectCreateFormString(R.image.c_str());
            g->MakeSelfVisible();
            g->Select();
            UndoStack.emplace_back(rt, "", g);
            break;
        }

        case RecType::CreateJoint:
        {
            auto seg = (TrackSeg*) FindObjByLoc(TypeId::TRACKSEG, R.seg_id);
            auto [x, y] = R.coords;
            auto tj = new TrackJoint(x, y);
            seg->Split(x, y, tj);
            UndoStack.emplace_back(R.rec_type, tj, R.seg_id);
            tj->Select();
            break;
        }

        case RecType::CutGO:
        {
            GOptr g = FindObjByLoc(R.obj_type, R.coords);
            UndoStack.emplace_back(rt, StringImageObject(g), R.obj_type);
            delete g;
            break;
        }
            
        case RecType::MoveGO:
        {
            GOptr g = FindObjByLoc(R.obj_type, R.coords);
            auto [x, y] = R.coords_old;
            if (R.obj_type == TypeId::JOINT)
                ((TrackJoint*)g)->MoveToNewWPpos(x, y);
            else
                MoveGO(g, R.coords_old);
            UndoStack.emplace_back(rt, g, R.coords);
            break;
        }
            
        case RecType::PropChange:
        {
            GOptr g = FindObjByLoc(R.obj_type, R.coords);
            R.changed_props->Restore(g);
            UndoStack.emplace_back(g, R.orig_props.release(), R.changed_props.release());
            g->Invalidate();
            break;
        }

        case RecType::Wildfire:
        {
            int new_id = R.wf_objptr->new_tcid;
            for (auto wp : R.wf_objptr->Segvec) {
                TrackSeg* seg = static_cast<TrackSeg*>
                (FindObjectByTypeAndWPpos(TypeId::TRACKSEG, wp.x, wp.y));
                assert(seg);
                seg->SetTrackCircuit(new_id, FALSE);
                seg->Invalidate();
            }
            UndoStack.emplace_back(R.wf_objptr.release());
            break;
        }
            
        case RecType::CutSegment:
        {
            auto tj1 = (TrackJoint*)FindObjByLoc(TypeId::JOINT, R.coords);
            auto tj2 = (TrackJoint*)FindObjByLoc(TypeId::JOINT, R.coords_old);
            auto [x1, y1] = tj1->WPPoint();
            auto [x2, y2] = tj2->WPPoint();
            auto seg = (TrackSeg*)FindObjByLoc(TypeId::TRACKSEG,
                                    Coords((x1 + x2)/2, (y1 + y2)/2));
            UndoStack.emplace_back(R.rec_type, R.coords, R.coords_old);
            tj1->Select(); // tj1 may vanish!
            seg->Cut_();
            break;
        }
            
        case RecType::CreateSegment:
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
            UndoStack.emplace_back(R.rec_type, Coords(tj1), Coords(tj2));
            break;
        }
        

        default:
            break;
    }
    RedoStack.pop_back();
    compute_menu_state();
}



}
