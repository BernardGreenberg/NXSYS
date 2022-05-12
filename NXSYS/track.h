#ifndef _NX_TRACK_H__
#define _NX_TRACK_H__

#include "nxgo.h"

class TrackDef;
class Turnout;
class Relay;
class ReportingRelay;
class ExitLight;
class Signal;

class TrackSec : public GraphicObject {
public:
    BOOL  Occupied, Routed, Coding;
    RW_cord rw_len;
    WP_cord wp_len;
    WP_cord num_xoff, num_yoff;

    TrackSec *North, *South;
    Turnout *Turnouts[4];
    TrackDef *Track;
    ExitLight* ExitLights[4];
    short Station_No;
    short N_Turnouts;
    short N_ExitLights;
    char  NominalSouthp;
    void* Relay;
    Signal* SigNorth, *SigSouth;
    short TrainCount;
    NXGOLabel * SLab;

 static void TrackReportFcn(BOOL state, void*);
 static void TrackKRptFcn(BOOL state, void*);
public:
    TrackSec (RW_cord rwx, RW_cord rwxe, RW_cord rwy,
	      short ijleft, short ijright, TrackDef* trk);
    virtual void Display (HDC hdc);
    virtual void Hit (int mb);
    virtual BOOL HitP (long x, long y);
    virtual int TypeID(), IsNomenclature(long);
    virtual void Invalidate();
    virtual void EditContextMenu(HMENU m);
    void SetOccupied (BOOL sta);
    void SetRouted (BOOL sta);
    void SetCoding (BOOL sta);
    void DisplayNKCoded(HDC dc, RW_cord pt1, RW_cord pt2, BOOL lit,
			BOOL merely_invalidate);
    void RegisterTurnout (Turnout*);
    void RegisterExitLight (ExitLight*);
    void DwarfDiddle();
    HPEN AppropriatePen (Turnout*, BOOL CLKing);
    void InvalidateAndTurnouts();
    int  FacingTrailingClear (int southp);
    void GetLimits (RW_cord& south, RW_cord&north);
    void AdjustSingleton (RW_cord esploc, BOOL northp);

private:
    void DisplaySubSeg (HDC dc, RW_cord scx1, RW_cord scx2, HBRUSH brush);
};

class TrackDef {
public:
    TrackDef (char route, short tkno, RW_cord sbas);
    void Push (TrackSec *ts); 

    char Route;
    BOOL Southp;
    short TrackNo;
    TrackSec *First, *Last;
    RW_cord Station_base;
    RW_cord PanelStationBase;
    RW_cord LastSno;
    RW_cord EndSno;			/* for super graphics system */
    RW_cord BegSno;			/* same like end-sno but no singleton adj. */
    void maketracklabel(long rwx);
};

class TrackLabel: public GraphicObject {
public:
    TrackDef* td;
    TrackLabel (TrackDef * td_, RW_cord rwx, RW_cord rwy);
    virtual int TypeID(), IsNomenclature(long);
    virtual void Display (HDC dc);
};


class Turnout: public GraphicObject {
public:
    TrackSec *TrackSec1, *TrackSec2;
    RW_cord rwx1, rwx2, rwy1, rwy2;
    WP_cord wpx1, wpx2, wpy1, wpy2;
    SC_cord scx1, scx2, scy1, scy2;
    int  XlkgNo;			/* cord 1 is A, 2 is B */
    char Thrown, LockSafe;
    char CLK_Coding;
    Relay *NWP, *RWP, *NWZ, *RWZ;
    Relay *NL, *RL;			/* new 10 December 1994 */

public:
    double TanTheta, SecTheta, SinTheta;
    DWORD MoveStartTime;
    char MovingPhase;
    char CLKCodingPhase;
    char Singleton;
    char AuxKeyForce;

    Turnout (TrackSec * ts1, int sta1, TrackSec * ts2, int sta2, int xno);
    void ThrowSwitch (int throwit);	/* name "Throw" is stolen already */

    void Timer ();
    virtual void Display (HDC hdc);
            void DisplayTurnout (HDC dc, int control);
    virtual void Hit (int mb);
    virtual int TypeID(), IsNomenclature(long);
    virtual BOOL HitP (long x, long y);
    virtual BOOL ComputeVisible (WPRECT& view);
    
    TrackSec * Ts_other (TrackSec* ts);
    RW_cord Ts_rwx (TrackSec* ts);
    WP_cord Ts_wpx (TrackSec* ts);
    SC_cord Ts_scx (TrackSec* ts);
    int Ts_trailingp (TrackSec* ts, int northp);
    void StartMove();
    void CodingFlash (BOOL state);
    void CLKReport (BOOL state);
    void CreatePointLabels();
    void ProcessLoadComplete();
    void DisplayNoDC();
    void InvalidateAndTurnouts();
    void InvalidateNKs();
    void DisplayNKCoded (HDC dc, BOOL lit, BOOL merely_invalidate);
  static void LSReporter(BOOL state, void*);
  static void TimeReporter(void*);
  static void CoderReporter(void* turnout, BOOL codestate);
  static void RWZReporter(BOOL state, void*);
  static void NWZReporter(BOOL state, void*);
  static void CLKReporter(BOOL state, void*);
};

#define TN_AUXKEY_FORCE_NORMAL 1
#define TN_AUXKEY_FORCE_REVERSE 2
#ifndef EXLIGHT_DEFINED
class ExitLight: public GraphicObject {
public:
    TrackSec * TSec;
    int  XlkgNo;
    char Lit;
    Relay * XPB;

public:

    ExitLight (TrackDef* td, RW_cord rwx, RW_cord rwy, int southp, int _xno);
    void DisplayExit (HDC hdc, int sw);
    virtual void Display (HDC hdc);
    virtual void Hit (int mb);
    virtual BOOL HitP (long x, long y);
    virtual int TypeID(), ObjIDp(long);
    void      ProcessLoadComplete();
  static void KReporter(BOOL state, void*);
};
#endif

extern BOOL TorontoStyle;

extern TrackDef *TrackDefs[];
extern int TrackDefCount;
TrackSec *FindContainingTrackSec (char rte, int t1, int s1);

void ReportToRelay (void * r, BOOL state);
void ToggleToRelay (void * r);
void PulseToRelay (void * r);
void PostprocessTurnouts();


WP_cord RWx_to_WPx (TrackDef *td, RW_cord rwx);
WP_cord RWy_to_WPy (TrackDef *td, RW_cord rwy);
WP_cord RWyhun_to_WPy (RW_cord rwy);
extern SC_cord Lit_Seg_HalfWidth, Track_Seg_Len, SectionInterstice,
   Track_Width_Delta;
extern SC_cord GU1, GU2;
Relay * CreateQuislingRelay (long xno, const char * nomenclature);
void ComputeHorizontalScaling (double f);
void SetTrackGeometryDefaults();
typedef void (*RelRptFcn) (BOOL State, void *Obj);
ReportingRelay * CreateAndSetReporter
   (long xno, const char * nomenclature, RelRptFcn f, void * object);
ReportingRelay * ReportingRelayNoCreate (long xno, const char * nomenclature);

#endif
