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
    Coords(int xx, int yy) {
        x = xx; y = yy;
    }
    Coords(GOptr g) {
        x = g->wp_x;
        y = g->wp_y;
    }
    WP_cord x, y;
};

struct UndoRecord {
    UndoRecord(RecType type, string img, GraphicObject* o) {
        rec_type = type;
        image = img;
        object = o;
        obj_type = object->TypeID();
    }
    UndoRecord(RecType type, string img, TypeId obt) {
       rec_type = type;
       image = img;
       object = nullptr;
       obj_type = obt;
    }
    UndoRecord(RecType type, GOptr g, Coords C) : coords (C) {
        rec_type = type;
        object = g;
        obj_type = object->TypeID();
    }

    RecType rec_type;
    string image;                    /* not valid or needed for create*/
    GOptr object = nullptr; /* not valid or needed for delete*/
    TypeId obj_type;
    Coords coords {0,0};

    string DescribeAction() {
        return "Undo " + RecTypeNames[rec_type] + " " + NXObjectTypeName(obj_type);
    }

};

struct RedoRecord {
     RedoRecord(RecType type, string img, TypeId obt) {
        rec_type = type;
        image = img;
        object = nullptr;
        obj_id_type = obt;
    }
    RedoRecord(RecType type, string img, GraphicObject* o) {
        rec_type = type;
        image = img;
        object = o;
        obj_id_type = object->TypeID();
    }
    RecType rec_type;
    string image;                    /* not valid or needed for create*/
    GOptr object = nullptr; /* not valid or needed for delete*/
    TypeId obj_id_type;
    
    string DescribeAction() {
        return "Redo " + RecTypeNames[rec_type] + " " + NXObjectTypeName(obj_id_type);
    }

};


static vector <UndoRecord> UndoStack;
static vector <RedoRecord> RedoStack;

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
        undo_str = UndoStack.back().DescribeAction();
        undo_avl = undo_str.c_str();
    }
    if (IsRedoPossible()) {
        redo_str = RedoStack.back().DescribeAction();
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
            RedoStack.emplace_back(R.rec_type, StringImageObject(R.object), R.obj_type);
            delete R.object;
            break;
            
        case RecType::CutGO:
        {
            GOptr obj = ProcessNonGraphObjectCreateFormString(R.image.c_str());
            obj->MakeSelfVisible();
            obj->Select();
            RedoStack.emplace_back(R.rec_type, "", obj);
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
    RedoRecord R = RedoStack.back();
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
            UndoStack.emplace_back(R.rec_type, StringImageObject(R.object), R.obj_id_type);
            delete R.object;
            break;
        }
        default:
            break;
    }
    compute_menu_state();
};

}
