#include "windows.h"

#include "compat32.h"
#include "nxgo.h"
#include "tledit.h"
#include "objreg.h"
#include "LoadFiascoMaps.h"

/* Rewritten for dyn-create STL 4/9/2022 */

struct NXObjectRegistryEntry {
    ObjCreateFn ObjectCreationFunction;
    ObjClassInitFn ObjectClassInitFunction;
};


static LoadFiascoProtectedUnorderedMap<TypeId, UINT> DlgIdByObjid;
static LoadFiascoProtectedUnorderedMap<int, NXObjectRegistryEntry>FnsByCommand;

/* this can be called from toplevels in other files even before this file's STL (would have been)
 initialized */
NXObTypeRegistrar::NXObTypeRegistrar (TypeId objid, int command, UINT dlg_id, ObjCreateFn ocf, ObjClassInitFn ocif) {

    FnsByCommand[command] = NXObjectRegistryEntry{ocf, ocif};
    DlgIdByObjid[objid] = dlg_id;
}

UINT FindDialogIdFromObjClassRegistry (TypeId objid) {
    return DlgIdByObjid.count(objid) ? DlgIdByObjid[objid] : 0;
}

GraphicObject * CreateObjectFromCommand (HWND hWnd, int command, int x, int y) {
    if (FnsByCommand.count(command) == 0)
        return NULL;

    RECT r;
    GetClientRect (hWnd, &r);
    if (x < r.left) x = r.left;
    if (y < r.top) y = r.top;
    if (x > r.right) x = r.right;
    if (y > r.bottom) y = r.bottom;
    int wpx = (int)(SCXtoWP (x) /* + Xoff*/);
    int wpy = (int)(SCYtoWP (y) /* + Yoff*/);
    return FnsByCommand[command].ObjectCreationFunction(wpx, wpy);
}

void InitializeRegisteredObjectClasses() {
    for (auto& pair : FnsByCommand) {
        auto E = pair.second;
        if (E.ObjectClassInitFunction != NULL)
            E.ObjectClassInitFunction();
    }
}
