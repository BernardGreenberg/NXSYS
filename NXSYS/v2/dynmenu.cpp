#define TEST_THROW false

#include "windows.h"
#include "lisp.h"
#include "stdio.h"
#include "relays.h"
#include "nxgo.h"
#include "objid.h"
#include "brushpen.h"
#include "nxsysapp.h"
#include "compat32.h"
#include "dynmenu.h"
#include "rlyapi.h"
#include <vector>
#include <string>
#include <memory>
#include <exception>
#include "relays.h"
#include "STLExtensions.h"
#include "WinApiSTL.h"

/* Relay-operated Windows menus - a clever hack by BSG, 21 February 1997 */
/* Mac adaptation 2014 */
/* C++11 STL version with STL instance-life management and move semantics Aug 2019 */

/* TBD
 Why don't we need message forwarding?
 */

/*
 14-15 Aug 2019
 
 The system has to know where all the button box representations are for only three reasons:
   1) to deallocate their resources, including dialogs, at scenario shutdown time
   2) to validate (for debug-assert) callback pointers sent in by relays
   3) to see if the box should pop up when a "train" reverses (in an already-
      occupied track circuit), which is very obscure.
 (2) and (3) are almost silly; (1) is the only real reason for tracking them.
 
 The possible object-instance management strategies are
   1) Array/vector of raw pointers to scattered objects (pre-Aug 2019),
   2) Array/vector of actual instances, residing in it
   3) Array/vector of std::unique_ptrs (or other STL smart ptrs) to instances.
 
 The original "raw pointer" method suffers in that the objects have to be iterate-deleted
 through the pointers, which can leave them bad, and it's not "STL elegant".
 
 "Array of actual instances" is very elegant; just push them on a vector/stack,
 but this has disadvantages - actual copying done by the language (which may be inexpensive),
 including temporary allocation of the original, and moving-around of elements already on the
 stack when it has to be reallocated to hold new elements, conceivably quadratically
 (depending upon vector reservation policy).  Worse yet, NXSYS objects disburse pointers to
 themselves to other objects for callback to themselves, most typically relays, timers, and
 operating-system-implemented dialogs, although only the relay connections are made during
 object construction/initialization (i.e., when there remains any possibility of
 "moving-around").  Relay-maintained pointers can now be handled by telling the relay
 (only can be one per object) to update its retained pointer when an object is moved
 (changes official location), which latter is now a supported concept and handleable event
 in C++11, handled thus via RelayMovingPointer.h and the mixin class it defines.
 
 The DynMenuEntry objects are now handled by "array of actual instances". They contain
 a relay pointer and are pointed-to by that relay, handled well by the mixin, and are
 otherwise small, no timers or dialogs or other callback address clients.  A vector
 in each DynMenu hosts its DynMenuEntrys whole, in place.
 
 The DynMenu objects, however, are dauntingly complex to handle by this method, for
 several reasons.  While the relay pointer and its reciprocation in each can be
 easily handled, there are other issues:
 
   1) A Mac or Windows dialog implementing the TrainId "menu" (button box) is created
      during DynMenu initialization.  That dialog then harbors a callback pointer to
      handle button pushes.  Mac menu implementation now supports a callback updater,
      but the Windows version stashes it non-accessibly.  Even in the Mac case,
      the DynMenu has to know when to call for the update, amd that requires its
      own "move constructor" ("movector").

   2) The DynMenuEntry instances already harbor a back-pointer to the DynMenu for
      inportant logical reasons.  They cannot possibly obtain a new value when
      the vector containing them is moved;  the movector of the DynMenu must handle it.
      The compiler demands we write a movector because we declare a destructor.
 
    3) The movector is seriously difficult to specify, code, and maintain:
       DynMenu::DynMenu(DynMenu&& old)
          : RelayMovingPointer<DynMenu>(std::move(old))   //Good guess, BSG
      invoking a base-class call on the mixin's own movector, which latter
      it has to know about, which is a modularity violation.  Worse yet, the
      movector has to include explicit code to copy every datum in the instance,
      some by simple assignment, others by std::move recursive "move" invocations,
      and others by the needs of (1) and (2) above.  Failure to remember to add
      or delete a line if instance variables are added or removed is a trap I have
      already fallen into.
 
 The method of unique_ptrs is chosen to eliminate the complexity and deficits
 of the DynMenu movector (DynMenuEntry doesn't even require its own).  The
 unique_ptrs limit access to the values to methods, and destroy the instances
 when the vector is cleared.  The DynMenu objects are scatter-allocated,
 and are few. Iteration over them (say, for callback pointer validation and
 train reversal) requires two or three more indirections, which are cheap).
 DynMenuEntries remain vector-hosted by full-object inclusion.
 
 The old DynMenu move ctor is conditionaled (#if 0) out below.
 
 2019 Aug 18:
 
 Entire "Move semantics" strategy and its include file discarded.  The right way
 to do this is to simply defer the setup of the Entrys' relay callbacks until
 a second pass through them, when the vector is fully formed and they are all
 in their final places.  This is far superior to the baroque pointer-moving
 system, but I sure learned a lot doing the latter.
 
 Also today, lambda expressions replaced the static callback functions. I'm
 not sure if that's better or not; while it does work, the setup functions
 that placed the lambda are still logically part of its heritage.

 */

#if NXSYSMac
#include "DynmenuMacCalls.h"
#endif

struct DynMenuCreateException : public std::exception {
    std::string msg;
    DynMenuCreateException(const char * beef) {
        msg = beef;
    }
};

#define CONTROL_ID_BASE 1000

/*  This is what a DynamicMenu (trackside TrainID pushbotton box) looks like
 in the interlocking definition:

    (MENU
0          223MENU		 	 ;relay symbol/identifier
1          (AND !4242NS !4242T)          ;expression that brings up the menu
2	   "Southbound Route"            ;title to appear on box
3	   (AUTODISMISS)                 ;options  -- currently ignored.
 ;;   Button definitions in order -- label, button(relay), lamp(relay)
4	   (("Brighton"   223MPBAPB 223MPBAK)
	    ("Fourth Ave" 223MPBBPB 223MPBBK)
	    ("West End"   223MPBCPB 223MPBCK)
	    ("Sea Beach"  223MPBDPB 223MPBDK)
	    ("Cancel"     223MPBEPB)))  ;no lamp

*/

/* Forward def so that parent pointer can be declared. */
class DynMenu;

static BOOL DynMenusEnabled = TRUE;

/* DynMenuEntry, corresponding to one row of button, label, lamp, must be declared before
   DynMenu, because the latter (reasonably) includes a "bodily residence" vector of the former */
class DynMenuEntry {
public:
    DynMenuEntry(Sexpr defining_lisp_form, DynMenu* menu, int control_id);
    void FinishUpInPlace(Sexpr defining_lisp_form);
    class DynMenu * Menu;		/* so callbacks can find - needed move-maintenance! */

    std::string String;                 /* string displayed as label on panel */
    UINT ControlId;                     /* OS dialog system control ID */
    Relay* PushButtonRelay;             /* Relay pulsed when button is clicked/pushed */
    ReportingRelay* ReporterRelay;

    void PushTheButton();               /* NYCT Push, not STL */
    void BeforeShow();                  /* set up "radio button" block to select right one */
    void LampReport (BOOL state);       /* manage lamp as directed by relay */

#if NXSYSMac
    void SendSelfToMac(HWND dlg) {
        SetWindowTextS(GetDlgItem(dlg, ControlId), String);
    }
#endif
};

/* Definition of the DynMenu class */

class DynMenu {
public:
    DynMenu(Sexpr rlysym, const char* title, Sexpr items);  // Ctor out of line, below.
    
    /* Destructor destroys the system dialog if its existence is recorded here */
    ~DynMenu() {
        if (Dlg)
            DestroyWindow (Dlg);
    }

    HWND Dlg;                                       // Handle (Windows or simulated) to OS dialog
    std::vector<DynMenuEntry> Entries;              // Button/lamp entries; self-relocating
    std::string Title;                              // Title shown on whole button box
    BOOL Up;                                        // Whether the box is visible.
    long Nomenclature;                              // interlocking "object number"
    Relay * MNZ;                                    // MNZ (Menu Control (Z=control) tell relays to put it up
    GraphicObject * TiedObject;                     // Object (usually signal) at which the box is located

    LRESULT DlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam); // Windows-compatible...
    void ShowIDBox (BOOL state);                    //Show or unshow the ID box.
    void SetLights();                               //Update state of lights
    HWND CreateSystemDialog();                      //Create Mac or Window dialog box.

    BOOL PushValidButton(int control_id) {
        int possible_index = control_id - CONTROL_ID_BASE;
        if (possible_index >= 0 && possible_index < (int)Entries.size()) {
            Entries[possible_index].PushTheButton();
            return TRUE;
        } else {
            return FALSE;
        }
    }
    void callCheckRadioButton(UINT to_be_checked) {
        CheckRadioButton(Dlg, CONTROL_ID_BASE, (UINT)(CONTROL_ID_BASE + Entries.size() -1), to_be_checked);
    }
#if NXSYSMac
    void SendDetailsToMac () {
        MacKludgeParam2(Dlg, (int)Entries.size(), 0);  // Mac Dlg must know the # entries a priori
        for (auto& entry : Entries)
            entry.SendSelfToMac(Dlg);
        SetWindowTextS(Dlg, Title);
    }
#endif

};


/* Define the vector managing the unique_ptrs to the DynMenu objects.
   See the explanation at file top.
*/
static std::vector<std::unique_ptr<DynMenu>>Menus;
static std::unique_ptr<DynMenu>& ValidateMenuCallback(void * ptr) {
    /* proves that our callback ptrs were updated validly. There aren't going to be
     dozens of dynamic menus in a scenario -- this test is cheap. */
    for (auto& rmenup : Menus) {   // iterate over a vector of unique_ptrs
        if ((void*)rmenup.get() == ptr)
            return rmenup;
    }
    assert(!"Alleged DynMenu relay callback ptr not found");
    /* These two lines will never, ever be executed, but the compiler (MSVC) doesn't know that,
       and without them complains that "not all control paths return a value;  Returning a
       value from a function that returns references to a unique_ptr is no trivial matter. */
    /* This is very crazy; There ought be a compiler annotation. */
    static std::unique_ptr<DynMenu> dumy;
    return dumy;
}

/* DynMenu general constructor; destructor is inline */
DynMenu::DynMenu(Sexpr rlysym, const char* title, Sexpr items) {
    /* Initialize POD values appropriately */
    Nomenclature = rlysym.u.r->n;
    Title = title;       // char* -> STL string
    Up = FALSE;

    /* point to NXSYS object not interested in their referrers */
    TiedObject = FindHitObject (Nomenclature, ID_SIGNAL);
    MNZ = GetRelay2NoCreate (Nomenclature, "MNZ");

    /* Create and stack the entries for the TrainID Box rows.  Moving others
       may happen during emplace allocation. */
    Sexpr save_items = items;
    for (int control_id = CONTROL_ID_BASE; CONSP(items); control_id++)
        Entries.emplace_back(SPopCar(items), this, control_id);  // <DynMenuEntry>
    /* Now, once they're all in place, do a second pass, setting up the callbacks. */
    for (auto& entry : Entries)
        entry.FinishUpInPlace(SPopCar(save_items));

    /* Create the reporting relay (if it doesn't exist) */
    auto rr = CreateReportingRelay(rlysym);
    rr->SetReporter ([](BOOL state, void*v) {
        if (DynMenusEnabled || !state)
            ValidateMenuCallback(v)->ShowIDBox(state);
    }, this);

    /* Create the system dialog that represents this Train ID box on screen */
    Dlg = CreateSystemDialog();
}

#if 0   /* For entertainment purposes only. Note the duplicated knowledge of every
       instance variable, and three or four strategies for moving them.
*/
/* DynMenu move constructor; move ctor = Mo; vector! */
DynMenu::DynMenu(DynMenu&& old)
: RelayMovingPointer<DynMenu>(std::move(old))   //Good guess, BSG
{
    /* Copy nonstructured data and strings in the Paleolithic fashion */
    Title = old.Title;
    Nomenclature = old.Nomenclature;
    Up = old.Up;
    MNZ = old.MNZ;
    TiedObject = old.TiedObject;
    
    /* Now copy the entries vector, which requires official 'move' and more. */
    Entries = std::move(old.Entries);           //This invokes their own movectors.
    for (auto& entry : Entries)                 //But they don't know about this.
        entry.Menu = this;
    
    /* The Dlg pointer requires more special casing.  The pointer in the old instance
     has to be clobbered lest it provoke the dlg's destruction when temp is destructed,
     and the dialog itself has to be updated to our new location, not possible (yet) on MSWindows */
    Dlg = old.Dlg;                              // The dialog handle/pointer doesn't change.
    old.Dlg = nullptr;                          // Null the old pointer (see above)
#if NXSYSMac
    if (Dlg)                                    // If it exists, update the dialog's retained ptr to us
        UpdateDlgCallbackActor(Dlg, this);
#endif
}
#endif

void DynMenu::ShowIDBox (BOOL state) {
    if (state) {
        RECT r;
        if (TiedObject && TiedObject->Visible) {
            GetWindowRect (Dlg, &r);
            POINT Origin = {0, 0};
            ClientToScreen (G_mainwindow, &Origin);
            int x = TiedObject->sc_x - 3*GU2 + Origin.x;
            int y = TiedObject->sc_y + 4*GU2 + Origin.y;
#if NXSYSMac
            setFrameTopLeft(Dlg, x, y);
#else
            int height = r.bottom - r.top;
            int width = r.right-r.left;
            MoveWindow (Dlg, x, y, width, height, FALSE);
#endif
        }
        SetLights();
    }
    ShowWindow (Dlg, state ? SW_SHOW : SW_HIDE);
}


LRESULT DynMenu::DlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    
    switch (message) {
            
        case WM_INITDIALOG:
            SetLights();
            Up = TRUE;
            return FALSE;
            
        case WM_SHOWWINDOW:
            if (wParam)
                SetLights();
            Up = (BOOL)wParam;
            return TRUE;
            
        case WM_COMMAND:
            switch (wParam) {
                case IDCANCEL:
                    Up = FALSE;
                    ShowWindow (hDlg, SW_HIDE);
                    return TRUE;
                default:
                    if (!Up)
                        return FALSE;
                    return PushValidButton((int)wParam);
            }
        default:
            return FALSE;
    }
}

void DynMenu::SetLights () {
    for (auto& entry : Entries)
        if (entry.ReporterRelay)                              // Cancel buttons don't have lights!
            entry.BeforeShow();
}

#if NXSYSMac
void DynmenuButtonPushCallback(void * v, int control_id) {   //Called from Mac ObjC/Cocoa code...
    ValidateMenuCallback(v)->PushValidButton(control_id);
}

/* Mac version -- Windows version with dialog-data semantics at bottom */
HWND DynMenu::CreateSystemDialog() {
    Dlg = MacCreateDynmenu(this);          //ObjectiveC++/Cocoa function
    SendDetailsToMac();
    return Dlg;
}
#endif


/* DynMenuEntry ctor, methods and related functions */

DynMenuEntry::DynMenuEntry(Sexpr button_def, DynMenu* pmenu, int control_id) :
   Menu(pmenu), ControlId(control_id), ReporterRelay(nullptr) {
    
    if (CAR(button_def).type != Lisp::STRING)
        throw DynMenuCreateException("Item destination label not a string");
    String = SPopCar(button_def).u.s;  // yay STL!

    if (CAR(button_def).type != Lisp::RLYSYM)
        throw DynMenuCreateException("Pushbutton relay symbol not a relay symbol");
    PushButtonRelay = CreateRelay (SPopCar(button_def));   // not a reporter, not moveneedy

    //Defer back-pointing relay connection until all DynMenuEntries are in place...
}

/* Pointers cannot be reported to the relay system until each DynMenuEntry is in its final place, and
   the vector will no longer expand.  The DynMenu's are allocated statically and do not move. */
void DynMenuEntry::FinishUpInPlace(Sexpr item_def) {
    Sexpr cddr = CDDR(item_def);
    if (CONSP(cddr)) {      //Some don't have lamps (e.g., "Cancel").
        Sexpr rlysym = CAR(cddr);   // real Lisp!!! :)
        ReporterRelay = CreateReportingRelay(rlysym);
        ReporterRelay->SetReporter ([](BOOL state, void* v) {
            ((DynMenuEntry*)v)->LampReport(state);
        }, this);   //divulge our (current, hopefully final) address to external agent!
    }
}

void DynMenuEntry::PushTheButton () {    // not the vector!
    PulseToRelay (PushButtonRelay);
}

void DynMenuEntry::BeforeShow () {
    bool state = ReporterRelay->State & 1;
    if (state) {
        HWND control = GetDlgItem(Menu->Dlg, ControlId);
        SetFocus(control);
        Menu->callCheckRadioButton(ControlId); //"Check" means "assert check-mark"
    }
}

void DynMenuEntry::LampReport (BOOL state) {
    if (Menu->Dlg)  {
#if NXSYSMac
        BeforeShow();
#else
	SendDlgItemMessage(Menu->Dlg, ControlId, BM_SETCHECK, state, 0L);
#endif
    }
}

/* Finally, the once-grand function that creates a DynMenu and its DynMenuEntry's fron Lisp,
   although much of that is now delegated to their respective constructors.
*/

int DefineMenuFromLisp (Sexpr s) {

    if (ListLen (s) < 5) {
        LispBarf ("Fewer than five arguments in MENU form", s);
        return 0;
    }

    Sexpr rlysym = SPopCar(s);   //Needed for catch clause, too.  Might be "bad", too...
    try {
        if (rlysym.type != Lisp::RLYSYM)
            throw DynMenuCreateException("Required relay symbol in MENU not a relay symbol");

        Sexpr expr = Lisp_Cons(SPopCar(s), NIL); // Make it look like a train cdr.
        if (!DefineRelayFromLisp2(rlysym, expr))   //compilation errors in expr can fail this way
            throw DynMenuCreateException("Creation of menu relay failed");

        if (TEST_THROW || CAR(s).type != Lisp::STRING)
            throw DynMenuCreateException("Title string in MENU not a Lisp string");
        const char * title = SPopCar(s).u.s;

        Sexpr options = SPopCar(s);
        (void)options;                 // ignorem for now.

        Sexpr button_items = SPopCar(s);
        if (!CONSP(button_items))
            throw DynMenuCreateException("Button list in MENU not a list");

        // Creates std::unique_ptr in place; compiler doesn't like push_back(std::unique_ptr....)
        // std::make_unique is in C++14, but not 11, we do it in header file. Don't say std::move, either.
        Menus.emplace_back(make_unique<DynMenu>(rlysym, title, button_items));  //can throw, too)

    } catch (DynMenuCreateException e) {
        std::string message = "Error in dynamic MENU definition:\n";
        message += e.msg + "\n for MENU";
        LispBarf(message.c_str(), rlysym);
        return 0;
    }

    return 1;
}

/* External API other than creation */

/* Called at interlocking Deinstall and app shutdown */
void DestroyDynMenus() {
    Menus.clear();
}

void EnableDynMenus (BOOL onoff) {
    DynMenusEnabled = onoff;
}

/* References to AZ are in the relay "code", not here */
static Relay * AZ = NULL;               // "Automatic control" relay.

BOOL AutoControlRelayExists () {
    AZ = GetRelay2NoCreate (0, "AZ");
    return (AZ != NULL);
}

/* Called from the app menu, presently */
void EnableAutomaticOperation (BOOL onoff) {
    if (AZ)
	ReportToRelay (AZ, onoff);
}

/* Called by Train system when a train reverses to see if Train
   has an ID Box at its new front end.  Identifying menus by nomenclature
   is also a really safe way (i.e., instead of moveneedy pointers) */
void TrySignalIDBox (long nomenclature) {
    for (auto& rmenup : Menus) {
        if (rmenup->Nomenclature == nomenclature) {
            if (rmenup->MNZ)
                PulseToRelay (rmenup->MNZ);
            break;
	}
    }
}

    
/* MS Windows version may never see the light of day again -- Aug 2019
 */
        
#if !(NXSYSMac)
        
static int nCopyAnsiToWideChar (LPWORD lpWCStr, const char * lpAnsiIn)
{
    int nChar = 0;
            
    do {
        *lpWCStr++ = (WORD) *lpAnsiIn;
        nChar++;
    } while (*lpAnsiIn++);
            
    return nChar;
}
        
static WORD* lpwAlign ( WORD* lpIn)
{
    INT_PTR ul;
            
    ul = (INT_PTR)lpIn;
    ul +=3;
    ul >>=2;
    ul <<=2;
    return (WORD*)ul;
}

static INT_PTR staticDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_INITDIALOG) {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
    }
    DynMenu* dmp = (DynMenu*)(void*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
    return dmp->DlgProc(hWnd, msg, wParam, lParam);
}
                              /*   MS-Windows  ifdef continues ... */

 HWND DynMenu::CreateSystemDialog () {
            
    WORD  *p, *pdlgtemplate;

    int   nchar;
    DWORD lStyle;
    int ypos;
    
    /* allocate some memory to play with  */
    pdlgtemplate = p = (PWORD) LocalAlloc (LPTR, 2048);
    
    /* start to fill in the dlgtemplate information.  addressing by WORDs */
    lStyle = DS_MODALFRAME | DS_3DLOOK | DS_CENTERMOUSE | WS_POPUP | WS_CAPTION;
  
    *p++ = LOWORD (lStyle);
    *p++ = HIWORD (lStyle);
    *p++ = 0;          // LOWORD (lExtendedStyle)
    *p++ = 0;          // HIWORD (lExtendedStyle)
    *p++ = (WORD)Entries.size();
    *p++ = (WORD)(10*Menus.size());         // x
    *p++ = (WORD)(10*Menus.size());         // y
    *p++ = 100;        // cx
    *p++ = (WORD)(10+(Entries.size())*20);
    *p++ = 0;          // Menu
    *p++ = 0;          // Class
    
    /* copy the title of the dialog */
    nchar = nCopyAnsiToWideChar (p, Title.c_str());
    p += nchar;
    
    /* add in the wPointSize and szFontName here iff the DS_SETFONT bit on */
    
    ypos = 10;
    for (size_t bno = 0; bno < Entries.size(); bno++) {
        
        /* make sure each item starts on a DWORD boundary */
        p = lpwAlign (p);
        
        lStyle = BS_RADIOBUTTON | WS_VISIBLE | WS_CHILD;
        if (bno == 0)
            lStyle |= WS_GROUP;
        
        *p++ = LOWORD (lStyle);
        *p++ = HIWORD (lStyle);
        *p++ = 0;          // LOWORD (lExtendedStyle)
        *p++ = 0;          // HIWORD (lExtendedStyle)
        *p++ = 10;         // x
        *p++ = ypos;       // y
        *p++ = 145;        // cx
        *p++ = 10;         // cy
        *p++ = Entries[bno].ControlId;
        
        ypos += 20;
        /* fill in class i.d. Button in this case */
        *p++ = (WORD)0xffff;
        *p++ = (WORD)0x0080;
       /* copy the text of the item */
        nchar = nCopyAnsiToWideChar (p, Entries[bno].String.c_str());
        p += nchar;
        
        *p++ = 0;  // advance pointer over nExtraStuff WORD
    }
    
    Dlg = CreateDialogIndirectParam
        (app_instance, (LPDLGTEMPLATE) pdlgtemplate,
         G_mainwindow, (DLGPROC)staticDialogProc, (LPARAM)(void*) this);
    
    LocalFree (LocalHandle (pdlgtemplate));
    return Dlg;
}

BOOL IsMenuDlgMessage(MSG* m) {
     for (size_t i = 0;i < Menus.size(); i++) {
         if (IsDialogMessage(Menus[i]->Dlg, m))
             return TRUE;
     }
     return FALSE;
 }
#endif

