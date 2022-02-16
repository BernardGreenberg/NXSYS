#ifndef _NXSYS_DIALOGS_H__
#define _NXSYS_DIALOGS_H__

enum class FDlgExt {Layout = 0, Interpreted, XDO, NXScript, ShellScript};
#include <string>
#include <utility>

struct FDlgReturnVal {
	bool valid;
	std::string Path;
	std::string FileTitle;
	FDlgReturnVal() : valid(false), Path(""), FileTitle("") {}
	FDlgReturnVal(const char* path, const char* ftitle) : valid(true), Path(path), FileTitle(ftitle) {}
};

BOOL FileOpenDlg(HWND hwnd, LPSTR lpstrFileName, LPSTR lpstrTitleName, int bufl, int rsw, enum FDlgExt);

struct FDlgReturnVal FileOpenDlgSTL(HWND hWnd, const std::string file_name, const std::string title, bool rsw, FDlgExt ext);

BOOL ScaleDialog();
void AboutDialog (HWND win, HINSTANCE instance);
void NullDialog (HWND win, HINSTANCE instance, char* name);

int ShowStopDlg(HWND win, HINSTANCE instance, int current_policy);
std::pair<bool, std::string> RlyDialog(HWND win, HINSTANCE instance);
void StatusReportDialog(HWND win, HINSTANCE instance);

void OfferChooseTrackDlg ();
void FlushChooseTrackDlg();
void OfferChooseTrackDlg(), FlushChooseTrackDlg();



#endif
