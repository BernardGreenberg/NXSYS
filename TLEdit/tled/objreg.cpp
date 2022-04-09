#include "windows.h"
#include <unordered_map>

#include "compat32.h"
#include "nxgo.h"
#include "tledit.h"
#include "objreg.h"

/* Rewritten for dyn-create STL 4/9/2022 */

struct NXObjectRegistryEntry {
    ObjCreateFn ObjectCreationFunction;
    ObjClassInitFn ObjectClassInitFunction;
};

static std::unordered_map<int, UINT> *DlgIdByObjid = nullptr;
static std::unordered_map<int, NXObjectRegistryEntry> *FnsByCommand = nullptr;

NXObTypeRegistrar::NXObTypeRegistrar
   (int objid, int command, UINT dlg_id, ObjCreateFn ocf, ObjClassInitFn ocif) {
    /* "load order fiasco" "pattern" -- can't use static maps */
    if (FnsByCommand == nullptr) {
        DlgIdByObjid = new std::unordered_map<int, UINT>;
        FnsByCommand= new std::unordered_map<int, NXObjectRegistryEntry>;
    }

    (*FnsByCommand)[command] = NXObjectRegistryEntry{ocf, ocif};
    (*DlgIdByObjid)[objid] = dlg_id;
}

UINT FindDialogIdFromObjClassRegistry (int objid) {
    return (*DlgIdByObjid).count(objid) ? (*DlgIdByObjid)[objid] : 0;
}

GraphicObject * CreateObjectFromCommand (HWND hWnd, int command, int x, int y) {
    if ((*FnsByCommand).count(command) == 0)
        return NULL;

    RECT r;
    GetClientRect (hWnd, &r);
    if (x < r.left) x = r.left;
    if (y < r.top) y = r.top;
    if (x > r.right) x = r.right;
    if (y > r.bottom) y = r.bottom;
    int wpx = (int)(SCXtoWP (x) /* + Xoff*/);
    int wpy = (int)(SCYtoWP (y) /* + Yoff*/);
    return (*FnsByCommand)[command].ObjectCreationFunction(wpx, wpy);
}

void InitializeRegisteredObjectClasses() {
    for (auto& pair : *FnsByCommand) {
        auto E = pair.second;
        if (E.ObjectClassInitFunction != NULL)
            E.ObjectClassInitFunction();
    }
}
