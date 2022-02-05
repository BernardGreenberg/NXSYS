#ifndef _NXSYS_EXTENDED_TRACK_GEOMETRY_H__
#define _NXSYS_EXTENDED_TRACK_GEOMETRY_H__

/* NXSYS Extended Geometry System  1 January 1997 */

#include "nxgo.h"
#ifndef TLEDIT
#ifndef REALLY_NXSYS
#define REALLY_NXSYS
#endif
#endif

class Relay;
class Signal;
class ExitLight;
class TrackSeg;
class Stop;

#ifdef REALLY_NXSYS
class Turnout;
#endif



struct JointOrganizationData {
    double Radang;
    double RRadang;
    double Interang;
    int original_index;
    BOOL SignalExists;
    TrackSeg * TSeg, *OpposingSeg;
};


class TrackJoint
#ifdef TLEDIT
   : public GraphicObject
#endif
{
    public:
	TrackJoint (WP_cord wpx1, WP_cord wpy1);
	~TrackJoint();

	long	Nomenclature;
	BOOL	Insulated;
	BOOL	NumFlip;
#ifdef TLEDIT
	BOOL    Marked;
	BOOL	Organized;
#else
	WP_cord wp_x, wp_y;
	Turnout * TurnOut;
#endif
	short   SwitchAB0;
	TrackSeg *TSA[3];
	int TSCount;
	NXGOLabel *Lab;

	int	AvailablePorts();
	BOOL	AddBranch(TrackSeg * ts);
	void	DelBranch(TrackSeg * ts);
	void	Organize();
	void    GetOrganization(JointOrganizationData*);
	void	PositionLabel();
	BOOL	FindEndIndex (TrackSeg * ts);
	virtual void Display (HDC dc);
	virtual int TypeID ();
	virtual int ObjIDp(long);
#ifdef TLEDIT

	virtual void Select();
	virtual void ShiftLayout2();
	virtual void Cut();
	virtual BOOL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
	virtual UINT DlgId();
	virtual BOOL ClickToSelectP();
	void	MoveToNewWPpos (WP_cord wpx1, WP_cord wpy1);
	void	SwallowOtherJoint (TrackJoint * tj);
	void	TDump (FILE * F, const char * form_head);
	void	EnsureID();
	void	Insulate();
	void	FlipNum();
	int	SignalCount();
	BOOL	SwitchDlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
#endif

#ifdef REALLY_NXSYS
	int	StationNumber();	/* mod off Nomenclature */
#endif
};

class TrackCircuit {		// no longer graphic object
    public:
	BOOL  Occupied, Routed, Coding;
	void* TrackRelay;
	TrackSeg ** SegmentArray;
	int nSegs;
	int SegArraySize;
	long StationNo;
	void AddSeg (TrackSeg*);
	void DeleteSeg (TrackSeg*);
	void SetOccupied (BOOL sta);
	void SetRouted (BOOL sta);
	void Invalidate();
	void ProcessLoadComplete();

	TrackCircuit(long sno);
	~TrackCircuit();
	
#ifdef REALLY_NXSYS
	void ComputeOccupiedFromTrains();
	void ComputeSwitchRoutedState ();
	TrackSeg * FindDemoHitSeg();
	static void TrackReportFcn(BOOL state, void* v);
	static void TrackKRptFcn(BOOL state, void* v);

#endif

	BOOL MultipleSegmentsP();

    private:
	void AddSegToArray(TrackSeg * ts);
	void GrowSegArray();
};


class TrackSegEnd {			//NOT a graphic object
    public:
	RW_cord rwx, rwy;      //meaning of y not so clear
	WP_cord wpx, wpy;      //Windows (all-scrolled-out) Panel coord
#ifdef REALLY_NXSYS
	TrackSeg *Next;   //if there is a switch, "normal" next
	TrackSeg *NextIfSwitchThrown; // "reverse" next, if switch...
	Turnout *FacingSwitch; 
#endif
	Signal * SignalProtectingEntrance; //Train sys uses this for instruc
	short EndIndexNormal, EndIndexReverse;
	ExitLight * ExLight;   // Has to know to redisplay this when lit.
	TrackJoint*Joint;
#ifdef REALLY_NXSYS
	void OffExitLight();
	void SpreadRWFactor (double rwf);
#endif	
	TrackSegEnd();
	BOOL InsulatedP();
	void Reposition();
};


class TrackSeg : public GraphicObject {
    public:
	TrackSegEnd Ends[2];
	TrackCircuit * Circuit;
	float   Length ;  // pythagoric, useful for drawing.
	float   CosTheta, SinTheta;
	BOOL    Routed;// this means "not the deselectd end of a trailing pt"
                      //sw, i.e., to be lit red/white in curr. sw. pos.
#ifdef TLEDIT
	BOOL    Marked;
#else
	Turnout *OwningTurnout;
	float   RWLength;  /* real-world length for train sys */
#endif
	short TrainCount;
	short GraphicBlips;
	TrackSeg (WP_cord wpx1, WP_cord wpy1, WP_cord wpx2, WP_cord wpy2);
	void DisplayInState (HDC dc, int state);
	BOOL SnapIntoLine (WP_cord& wpx, WP_cord& wpy);
	TrackJoint * FindOtherJoint (TrackJoint * tj);
	void Align(WP_cord wpx1, WP_cord wpy1, WP_cord wpx2, WP_cord wpy2);
	void Align();
	void Split(WP_cord wpx1, WP_cord wpy1, TrackJoint* tj);
	~TrackSeg();

	virtual void Display (HDC dc);
	virtual int TypeID ();
	virtual int ObjIDp(long);
	TrackCircuit* SetTrackCircuit (long ID, BOOL wildfire);
	void SetTrackCircuit0 (TrackCircuit * tc);
	void SetTrackCircuitWildfire (TrackCircuit * tc);
	void GetGraphicsCoords (int ex, int& x, int& y);
	virtual BOOL HitP (long x, long y);
	BOOL HasCircuitBrothers();
#ifdef TLEDIT
	char         EndOrientationKey (int whichend);
	virtual void Select();
	virtual void Cut();
	void         SelectMsg();
	virtual BOOL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
	virtual UINT DlgId();
	virtual BOOL ClickToSelectP();
#else
	void SpreadSwitchRoutingState(BOOL p_routed);
	void SpreadRWFactor (double rwf);
	void ProcessLoadComplete();
	void ComputeSwitchRoutedState();
	BOOL ComputeSwitchRoutedEndState(int ex);
	int  StationPointsEnd (WP_cord &wpcordlen, int end_index);
	virtual void EditContextMenu(HMENU m);
#ifndef NOTRAINS
	void SetOccupied (BOOL occupied);
#endif
	virtual void Hit (int mb);
#endif

};

#define TSA_NOTFOUND -1
#define TSA_STEM    0
#define TSA_NORMAL  1
#define TSA_REVERSE 2

class PanelSignal  : public GraphicObject {
    public:    
	PanelSignal(TrackSeg * ts, int end_index, Signal * s, char * text);
	~PanelSignal();

	Signal * Sig;			/* operational logic object */

	TrackSeg * Seg;
	int     EndIndex;

	WP_cord TRelX, TRelY;		/* track-aligned rel to IJ */
	WP_cord Radius;			/* head radius */
	NXGOLabel * Label;		/* labelling info */

	void GetAngles (float &, float &);
	void GetDispPoints (WP_cord &x1, WP_cord &y1,
			    WP_cord &x2, WP_cord &y2,
			    WP_cord &x3, WP_cord &y3);
	void LimWPCoord (WP_cord x, WP_cord y);
	void SetXlkgNo (int xlkgno, BOOL compvisible);
	void PositionLabel (BOOL compvisible);
#ifdef TLEDIT
	char Orientation();

	virtual void Cut();
	virtual BOOL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
	virtual UINT DlgId();
#else
	virtual void EditContextMenu(HMENU m);
	void DrawFleeting(HDC dc, BOOL enabled);
#endif
	virtual void Display (HDC dc);
	void Reposition();
	virtual int TypeID ();
	virtual int ObjIDp(long);
	virtual void ComputeWPRect();
#ifdef TLEDIT
	virtual void Select();
	virtual int Dump (FILE * f);
#else
	virtual void Hit (int mb);
	virtual void UnHit();
#endif

};


class Stop : public GraphicObject{
public:
    Signal     *Sig;
    char	Tripping;
    Relay      *NVP, *RVP, *VPB;
    Stop(Signal * g);

    WP_cord x1, y1, x2, y2, x3, y3;

    int        coding;	       
    int	       code_clicks;

    void       Reposition();
    int        PressStopPB();
    void       StopCoder(BOOL state);
    void       VReporter(BOOL state);
    void       CodingDisplay();
    void       SigWinDisplay (HDC hDC, RECT * rect);
    void       ProcessLoadComplete();


    static  void  VReporterReporter(BOOL state, void* v);
    static  void  StopCoderReporter(void *v, BOOL state);

    virtual void Display (HDC dc);
    virtual int TypeID(), ObjIDp(long);
    virtual BOOL HitP(long, long);

};


#ifdef TLEDIT

class zSignal {
    public:
	zSignal();
	~zSignal();
	PanelSignal * PSignal;
	HBRUSH GetGKBrush();
	short  XlkgNo;
	int StationNo;
	const char * HeadsString;
	Stop * TStop;
	BOOL ExplicitID;
};


#endif

#pragma pack (push,xl)
#pragma pack(1)

/* Same name as V1 exit light, but  quite different. Of course,
   some methods in common. */
#define EXLIGHT_DEFINED 1

class ExitLight : public GraphicObject {
public:
    TrackSeg * Seg;
    int  XlkgNo;
    Relay * XPB;

    unsigned char Lit;
    unsigned char EndIndex;
    unsigned char RedFlash;
    unsigned char Blacking;


public:

    ExitLight (TrackSeg * seg, int EndIndex, int xno);
    ~ExitLight();
    void DisplayExit (HDC hdc, int sw);
    void Reposition();
    void SetLit (BOOL onoff);

    virtual void Display (HDC hdc);
    virtual BOOL HitP (long x, long y);
    virtual int TypeID(), ObjIDp(long);
#ifdef TLEDIT
    virtual int Dump (FILE * f);
    virtual void Select();
    virtual void Cut();
    virtual BOOL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
    virtual UINT DlgId();
#else 
    virtual void Hit (int mb);
    void SetRedFlash (BOOL onoff);
    void ProcessLoadComplete();
    void DrawBlack();
    void Off();
#endif
    static void KReporter(BOOL state, void*);
};

#pragma pack (pop,xl)

TrackSeg * SnapToTrackSeg (WP_cord& wpx, WP_cord& wpy);
extern BOOL ShowNonselectedJoints;

#ifdef REALLY_NXSYS
void TrackCircuitSystemLoadTimeComplete();
void TrackCircuitSystemReInit();
void DecodeDigitated (long input, int &trackno, int &sno);
TrackCircuit * FindTrackCircuit (long sno);
#endif

#endif
