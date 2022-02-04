#ifndef _NXSYS_DIALOGS_H__
#define _NXSYS_DIALOGS_H__

enum FDlgExt {FDE_Layout = 0, FDE_Interpreted, FDE_XDO, FDE_NXScript};

BOOL FileOpenDlg (HWND hwnd, LPSTR lpstrFileName, LPSTR lpstrTitleName,
		  int bufl, int rsw, enum FDlgExt);

BOOL ScaleDialog();
void AboutDialog (HWND win, HINSTANCE instance);
void NullDialog (HWND win, HINSTANCE instance, char* name);

int ShowStopDlg(HWND win, HINSTANCE instance, int current_policy);
int RlyDialog (HWND win, HINSTANCE instance, char* buf);

void OfferChooseTrackDlg ();
void FlushChooseTrackDlg();
void OfferChooseTrackDlg(), FlushChooseTrackDlg();



#endif
