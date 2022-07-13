
#include <vector>
#include <unordered_map>

#include "windows.h"
#include "nxgo.h"
#include "typeid.h"
#include "brushpen.h"

#include "lyglobal.h"
NXSYSLayoutGlobal Glb;

#define DEFAULT_SIM_FEET_PER_SCREEN 3000.00

SC_cord GU1, GU2,
      Track_Width_Delta, Track_Side_Fudge, Track_Side_Nolight, Track_Seg_Len,
      Track_Normal_Halfwidth, Lit_Seg_HalfWidth, SectionInterstice,
      Unlit_Half_Len, RWXO, RWYO;

int   RWY_factor;
int MainWindowWidth;

void SetTrackGeometryDefaults ();

HPEN YellowXPen, FleetGreenPen, FleetBlackPen, NullPen;

HBRUSH TrackDftBrush, TrackOccupiedBrush, TrackRoutedBrush,
    GKRedBrush, GKYellowBrush, GKOffBrush, GKGreenBrush, GKWhiteBrush,
    GKHalfRedBrush;
HPEN TrackPen, SigPen, BgPen, HighlightTrackPen,
  TrackDftPen, TrackOccupiedPen, TrackRoutedPen, TrackDftPenThrown,
  PlatformPen,
#if TLEDIT
  SelectedExitLightPen,
#endif

  RedExitLightPen,
  BlackExitLightPen,
  ExitLightPen;

COLORREF TrackDftCol = RGB(128,128,128),
    TrackOccupiedCol = RGB (255,0,0),
    TrackRoutedCol = RGB (255,255,255);

#if NXSYSMac
static void InitStockObjects();
#else
static void InitStockObjects(){};
#endif

struct GDIEnt {
    static std::vector<GDIEnt> Objects;

    HGDIOBJ Handle;
    bool IsPen;
    COLORREF Color;
    int param1, param2;

    bool equal (bool b, COLORREF c, int pm1, int pm2) const {
	return ((b == IsPen) && (c == Color) && (pm1 == param1) && (pm2 == param2));}
    static void Reset () {
        Objects.clear();
    }
    static GDIEnt * FindEntry (bool b, COLORREF c, int ax, int ay) {
	for (auto& e : Objects)
	    if (e.equal(b, c, ax, ay))
                return &e;
	return NULL;
    }
    static HGDIOBJ Find (bool b, COLORREF c, int ax, int ay) {
        GDIEnt * pE = FindEntry(b, c, ax, ay);
        if (pE == NULL)
            return NULL;
        return pE->Handle;
    }
    GDIEnt(HGDIOBJ h, bool isPen, COLORREF color, int pm1, int pm2) {
#if NXSYSMac
        h = (void*)(long)(Objects.size()+1);
#endif
        Handle = h;
        IsPen = isPen;
        Color = color;
        param1 = pm1;
        param2 = pm2;
    }
    ~GDIEnt() {DeleteObject(Handle);}
    
    void RetrieveValues(bool& isPen, COLORREF& color, int& pm1, int&pm2) {
        isPen = IsPen;
        color = Color;
        pm1 = param1;
        pm2 = param2;
    }

    static GDIEnt * Push (HGDIOBJ h, bool b, COLORREF c, int pm1, int pm2) {
        Objects.emplace_back(h,b,c,pm1,pm2);
        return &Objects.back();
    }
};

std::vector<GDIEnt>GDIEnt::Objects;


HPEN CreateSPen (int width, COLORREF color) {
#if _WIN32
    return CreatePen(PS_SOLID, width, color);
#endif
    /* 3/21/2022 -- It is not currently known why the code below fails on Windows.
     Tracks don't get drawn.  The kludge of just bypassing the whole thing, and calling
     CreateSoldBrush and CreatePen directly, is acceptable because these objects are
     NOT allocated on the fly, but cached into more specific variables whose lifetime
     is that of the app invocation, e.g., TrackPen, HighlightPen, etc.
     
     */
    GDIEnt * pE = GDIEnt::FindEntry(true, color, PS_SOLID, width);
    if (!pE) {
	HPEN p = CreatePen (PS_SOLID, width, color);
	pE = GDIEnt::Push (p, true, color, PS_SOLID, width);
    }
    return (HPEN)pE->Handle;
}

HBRUSH CreateSBrush (COLORREF color) {
#if _WIN32
    return CreateSolidBrush(color);
#endif
  GDIEnt * pE = GDIEnt::FindEntry (false, color, 0, 0);
    if (!pE) {
	HBRUSH b = CreateSolidBrush(color);
	pE = GDIEnt::Push (b, false, color, 0, 0);
    }
    return (HBRUSH)pE->Handle;
}

void InitTrackGDI (int mww, int mwh) {
    MainWindowWidth = mww;
    TrackDftBrush = CreateSBrush (TrackDftCol);
    TrackOccupiedBrush = CreateSBrush (TrackOccupiedCol);
    TrackRoutedBrush = CreateSBrush(TrackRoutedCol);
    GKRedBrush = CreateSBrush (RGB(255, 0, 0));
    GKHalfRedBrush = CreateSBrush (RGB(128, 0, 0));
    GKYellowBrush = CreateSBrush (RGB(255, 192,0));
    GKGreenBrush = CreateSBrush (RGB(0,255,0));
    GKOffBrush = CreateSBrush (RGB(128,128,128));
    GKWhiteBrush = CreateSBrush (RGB(255,255,255));

    GU1= mwh/300;
    Track_Normal_Halfwidth = GU1;
    GU2 = mwh/200;
    Lit_Seg_HalfWidth = GU2;
    Track_Width_Delta = (int)(GU2*1.5);
    Track_Side_Fudge = 0;
    Track_Side_Nolight =0;
    Track_Seg_Len = 4*GU2;
    Unlit_Half_Len = GU2;
    RWY_factor = mww/25;
    RWYO = mww/25;
    RWXO = mwh/24;
    SetTrackGeometryDefaults();

    SectionInterstice = mwh/400;
    SigPen = CreateSPen (mwh/400, RGB(255,255,255));
    BgPen = CreateSPen (mwh/400, RGB(0,0,0));
    TrackPen = CreateSPen (2*Track_Normal_Halfwidth,RGB(128,128,128));
    HighlightTrackPen = CreateSPen (2*Track_Normal_Halfwidth,RGB(0,255,0));
    ExitLightPen = CreateSPen (3*Track_Normal_Halfwidth,RGB(255,255,255));
    BlackExitLightPen = CreateSPen (3*Track_Normal_Halfwidth,RGB(0,0,0));
#if TLEDIT
    SelectedExitLightPen = CreateSPen (3*Track_Normal_Halfwidth,RGB(0,255,0));
#endif

    RedExitLightPen = CreateSPen (3*Track_Normal_Halfwidth,RGB(255,0,0));

    int w = (int)(1.2*GU2);

    YellowXPen = CreateSPen (GU2, RGB (255, 255, 0));
    FleetGreenPen = CreateSPen (GU2/3, RGB(0,255,0));
    FleetBlackPen = CreateSPen (GU2/3, RGB(0,0,0));
    TrackDftPen = CreateSPen (w, TrackDftCol);
    TrackDftPenThrown = CreateSPen (w, RGB(0,0,255));
    TrackOccupiedPen = CreateSPen (w, TrackOccupiedCol);
    TrackRoutedPen = CreateSPen (w, TrackRoutedCol);
    PlatformPen = CreateSPen (2, TrackDftCol);
    NullPen = (HPEN)GDIEnt::Find (true, RGB(0,0,0), PS_NULL, w);
    if (!NullPen) {
	NullPen = CreatePen (PS_NULL, w, RGB(0,0,0));
	GDIEnt::Push (NullPen, true, RGB(0,0,0), PS_NULL, w);
    }
    InitStockObjects(); // only non-noop on Macintosh
}

void ComputeHorizontalScaling (double f) {
    Glb.PixelsPerSimFoot = ((double)MainWindowWidth)/f;
}

void SetTrackGeometryDefaults () {
    ComputeHorizontalScaling(DEFAULT_SIM_FEET_PER_SCREEN);
}

void TrackGraphicsCleanup() {
  GDIEnt::Reset();
}

#if NXSYSMac
BOOL RetrieveGDIWrapperInfo(HGDIOBJ gh, bool& isPen, COLORREF& color, int& param1, int& param2) {
    long index = (long)gh;
    assert (index >= 1 && index <= GDIEnt::Objects.size());
    GDIEnt::Objects[index - 1].RetrieveValues(isPen, color, param1, param2);
    return TRUE;
}

static std::unordered_map<HGDIOBJ, HGDIOBJ> StockObjects;

static void InitStockObjects() {
    StockObjects[BLACK_BRUSH] = CreateSBrush(RGB(0,0,0));
    StockObjects[WHITE_BRUSH] = CreateSBrush(RGB(255,255,255));
    StockObjects[LTGRAY_BRUSH] = CreateSBrush(RGB(200,200,200));   //+++ learn by experience what the right value ought be
    StockObjects[BLACK_PEN] = CreateSPen(2, RGB(0,0,0));
    NullPen = StockObjects[NULL_PEN] = CreateSPen(20, RGB(7,7,7));
}

/* Implement Windows API "GetStockObject" right here. */
HGDIOBJ GetStockObject(HGDIOBJ moniker) {
    long monica = (long)moniker;
    /* input -- a number between 1001 and 1004 inclusive */
    /* output (if not null) a GDI handle which is 1+ an index into table */
    if (!(monica > BRUSH_PEN_FIRST && monica < BRUSH_PEN_END))
        return NULL;

    auto it = StockObjects.find(moniker);
    if (it != StockObjects.end())
        return it->second;
    else {
        MessageBox(NULL, "Can't find stock object", "Track GDI", MB_ICONSTOP |MB_OK);
        return NULL;
    }
}
#endif
