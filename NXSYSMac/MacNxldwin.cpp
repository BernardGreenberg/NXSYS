#include "windows.h"

#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include "compat32.h"
#include "commands.h"
#include "ssdlg.h"
#include "dialogs.h"
#include "nxsysapp.h"
#include "trainapi.h"
#include "nxgo.h"
#include "lisp.h"
#include "usermsg.h"
#include <string>
#include <vector>

#include "AppDelegateGlobals.h"
#define ID_EXTHELP0 10000   

#define TIME_FORMAT_STR "%#d %b %Y %H:%M"

bool SimpleGetDialog(const char *, const char *, char*, int);
int StopPolicyDialog(int oldPolicy);
void OfferChooseTrack();

bool GlobalOfferingChooseTrack = false;

void HelpSystemDisplay(const char * text);
void HelpSystemDisplayURL(const char * url);

struct HelpText {
    std::string Title;
    std::string Text;
    std::string URL;
    UINT FixedId;
    void Display();
    void Dialog();
    HelpText (const char * text, const char * title, UINT hfid);
};

static std::vector<HelpText> HelpTexts;
static int train_cmd;
void OfferChooseTrackDlg(bool go){
    if (GlobalOfferingChooseTrack)
        return;  // one at a time, please.
    if (go) {
        train_cmd = TRAIN_HALTCTL_GO;
    } else {
        train_cmd = TRAIN_HALTCTL_HALTED;
    }
    GlobalOfferingChooseTrack = true;
    OfferChooseTrack();
}

void LoseChooseTrack();
void TrainDialog(GraphicObject* g, int ctl);

void GlobalChooseTrackHandler(void * v) {
    GlobalOfferingChooseTrack = false;
    LoseChooseTrack();
    TrainDialog((GraphicObject*)v, train_cmd);
}

Relay* MacRlyDialog (const char* label) {
    char buf[32];
    if (SimpleGetDialog(label, "Enter relay symbol (e.g., 244pbs)", buf, 32)) {
        if (!strcmp(buf, "")) {  /* vrfy for blanks? */
            return NULL;
        }
        Sexpr s = RlysymFromStringNocreate (buf);
	if (s == NIL || s.u.r->rly==NULL) {
	    usermsg ("No such relay: %s", buf);
	    return NULL;
        }
        return s.u.r->rly;
    }
    return NULL;
}

HWND ChooseTrackDlg = NULL;

void FlushChooseTrackDlg() {
    if (ChooseTrackDlg != NULL)
	DestroyWindow (ChooseTrackDlg);
    ChooseTrackDlg = NULL;
}
/*dynamic help-menu content management */

void DisplayHelpTextByCommand (UINT Cmd) {
  //  HelpTexts[Cmd - ID_EXTHELP0].Display();
}
void RegisterHelpMenuText (const char * text, const char * title) {
    HelpTexts.push_back(HelpText(text, title,0));
}

void RegisterInHelpfileMenuItem (const char * title, UINT helpfile_id) {
    HelpTexts.push_back(HelpText(NULL, title, helpfile_id));
}

void RegisterHelpURL(const char * title, const char * URL) {
    HelpText T (NULL, title, 0);
    T.URL = URL;
    HelpTexts.push_back(T);
}

HelpText::HelpText (const char * text, const char * title, UINT hfid) {
    if (text)
        Text = text;
    FixedId = hfid;
    if (title)
        for (auto p = title; *p != '\0'; p++)
            if (*p != '&')
                Title += *p;
    // To be accurate, there should be n helps -- but one for now.
    ScenarioHelpTitler(Title.c_str());
}

void HelpText::Display () {
    if (URL.size()) {
        HelpSystemDisplayURL(URL.c_str());
    } else if (FixedId) {
//	WinHelp (G_mainwindow, HelpPath, HELP_CONTEXT, FixedId);
    }
    else if (Text.length() > 5 && Text.substr(0, 5) == "file:") {
        HelpSystemDisplayURL(Text.c_str());
    }
    else {
        HelpSystemDisplay(Text.c_str());
    }
}

void ScenarioHelp(int n) {
    if (HelpTexts.size() == 0) {
        MessageBox(NULL, "This scenario doesn't provide any interlocking-specific help.", "NXSYS / Mac help", MB_OK);
        return;
    }
    // assert(n - 1 < N)
    HelpTexts[n-1].Display();
}

void ClearHelpMenu () {
    HelpTexts.clear();
}

