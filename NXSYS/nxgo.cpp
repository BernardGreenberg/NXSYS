#include "windows.h"
#include <string.h>
#include <math.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <cassert>
#include "NXSYSMinMax.h"

#include "nxgo.h"

#ifdef TLEDIT
#define INVALIDATE_CLEAR_BKGD 1
#else
#define INVALIDATE_CLEAR_BKGD 0
#endif

/* Global defined in this program */
GraphicObject *SelectedObject = NULL;
WORD NXGOHitX, NXGOHitY;
BOOL NXGODeleteAll = FALSE;
double NXGO_Scale = 1.0;
static const int QUAD = 50;

/* global which damned -should- be defined in this program */
extern HWND G_mainwindow;
extern HFONT Fnt;

#if TLEDIT  // we'll need inserts and deletes
static std::unordered_set<GraphicObject*>AllObjects;
static std::unordered_set<GraphicObject*>VisibleObjects;
#else   // otherwise, order of addition and display actually matters.
static std::vector<GraphicObject*>AllObjects;
static std::vector<GraphicObject*>VisibleObjects;
#endif

static std::unordered_map<unsigned int, std::vector<GraphicObject*>> Quads;


static long VPSCWidth, VPSCHeight;
static WPRECT Viewport = {0, 0, 0, 0};
static long ScaleNum = 100, ScaleDen = 100;
static tNXGOSelectHook SelectHook = NULL;
static GraphicObject *MouseUpObject = NULL;
static WPRECT LRect = {0, 0, 0, 0};

static void GetScrollSlops (WP_cord& xslop, WP_cord&yslop);

int GraphicObjectCount() {
    return (int)AllObjects.size();
}

static inline unsigned int QuadKey (long i, long j) {
    return (unsigned int)((i << 16) | j);
}

BOOL GraphicObject::ComputeVisible (WPRECT& view) {
    if (view.right < (wp_x+ wp_limits.left)
	|| view.left > (wp_x + wp_limits.right)
	|| view.top > (wp_y + wp_limits.bottom)
	|| view.bottom < (wp_y + wp_limits.top))
#ifdef NXSYSMac
    // "Credo in unam fenestram,"
   // "Factorem omnium visibilium et invisibilium"
/* What's going on here -- deep six the entire visibility system because that's really the NSScrollView's
   job to manage.  He'll probably nix the Mac GDI calls, although we can optimize better by either observing
   our own "dirty" clip rectangles and/or monitoring the scroll position "document view rectangle" to reinstate
   NXSYS clip management. But this may be enough; it seems to work with no extended window, with the view
   set to the right LayoutRectangle by the call to "compute real layout dims." I hope this solves all problems.
 */
       
        ;
#else
        return Visible = FALSE;
#endif
    sc_limits.left = WPXtoSC(wp_x + wp_limits.left);
    sc_limits.right = WPXtoSC(wp_x + wp_limits.right);
    sc_limits.top = WPYtoSC(wp_y + wp_limits.top);
    sc_limits.bottom = WPYtoSC(wp_y + wp_limits.bottom);
    sc_x = WPXtoSC (wp_x);
    sc_y = WPYtoSC (wp_y);
    return Visible = TRUE;
}

BOOL GraphicObject::HitP (long x,  long y) {
    return x >= sc_limits.left
	    && x < sc_limits.right
	    && y >= sc_limits.top
	    && y < sc_limits.bottom;
}

void GraphicObject::Invalidate () {
    InvalidateRect (G_mainwindow, &sc_limits, INVALIDATE_CLEAR_BKGD);
}


void GraphicObject::UnHide (){
    //    if (!Visible) {
#if TLEDIT
    VisibleObjects.emplace(this);
#else
    VisibleObjects.push_back(this);
#endif
    Visible = TRUE;
    Invalidate();
    //    }
}


void GraphicObject::ContributeToLayoutRect() {
    WP_cord x0 = wp_x + wp_limits.left;
    WP_cord x1 = wp_x + wp_limits.right;
    WP_cord y0 = wp_y + wp_limits.top;
    WP_cord y1 = wp_y + wp_limits.bottom;
    LRect.left = NXMIN(LRect.left, x0);
    LRect.right = NXMAX(LRect.right, x1);
    LRect.top = NXMIN(LRect.top, y0);
    LRect.bottom = NXMAX(LRect.bottom, y1);
    // New lookup system 9/2/2019, RealNXSYS only - TLEdit moves, creates, and deletes objects.
#if ! TLEDIT
    if (MouseSensitive())
        for (WP_cord i = x0/QUAD; i <= (x1 + 1)/QUAD; i++)
            for (WP_cord j = y0/QUAD; j <= (y1 + 1)/QUAD; j++)
                Quads[QuadKey(i, j)].push_back(this);
#endif
}

void ComputeVisibleObjects (WPRECT& view) {
    Viewport = view;
    VisibleObjects.clear();
    LRect.top = LRect.left = LRect.bottom = LRect.right = 0;
    for (GraphicObject *go : AllObjects) {
        go->ContributeToLayoutRect();
        if (go->ComputeVisible (view))
            go->UnHide();
        else go->Visible = FALSE;
    }
    NXGO_SetScrollPosition(G_mainwindow);

#if REPORT_QUADMAP_DISTRIBUTION
    if (Quads.empty())  // happens during initialization
        return;
    std::map<size_t, int> M;    //ordered map.
    for (auto& pair : Quads) {
        size_t len = pair.second.size();
        if (M.count(len))
            M[len] += 1;
        else
            M[len] = 1;
    }
    int sensitive = 0;
    for (auto obj : AllObjects)
        if (obj->MouseSensitive())
            sensitive += 1;
    for (auto& e : M)
        printf("%2ld: %2d\n", e.first, e.second);
    printf("%ld nonempty quads, %ld graphical objects, %d sensitive.\n",
           Quads.size(), AllObjects.size(), sensitive);
#endif
}

WP_cord SCXtoWP (SC_cord x) {
    return (WP_cord)((x * ScaleDen)/ScaleNum)+Viewport.left;
}

WP_cord SCYtoWP (SC_cord y) {
    return (WP_cord)((y * ScaleDen)/ScaleNum)+Viewport.top;
}

SC_cord WPXtoSC (WP_cord x) {
    return (SC_cord)(((x - Viewport.left)* ScaleNum) /ScaleDen);
}

SC_cord WPYtoSC (WP_cord y) {
    return (SC_cord)(((y - Viewport.top) * ScaleNum) /ScaleDen);
}


void ComputeVisibleObjectsLast() {
    ComputeVisibleObjects (Viewport);
}

void GraphicObject::ComputeVisibleLast() {
    ComputeVisible(Viewport);
}

void GraphicObject::MakeSelfVisible () {
    ComputeVisibleLast();
    UnHide();
}

void ComputeWindowPos () {
    for (auto objp : AllObjects)
	objp->ComputeWP();
}

void GraphicObject::ComputeWP() {};

void DisplayVisibleObjects (HDC dc) {
    for (auto objp : VisibleObjects)
        objp->Display(dc);
}

void DisplayVisibleObjectsRect (HDC dc, RECT &ur) {
    for (GraphicObject * g : VisibleObjects) {
        if (ur.left > g->sc_limits.right)
            continue;
        if (ur.right < g->sc_limits.left)
            continue;
        if (ur.top > g->sc_limits.bottom)
            continue;
        if (ur.bottom < g->sc_limits.top)
            continue;
        g->Display(dc);
    }
}

GraphicObject::GraphicObject () {
    Selected = FALSE;
    Visible = FALSE;
#if TLEDIT
    AllObjects.insert(this);
#else
    AllObjects.push_back(this);
#endif
}

void GraphicObject::Consume() {
    /* here to fake out flow analyzer, who doesn't know that the above
     constructor stores a reference to the object */
}

bool GraphicObject::MouseSensitive() {
    return true;
}                                          /* default "yes" */

void GraphicObject::Hit (int /*mh*/) {
}

void GraphicObject::UnHit () {
}

bool GraphicObject::IsNomenclature(long) {
    return 0;
}

TypeId GraphicObject::TypeID() {
    return TypeId::NONE;
}

GraphicObject * GetMouseHitObject (WORD x, WORD y) {
    if (Quads.size()) {
        auto key = QuadKey(SCXtoWP(x)/QUAD, SCYtoWP(y)/QUAD);
        if (Quads.count(key)) {
            for (GraphicObject* g : Quads[key]) {
                if (g->HitP(x, y))  // only Mouse-sensitive objects are in Quads.
                    return g;
            }
        }
        return NULL;
    }
    for (auto objp : VisibleObjects)
        if (objp->MouseSensitive() && objp->HitP(x, y))
            return objp;
    return NULL;
}

// Default handler if not redefined.
void GraphicObject::HitXY (WORD x, WORD y, WORD message) {
    NXGOHitX = x;
    NXGOHitY = y;
    Hit(message);
}

/* in all of these, "message" is a WM_ windows message, or one of NXGO's
   private WM_NXGO_ messages that include shift/control key modifiers */
void NXGO_Rodentate (WORD x, WORD y, WORD message) {
    GraphicObject * g = GetMouseHitObject (x, y);
    if (g && g->MouseSensitive())
        g->HitXY(x, y, message);
}

GraphicObject * MapAllGraphicObjects (GOGOMapperFcn fn, void * arg) {
    for (GraphicObject * g : AllObjects)
        if (fn (g, arg))
	    return g;
    return NULL;
}

GraphicObject * MapAllVisibleGraphicObjects (GOGOMapperFcn fn, void * arg) {
    for (GraphicObject * g : VisibleObjects)
        if (fn (g, arg))
            return g;
    return NULL;
}

int MapGraphicObjectsOfType (TypeId type, GOMapperFcn fn) {
    for (GraphicObject * g : AllObjects)
        if (g->TypeID() == type)
	    if (fn (g))
		return 1;
    return 0;
}

GraphicObject* MapFindGraphicObjectsOfType (TypeId type, GOGOMapperFcn fn, void* arg) {
       for (GraphicObject * g : AllObjects)
           if (g->TypeID() == type)
               if (fn (g, arg))
                   return g;
    return NULL;
}

void GraphicObject::Hide() {
#if REALLY_NXSYS
    assert(!"GraphicObject::Hide called in real NXSYS");
#else
    if (Visible) {
        Invalidate();
        InvalidateRect (G_mainwindow, &sc_limits, 1);
    }
    VisibleObjects.erase(this);
    Visible = FALSE;
#endif
}

void GraphicObject::GetVisible () {
#if TLEDIT
    ComputeVisibleLast();
    if (Visible) {
       if (!VisibleObjects.count(this))
            UnHide();
    }
#else
    assert(!"GraphicObject::GetVisible in real NXSYS; no container search.");
#endif
}


void GraphicObject::MoveWP (WP_cord x, WP_cord y) {
    /* 5/13/2022 - very important for null property changes in new TLEdit undo system*/
    if (x == wp_x && y == wp_y)
        return;

    BOOL wasv = Visible;
    if (wasv)
	Hide();
    wp_x = x;
    wp_y = y;
    ComputeWPRect();
    /* this has  to be done more efficiently  +++++++++++++++++*/
    ComputeVisible (Viewport);
    if (wasv)
	UnHide();
}


void GraphicObject::MoveSC (SC_cord scx, SC_cord scy) {
    MoveWP (SCXtoWP (scx), SCYtoWP (scy));
}

void GraphicObject::ComputeWPRect() {
}

static void Deselect0() {
    SelectedObject = NULL;
    if (SelectHook)
	SelectHook(NULL);
}

GraphicObject::~GraphicObject() {

    if (this == SelectedObject)
	Deselect0();
    if (this == MouseUpObject)
	MouseUpObject = NULL;
#if TLEDIT
    if (!NXGODeleteAll) {
        Hide();
        if (AllObjects.count(this))
            AllObjects.erase(this);
    }
#endif
}

void FreeGraphicObjects () {
    Quads.clear();
    SelectedObject = NULL;
    MouseUpObject = NULL;
    NXGODeleteAll = TRUE;
    decltype(AllObjects) all_copy = AllObjects; // don't delete while copying!
    for (GraphicObject * g : all_copy)
        delete g;
    all_copy.clear();
    NXGODeleteAll = FALSE;
    AllObjects.clear();
    VisibleObjects.clear();
}

GraphicObject * FindObjectByTypeAndWPpos(TypeId type, WP_cord wp_x, WP_cord wp_y) {
    for (GraphicObject * g : AllObjects) {
        if (g->TypeID() == type)
            if (g->wp_x == wp_x && g->wp_y == wp_y)
                return g;
    }
    return NULL;
}

GraphicObject * FindHitObject (long nomenclature, TypeId type) {
    for (GraphicObject * g : AllObjects) {
	if (g->TypeID() == type)
	    if (g->IsNomenclature (nomenclature))
		return g;
    }
    return NULL;
}


GraphicObject * FindHitObjectOfType (TypeId type, WORD x, WORD y) {
    for (GraphicObject *g : VisibleObjects)
	if (g->TypeID () == type)
	    if (g->HitP((long)x, (long)y))
		return g;
    return NULL;
}

GraphicObject * FindHitObjectOfTypes (TypeId *keys, int nkeys, WORD x, WORD y){
    long lx = (long) x;
    long ly = (long) y;
    for (GraphicObject *g : VisibleObjects) {
	TypeId type = g->TypeID();
	for (int j = 0; j < nkeys; j++)
	    if (keys[j] == type)
		if (g->HitP(lx, ly))
		    return g;
    }
    return NULL;
}

/* called by demo system */
int GraphicObject::FindHitGo (SC_cord&x, SC_cord&y, int buttons) {
    x = (sc_limits.left+sc_limits.right)/2;
    y = (sc_limits.top+sc_limits.bottom)/2;
    if (MouseSensitive()) {
        HitXY(x, y, buttons);
        NXGOMouseUp();			/* oops */
        return 1;
    }
    else
        return 0;
}

void GraphicObject::Select () {
    if (SelectedObject)
	SelectedObject->Deselect();
    Selected = TRUE;
    SelectedObject = this;
    Invalidate();
    if (SelectHook)
	SelectHook(this);
}

void GraphicObject::Deselect () {
    Selected = FALSE;
    Invalidate();
    Deselect0();
}

void NXGO_SetSelectHook (tNXGOSelectHook h) {
    SelectHook = h;
}

void GraphicObject::WantMouseUp() {
    MouseUpObject = this;
}

void NXGOMouseUp() {
    if (MouseUpObject) {
	GraphicObject * g = MouseUpObject;
	MouseUpObject = NULL;
	g->UnHit();
    }
}


void NXGOMouseMove (WORD x, WORD y) {
    if (MouseUpObject && !MouseUpObject->HitP ((long) x, (long) y))
	NXGOMouseUp();
}




void NXGO_SetViewportDims (int width, int height) {
    VPSCWidth = width;
    VPSCHeight = height;
//    Viewport.left = Viewport.top = 0;
    Viewport.right = Viewport.left + (long)(width / NXGO_Scale);
    Viewport.bottom = Viewport.top + (long)(height / NXGO_Scale);
    ComputeVisibleObjectsLast();
}

void
NXGO_HScroll (HWND window, int code, int pos) {
    WP_cord xslop, yslop;
    GetScrollSlops (xslop, yslop);
    WP_cord vpwidth = Viewport.right - Viewport.left;
    WP_cord inc;
    WP_cord lineinc = vpwidth/20;
    if (code == SB_PAGELEFT || code == SB_PAGERIGHT)
	inc = (4*vpwidth)/5;
    else
	inc = lineinc;

    switch (code) {
	case SB_TOP:
	    if (Viewport.left <= LRect.left)
		return;
	    Viewport.left = LRect.left;
	    break;
	case SB_BOTTOM:
	    if (Viewport.left >= xslop + LRect.left)
		return;
	    Viewport.left = xslop + LRect.left;
	    break;
	case SB_THUMBPOSITION:
	    if (xslop < 0)
		Viewport.left = 0;
	    else
		Viewport.left = (WP_cord)(((long)xslop * pos)/100)+LRect.left;
	    break;
	case SB_PAGELEFT:
	case SB_LINELEFT:
	    if (Viewport.left <= LRect.left)
		return;

	    if (Viewport.left - inc < LRect.left)
		Viewport.left = LRect.left;
	    else
		Viewport.left -= inc;

	    break;
	case SB_PAGERIGHT:
	case SB_LINERIGHT:
#ifndef TLEDIT
	    if (Viewport.right >= LRect.right)
		return;
#endif
	    Viewport.left += inc;
#ifndef TLEDIT
            if (Viewport.left + vpwidth > LRect.right) {
		if (xslop < 0)
		    Viewport.left = LRect.left;
	    else
		Viewport.left = xslop + LRect.left;
            }
#endif
	    break;
	default:
	    return;
    }
    Viewport.right = Viewport.left + vpwidth;
    ComputeVisibleObjects(Viewport);
    InvalidateRect (window, NULL, 1);
}



void
NXGO_VScroll (HWND window, int code, int pos) {
    WP_cord xslop, yslop;
    GetScrollSlops (xslop, yslop);
    WP_cord vpheight = Viewport.bottom - Viewport.top;
    WP_cord inc;
    WP_cord lineinc =vpheight/20;
    if (code == SB_PAGEUP || code == SB_PAGEDOWN)
	inc = (4*vpheight)/5;
    else
	inc = lineinc;
    WP_cord lheight = LRect.bottom - LRect.top + lineinc;
	    
    WP_cord bottom_limit_top = (lheight < vpheight) ? LRect.top
			       : lheight - vpheight;
    switch (code) {
	case SB_TOP:
	    if (Viewport.top == LRect.top)
		return;
	    Viewport.top = LRect.top;
	    break;
	case SB_BOTTOM:
	    if (Viewport.top == bottom_limit_top)
		return;
	    Viewport.top = bottom_limit_top;
	    break;
	case SB_LINEUP:
	case SB_PAGEUP:
	    if (Viewport.top == LRect.top)
		return;
	    if (Viewport.top < inc)
		Viewport.top = LRect.top;
	    else
		Viewport.top -= inc;
	    break;
	case SB_LINEDOWN:
	case SB_PAGEDOWN:
#ifndef TLEDIT
	    if (Viewport.top >= bottom_limit_top)
		return;
#endif
	    Viewport.top += inc;
#ifndef TLEDIT
	    if (Viewport.top >= bottom_limit_top)
		Viewport.top = bottom_limit_top;
#endif
	    break;
	case SB_THUMBPOSITION:
	    if (yslop < 0)
		Viewport.top = LRect.top;
	    else
		Viewport.top = (WP_cord)(((long)yslop * pos)/100);
	    break;
	default:
	    return;
    }
    Viewport.bottom = Viewport.top + vpheight;
    ComputeVisibleObjects(Viewport);
    InvalidateRect (window, NULL, 1);
}

void NXGO_SetDisplayScale (double s) {
    NXGO_Scale = s;
    ScaleNum = (long)(s*100.0);
    ScaleDen = 100;
    Viewport.right = Viewport.left + (long)(VPSCWidth/s);
    Viewport.bottom = Viewport.top + (long)(VPSCHeight/s);
    ComputeVisibleObjectsLast();
}

double NXGO_GetDisplayScale() {
    return double(ScaleNum)/double(ScaleDen);
}

void NXGO_SetDisplayWPOrg (WP_cord x, WP_cord y) {
    WP_cord width = Viewport.right-Viewport.left;
    WP_cord height = Viewport.bottom-Viewport.top;
    Viewport.left = x;
    Viewport.right = x + width;
    Viewport.top = y;
    Viewport.bottom = y + height;
    ComputeVisibleObjectsLast();
    InvalidateRect (G_mainwindow, NULL, 1);
}

void NXGO_GetDisplayWPOrg (WP_cord &x, WP_cord&y) {
    x = Viewport.left;
    y = Viewport.top;
}

static void GetScrollSlops (WP_cord& xslop, WP_cord&yslop) {
    WP_cord image_width = LRect.right - LRect.left;
    WP_cord image_height = LRect.bottom - LRect.top;
    WP_cord viewport_width = Viewport.right - Viewport.left;
    WP_cord viewport_height = Viewport.bottom - Viewport.top;
    xslop = image_width - viewport_width;
    yslop = image_height - viewport_height;
}


void NXGO_SetScrollPosition(HWND hWnd) {
    /* assume LRect and viewport meaningful*/
    WP_cord xslop, yslop;
    GetScrollSlops (xslop, yslop);
    int xscrollpos, yscrollpos;
    if (xslop <= 0)
	if (LRect.right <= LRect.left)
	    xscrollpos = 0;
	else
	    xscrollpos = (int)((long)Viewport.left *100L)/
			 (LRect.right - LRect.left);
    else if (Viewport.left > xslop)
	xscrollpos = 100;
    else 
	xscrollpos = (int)(((long)(Viewport.left - LRect.left)* 100L)/xslop);
    if (yslop <= 0)
	if (LRect.bottom <= LRect.top)
	    yscrollpos = 0;
	else
	    yscrollpos = (int)((long)Viewport.top *100L)/
			 (LRect.bottom - LRect.top);
    else if (Viewport.top > yslop)
	yscrollpos = 100;
    else 
	yscrollpos = (int)(((long)Viewport.top * 100L)/yslop);
    SetScrollPos (hWnd, SB_HORZ, xscrollpos, TRUE);
    SetScrollPos (hWnd, SB_VERT, yscrollpos, TRUE);
}

void NXGO_ValidateWpVp(HWND hwnd) {
    WP_cord xslop, yslop;
    WP_cord width = Viewport.right-Viewport.left;
    GetScrollSlops (xslop, yslop);
    if (Viewport.left < LRect.left) {
	Viewport.left = LRect.left;
	Viewport.right = Viewport.left + width;
rcg:
	ComputeVisibleObjects(Viewport);
	InvalidateRect (hwnd, NULL, 0);
    }
    else if (Viewport.right > LRect.right) {
	if (xslop > 0) {
	    Viewport.right = LRect.right;
	    Viewport.left = Viewport.right - width;
	    goto rcg;
	}
    }
}

void NXGOExtractWPCoords(void* vg, long& x, long& y) {
    GraphicObject* g = (GraphicObject*)vg;
    x = g->wp_x;
    y = g->wp_y;
}

/* This just duplicates LRect. Figure out how to use that instead. */
RECT NXGO_ComputeTrueLayoutDimensions() {

    WP_cord lowest_x = 128;
    WP_cord lowest_y = 128;
    WP_cord highest_y = 128;
    WP_cord highest_x = 128;
    
    for (GraphicObject * g : AllObjects) {
        WP_cord x = g->wp_x;
        WP_cord y = g->wp_y;
        WP_cord left = x + g->wp_limits.left;
        WP_cord right = x + g->wp_limits.right;
        WP_cord top = y + g->wp_limits.top;
        WP_cord bottom = y + g->wp_limits.bottom;
        if (top < lowest_y)
            lowest_y = top;
        if (bottom > highest_y)
            highest_y = bottom;
        if (left < lowest_x)
            lowest_x = left;
        if (right > highest_x)
            highest_x = right;
    }
    RECT answer;
    answer.left = 0; //(int) lowest_x;
    answer.right = (int) highest_x;
    answer.top = 0; //(int) lowest_y;  // @#$@$ This negative #!@# (don't care if it's not at origin, we origin.
    answer.bottom = (int) highest_y;
    //printf ("[%d, %d]->[%d,%d]\n", answer.left, answer.top, answer.right, answer.bottom);
    return answer;
}
