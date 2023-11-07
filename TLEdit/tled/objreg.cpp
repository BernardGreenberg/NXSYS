#include "windows.h"

#include "compat32.h"
#include "nxgo.h"
#include "tledit.h"
#include "objreg.h"
#include "LoadFiascoMaps.h"
#include <unordered_map>
#include <string>

/* Rewritten for dyn-create STL 4/9/2022 */

struct NXObjectRegistryEntry {
    ObjCreateFn ObjectCreationFunction;
    ObjClassInitFn ObjectClassInitFunction;
};

static std::unordered_map<TypeId, std::string> TypeIdNames {
    {TypeId::TRACKSEG, "track segment"},
    {TypeId::SIGNAL, "signal"},
    {TypeId::JOINT, "joint"},
    {TypeId::STOP, "stop"},
    {TypeId::EXITLIGHT, "exit light"},
    {TypeId::PANELLIGHT, "panel light"},
    {TypeId::PANELSWITCH, "panel switch"},
    {TypeId::TRAFFICLEVER, "traffic lever"},
    {TypeId::SWITCHKEY, "switch key"},
    {TypeId::TEXT, "text string"},
    {TypeId::NONE, "?NONE?"}
};

static LoadFiascoProtectedUnorderedMap<TypeId, UINT> DlgByTypeid;
static LoadFiascoProtectedUnorderedMap<int, NXObjectRegistryEntry>FnsByCommand;

/* this can be called from toplevels in other files even before this file's STL (would have been)
 initialized */
NXObTypeRegistrar::NXObTypeRegistrar (TypeId type, int command, UINT dlg_id, ObjCreateFn ocf, ObjClassInitFn ocif) {

    FnsByCommand[command] = NXObjectRegistryEntry{ocf, ocif};
    DlgByTypeid[type] = dlg_id;
}

UINT FindDialogIdFromObjClassRegistry (TypeId type) {
    return DlgByTypeid.count(type) ? DlgByTypeid[type] : 0;
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

const char * NXObjectTypeName(TypeId type) {
    if (TypeIdNames.count(type))
        return TypeIdNames[type].c_str();
    else
        return "??type??";
}
