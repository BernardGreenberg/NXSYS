#define OBJ_TYPE_REGISTRY_MAX 50

#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "compat32.h"
#include "nxgo.h"
#include "xtgtrack.h"
#include "tledit.h"
#include "objreg.h"

struct NXObjectRegistryEntry {
    short ObjectId;
    short DialogId;
    int Command;
    ObjCreateFn ObjectCreationFunction;
    ObjClassInitFn ObjectClassInitFunction;
    int ObjectCount;
};


static int N_Registry = 0;
static NXObjectRegistryEntry Registry[OBJ_TYPE_REGISTRY_MAX];

typedef NXObjectRegistryEntry *ObjRegEntP;

NXObTypeRegistrar::NXObTypeRegistrar
   (int objid, int command, UINT dlg_id, ObjCreateFn ocf, ObjClassInitFn ocif) {
    /* check overflow */
    ObjRegEntP e = &Registry[N_Registry++];
    e->ObjectId = objid;
    e->Command = command;
    e->DialogId = dlg_id;
    e->ObjectCreationFunction = ocf;
    e->ObjectClassInitFunction = ocif;
    e->ObjectCount = 0;
}

UINT FindDialogIdFromObjClassRegistry (int objid) {
    for (int i = 0; i < N_Registry; i++) {
	ObjRegEntP e = &Registry[i];
	if (e->ObjectId == objid)
	    return e->DialogId;
    }
    return 0;
}

GraphicObject * CreateObjectFromCommand (HWND hWnd, int command, int x, int y) {
    int i;
    ObjRegEntP e = NULL;
    for (i = 0; i < N_Registry; i++) {
	e = &Registry[i];
	if (e->Command == command)
	    break;
    }
    if (i >= N_Registry)
	return NULL;
    RECT r;
    GetClientRect (hWnd, &r);
    if (x < r.left) x = r.left;
    if (y < r.top) y = r.top;
    if (x > r.right) x = r.right;
    if (y > r.bottom) y = r.bottom;
    int wpx = (int)(SCXtoWP (x) /* + Xoff*/);
    int wpy = (int)(SCYtoWP (y) /* + Yoff*/);
    return e->ObjectCreationFunction(wpx, wpy);
}

void InitializeRegisteredObjectClasses() {
    for (int i = 0; i < N_Registry; i++) {
	ObjRegEntP e = &Registry[i];
	if (e->ObjectClassInitFunction != NULL)
	    (e->ObjectClassInitFunction)();
    }
}
