//
//  undo.cpp
//  TLEdit
//
//  Created by Bernard Greenberg on 5/10/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//
#include "typeid.h"
#include "nxgo.h"
#include "objreg.h"
#include "undo.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdarg>
#include <cassert>
#include <memory>

using std::vector, std::string;
using GOptr = GraphicObject*;
GOptr ProcessNonGraphObjectCreateFormString(const char * s);

void SetUndoRedoMenu(const char * undo, const char * redo);

namespace Undo {

enum class RecType {CreateGO, CutGO, MoveGO, PropChange, CreateArc, DeleteArc, CreateJoint, DeleteJoint,
    SimpleMoveJoint
};

std::unordered_map<RecType, string> RecTypeNames {
    {RecType::CreateGO, "create"},
    {RecType::CutGO, "cut"},
    {RecType::PropChange, "property change"},
    {RecType::MoveGO, "move"},
    {RecType::SimpleMoveJoint, "move"}
};

PropCellBase::~PropCellBase () {
    //even though destructor is marked pure, this must be provided!!!
}

struct Coords {
    Coords(WP_cord x, WP_cord y) {
        wp_x = x;
        wp_y = y;
    }
    Coords(GOptr g) {
        wp_x = g->wp_x;
        wp_y = g->wp_y;
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

    RecType rec_type;
    string image;                    /* not valid or needed for create*/
    TypeId obj_type;
    Coords coords {0,0};
    Coords coords_old {-1,-1};
    std::unique_ptr<PropCellBase> orig_props;
    std::unique_ptr<PropCellBase> changed_props;

    string DescribeAction(string tag) {
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

static GOptr FindObjByLoc(TypeId type, const Coords& C) {
    GOptr g = FindObjectByTypeAndWPpos(type, C.wp_x, C.wp_y);
    assert(g);
    return g;
}

static void MoveGO (GOptr g, const Coords& C) {
    g->MoveWP(C.wp_x, C.wp_y);
}

static void MarkForwardAction() {
    RedoStack.clear();
    compute_menu_state();
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


void Undo() {
    if (!IsUndoPossible())
        return;
    UndoRecord& R = UndoStack.back();
    RecType rt = R.rec_type;
    switch(rt) {
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

        case RecType::MoveGO:
        {
            GOptr g = FindObjByLoc(R.obj_type, R.coords);
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

        default:
            break;
            
    }
    UndoStack.pop_back();
    compute_menu_state();
}

/* Snapshot and manage BufferModified */
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

 

        default:
            break;
    }
    RedoStack.pop_back();
    compute_menu_state();
}



}
