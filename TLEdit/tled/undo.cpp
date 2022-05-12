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

using std::vector, std::string;
using GOptr = GraphicObject*;
GOptr ProcessNonGraphObjectCreateFormString(const char * s);

void SetUndoRedoMenu(const char * undo, const char * redo);

namespace Undo {

enum class RecType {CreateGO, CutGO, MoveGO,PropChange, CreateArc, DeleteArc, CreateJoint, DeleteJoint,
    SimpleMoveJoint
};

std::unordered_map<RecType, string> RecTypeNames {
    {RecType::CreateGO, "create"},
    {RecType::CutGO, "cut"},
    {RecType::PropChange, "property change"},
    {RecType::MoveGO, "move"},
    {RecType::SimpleMoveJoint, "move"}
};

struct Coords {
    Coords(WP_cord x, WP_cord y) {
        wp_x = x;
        wp_y = y;
    }
    Coords(GOptr g) {
        wp_x = g->wp_x;
        wp_y = g->wp_y;
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
        coords = C;
        rec_type = type;
        obj_type = g->TypeID();
    }

    RecType rec_type;
    string image;                    /* not valid or needed for create*/
    TypeId obj_type;
    Coords coords {0,0};

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
    UndoStack.emplace_back(RecType::MoveGO, g, GOMovCoords);
    
}

void Undo() {
    if (!IsUndoPossible())
        return;
    UndoRecord R = UndoStack.back();
    UndoStack.pop_back();
    switch(R.rec_type) {
        case RecType::CreateGO:
        {
            /* Can't store real object pointer in undo stack -- the object can be deleted and recreated
             (but at the same position) before the undo element is used. */

            GOptr g = FindObjectByTypeAndWPpos(R.obj_type, R.coords.wp_x, R.coords.wp_y);
            assert (g != nullptr);
            RedoStack.emplace_back(R.rec_type, StringImageObject(g), R.obj_type);
            delete g;
            break;
        }
            
        case RecType::CutGO:
        {
            GOptr obj = ProcessNonGraphObjectCreateFormString(R.image.c_str());
            obj->MakeSelfVisible();
            obj->Select();
            RedoStack.emplace_back(R.rec_type, obj, Coords(obj));
            break;
        }
        default:
            break;
            
    }
    compute_menu_state();
    
    
};

/* Snapshot and manage BufferModified */
void Redo() {
    if (!IsRedoPossible())
        return;
    UndoRecord R = RedoStack.back();
    RedoStack.pop_back();
    switch(R.rec_type) {
        case RecType::CreateGO:
        {
            GOptr obj = ProcessNonGraphObjectCreateFormString(R.image.c_str());
            obj->MakeSelfVisible();
            obj->Select();
            UndoStack.emplace_back(R.rec_type, "", obj);
            break;
        }
        case RecType::CutGO:
        {
            GOptr obj = FindObjectByTypeAndWPpos(R.obj_type, R.coords.wp_x, R.coords.wp_y);
            assert(obj != nullptr);
            UndoStack.emplace_back(R.rec_type, StringImageObject(obj), R.obj_type);
            delete obj;
            break;
        }
        default:
            break;
    }
    compute_menu_state();
};

}
