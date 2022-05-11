//
//  undo.cpp
//  TLEdit
//
//  Created by Bernard Greenberg on 5/10/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//
#include "objid.h"
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

std::unordered_map<ObjId, string> ObjIdNames {
    {ObjId::TRACKSEG, "track segment"},
    {ObjId::SIGNAL, "signal"},
    {ObjId::JOINT, "joint"},
    {ObjId::STOP, "stop"},
    {ObjId::EXITLIGHT, "exit light"},
    {ObjId::PANELLIGHT, "panel light"},
    {ObjId::PANELSWITCH, "panel switch"},
    {ObjId::TRAFFICLEVER, "traffic lever"},
    {ObjId::SWITCHKEY, "switch key"},
    {ObjId::TEXT, "text string"}
};

enum class RecType {CreateGO, DeleteGO, PropChange, CreateArc, DeleteArc, CreateJoint, DeleteJoint };

std::unordered_map<RecType, string> RecTypeNames {
    {RecType::CreateGO, "create"},
    {RecType::DeleteGO, "delete"},
    {RecType::PropChange, "property change"}
};

struct UndoRecord {
    UndoRecord(RecType type, string img, GraphicObject* o) {
        rec_type = type;
        image = img;
        object = o;
        obj_id_type = object->TypeID();
    }
    UndoRecord(RecType type, string img, ObjId obt) {
       rec_type = type;
       image = img;
       object = nullptr;
       obj_id_type = obt;
   }

    RecType rec_type;
    string image;                    /* not valid or needed for create*/
    GOptr object = nullptr; /* not valid or needed for delete*/
    ObjId obj_id_type;
    
    string DescribeAction() {
        return "Undo " + RecTypeNames[rec_type] + " " + ObjIdNames[obj_id_type];
    }

};

struct RedoRecord {
     RedoRecord(RecType type, string img, ObjId obt) {
        rec_type = type;
        image = img;
        object = nullptr;
        obj_id_type = obt;
    }
    RecType rec_type;
    string image;                    /* not valid or needed for create*/
    GOptr object = nullptr; /* not valid or needed for delete*/
    ObjId obj_id_type;
    
    string DescribeAction() {
        return "Redo " + RecTypeNames[rec_type] + " " + ObjIdNames[obj_id_type];
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
        undo_str = UndoStack.back().DescribeAction();
        undo_avl = undo_str.c_str();
    }
    if (IsRedoPossible()) {
        redo_str = RedoStack.back().DescribeAction();
        redo_avl = redo_str.c_str();
    }
    SetUndoRedoMenu(undo_avl, redo_avl);
}

void RecordGOCreation(GraphicObject* g){
    UndoStack.emplace_back(RecType::CreateGO, "", g);
    RedoStack.clear();
    compute_menu_state();
}

void RecordGODeletion(GraphicObject* g) {
    
}

void Undo() {
    if (!IsUndoPossible())
        return;
    UndoRecord R = UndoStack.back();
    UndoStack.pop_back();
    if (R.rec_type == RecType::CreateGO) {
        StringOutputWriter W;
        R.object->Dump(W);
        RedoStack.emplace_back(R.rec_type, W.get(), R.obj_id_type);
        delete R.object;
    }
    compute_menu_state();
    
    
};
void Redo() {
    if (!IsRedoPossible())
        return;
    UndoRecord R = RedoStack.back();
    RedoStack.pop_back();
    if (R.rec_type == RecType::CreateGO) {
        GraphicObject* obj = ProcessNonGraphObjectCreateFormString(R.image.c_str());
        obj->MakeSelfVisible();
        obj->Select();
    }
    compute_menu_state();
};

}
