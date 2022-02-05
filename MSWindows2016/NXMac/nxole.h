#ifndef  _NXSYS_OLE_API_H__
#define  _NXSYS_OLE_API_H__

BOOL EnsureOleInitialized(BOOL need_registration);

BOOL InitNXOLE ();
BOOL CloseNXOLE();
BOOL RegisterNXOLE();
BOOL UnRegisterNXOLE();

/* client side */
void NXScript (const char * filename);
void CommandLoopDlg();
BOOL IsCmdLoopDlgMessage (MSG*);
#endif
