#ifndef _TLEDIT_PROPERTY_DLG_H__
#define _TLEDIT_PROPERTY_DLG_H__

void uerr (HWND hDlg, const char * control_string, ... );
void SetDlgItemCheckState (HWND hDlg, UINT id, BOOL state);
BOOL GetDlgItemCheckState (HWND hDlg, UINT id);

#endif
