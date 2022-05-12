#ifndef _TLEDIT_OBJECT_REGISTRY_H__
#define _TLEDIT_OBJECT_REGISTRY_H__

#include "typeid.h"
#include "resource.h"
#include "tlecmds.h"

typedef GraphicObject* (*ObjCreateFn)(int wp_x, int wp_y);
typedef void           (*ObjClassInitFn)(void);

void RegisterNXObjectType (TypeId type,	/* object type */
			   UINT command, /* creation command */
			   UINT dlg_id, /* dialog id */
			   ObjCreateFn obfn, /* function to create 'em */
			   ObjClassInitFn class_init_fn);
			    /* function to init the class when are none */

class NXObTypeRegistrar {
    public:
	NXObTypeRegistrar(TypeId type, int command, UINT dlg_id, ObjCreateFn, ObjClassInitFn);
};

#define REGISTER_NXTYPE(typeid,command,did,obfn,cifn) \
static NXObTypeRegistrar Registrar##did (typeid,command,did,obfn,cifn);


GraphicObject * CreateObjectFromCommand (HWND hWnd, int command, int x, int y);
UINT FindDialogIdFromObjClassRegistry (TypeId type);

void InitializeRegisteredObjectClasses();

#endif
