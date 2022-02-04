#ifndef _NXSYS_DYNAMIC_MENU_H__
#define _NXSYS_DYNAMIC_MENU_H__


#ifndef _NX_LISP_SYS_H__
struct Sexpr;
#endif

void DestroyDynMenus();
int DefineMenuFromLisp (Sexpr s);
BOOL IsMenuDlgMessage (MSG*);
void EnableDynMenus(BOOL onoff);
void TrySignalIDBox (long nomenclature);

void EnableAutomaticOperation(BOOL onoff);
BOOL AutoControlRelayExists();
#endif
