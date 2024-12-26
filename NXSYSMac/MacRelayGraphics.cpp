#include "windows.h"
#include "lisp.h"
#include <stdio.h>
#include "relays.h"
#include "ldraw.h"
#include "MacRlyDlg.h"  //Shared for now until real Windows Rlydlg is fixed.
#include <vector>
#include <string>


Relay* MacRlyDialog(const char* title);
void ShowStateRelay(Relay*);
int RelayGraphicsMouse(WPARAM, WORD, WORD);
void PlaceDrawing();
void DrafterMakeScrollVisible(HWND, int x, int y, int h);
HWND getRelayDrafterHWND(bool force);

extern int LastDrawingX;
extern int LastDrawingY;
extern int LastDrawingRise;

#define YSLOP 200  //in order to show context.

void InvalidateRelayDrafter () {
    HWND rd = getRelayDrafterHWND(false);
    if (rd != NULL)
        InvalidateRect(rd, NULL, 0);
}


static void ScrollToLastDrawing () {
    int targy = LastDrawingY + LastDrawingRise;
    if (targy > YSLOP) {
        targy -= YSLOP;
    }
    targy -= 15; //lettering on top
    DrafterMakeScrollVisible(getRelayDrafterHWND(true), LastDrawingX, targy, 50);
}


static void TryToPlaceRelayDrawing() {
    if (!PlaceRelayDrawing()) {
        MessageBox (0, "Requested relay circuit too big for "
			"current size of relay graphics window.  Make it longer and "
			"maybe NARROWER.", PRODUCT_NAME " Relay Graphics",
			MB_OK | MB_ICONSTOP);
        return;
    }
   // printf("Last x %d, y %d\n", LastDrawingX, LastDrawingY);
    ScrollToLastDrawing();
}

void PlaceDrawing () {
    if (!PlaceRelayDrawing()) {
	ClearRelayGraphics();
        TryToPlaceRelayDrawing();
    }
    InvalidateRelayDrafter();
}

extern bool haveDisassembly;

static void drAPIcommon () {
    if (!haveDisassembly)
        TryToPlaceRelayDrawing();
    InvalidateRelayDrafter();
    ShowWindow(getRelayDrafterHWND(true), SW_SHOWNORMAL);
}

int RelayGraphicsLeftClick(int x, int y) {
    if (RelayGraphicsMouse(0, x, y))
        drAPIcommon();
    return 0;
}

/* Called from miscellaneous UI functions that want to draw a relay-in-hand */
void DrawRelayAPI(Relay* r) {
    DrawRelayFromRelay(r);
    drAPIcommon();
}

/* Called by demo system CIRCUIT command */
void RelayShowString(const char * name) {
    DrawRelayFromName(name);
    drAPIcommon();
}

void DraftsbeingCleanupForOneLayout () {
    CleanUpDrawing();
    if (getRelayDrafterHWND(false)) { //"weak" -- DON'T create it if not there.
        ShowWindow(getRelayDrafterHWND(true), SW_HIDE);
    }
}

void DraftsbeingCleanupForSystem () {
        //nuthin' here.  Mac AppDelegate will release the resource.
}

/* API called by "Draw Relay" command on gamut of NXGObj context menus */
void DrawRelaysForObject(int objNo, const char* objTypeName) {
    std::vector<Relay*>relays(get_relay_array_for_object_number(objNo));
    if (relays.empty())
        MessageBox(0, "Seemingly no relays for this object.", "Relay Dialog Manager", MB_OK);
    else
    {
        Relay * r =   RelayListDialog (objNo, objTypeName, relays, "Draw");
        if (r != NULL)
            DrawRelayAPI(r);
    }
}

/* API called by gamut of NXGObjects for "query relay" context menu command */
void ShowStateRelaysForObject(int object_number, const char * descrip) {
    std::vector<Relay*>relays(get_relay_array_for_object_number(object_number));
    if (relays.empty())
        MessageBox(0, "Seemingly no relays for this object.", "Relay Dialog Manager", MB_OK);
    else
    {
        Relay * r = RelayListDialog (object_number, descrip, relays, "Query");
        if (r != NULL)
            ShowStateRelay(r);
    }
}

/* Called from relay eval loop guts on each pass */
void CheckRelayDisplay () {
    InvalidateRelayDrafter();  // -- nxldwin does exactly this.
}

/* Called by unimplemented commands */
void PrintInterlocking (const char *) {
 
}


/* Called by unimplemented commands */
void DrawInterlockingFromFile (const char *, const char *) {

}

/* Relay menu command */
void AskForAndDrawRelay(void *){
    
    Relay * r = MacRlyDialog ("Draw Relay Circuit");
    if (r != NULL) {
        DrawRelayAPI(r);
    }
}

/* Relay menu command */
void AskForAndShowStateRelay(void *) {
    Relay * r = MacRlyDialog ("Query Relay State");
    if (r != NULL) {
	ShowStateRelay (r);
    }
}

/* For relays.h-free access to relay attributes. */

bool boolRelayState(Relay * r) {
    return (r->State != 0);
}

std::string STLRelayName(Relay * r) {
    return r->RelaySym.u.r->PRep();
}

void vectorizeDependents(Relay * r, std::vector<Relay*>& relays) {
    relays = r->Dependents;
}
