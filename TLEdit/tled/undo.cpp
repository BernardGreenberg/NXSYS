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

void SetUndoRedoMenu(const char * undo, const char * redo);

namespace Undo {

std::unordered_map<int, const char *> ObjIdNames {
    {ID_TRACKSEG, "track segment"},
    {ID_SIGNAL, "signal"},
    {ID_JOINT, "joint"},
    {ID_STOP, "stop"},
    {ID_EXITLIGHT, "exit light"},
    {ID_PANELLIGHT, "panel light"},
    {ID_PANELSWITCH, "panel switch"},
    {ID_TRAFFICLEVER, "traffic lever"},
    {ID_SWITCHKEY, "switch key"},
    {ID_TEXT, "text string"}
};

enum class RecType {CreateGO, DeleteGO, PropChangeBefore, CreateArc, DeleteArc, CreateJoint, DeleteJoint };

struct UndoRecord {
    UndoRecord(RecType type, string img, GraphicObject* o) {
        rec_type = type;
        image = img;
        object = o;
        obj_id_type = object->TypeID();
    }
    UndoRecord(RecType type, string img, int obt) {
        rec_type = type;
        image = img;
        object = nullptr;
        obj_id_type = obt;
    }
    RecType rec_type;
    string image;                    /* not valid or needed for create*/
    GraphicObject* object = nullptr; /* not valid or needed for delete*/
    int obj_id_type;
    
    string DescribeCommand(string cmd) {
        string s = cmd + " CREATE ";
        s + ObjIdNames[obj_id_type];
        return s;
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
        undo_str = UndoStack.back().DescribeCommand("Undo");
        undo_avl = undo_str.c_str();
    }
    if (IsRedoPossible()) {
        redo_str = RedoStack.back().DescribeCommand("Redo");
        redo_avl = redo_str.c_str();
    }
    SetUndoRedoMenu(undo_avl, redo_avl);
}

void RecordGOCreation(GraphicObject* g){
    UndoStack.emplace_back(RecType::CreateGO, "", g);
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
    compute_menu_state();
};

}
