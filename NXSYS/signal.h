#ifndef _NX_SYS_SIGNAL_H__
#define _NX_SYS_SIGNAL_H__

#include "nxgo.h"

#ifdef XTG
#define _SIG_virtual
#else
#define _SIG_virtual virtual
#endif

#include <string>
#include <vector>

class TrackSec;
class Relay;
class Stop;
class PanelSignal;
class Signal;

const int SIGF_CO =               0x001,
          SIGF_LUNAR =            0x002, /* actual control of light */
	  SIGF_S     =            0x004,
	  SIGF_D     =		  0x008,
	  SIGF_LUNAR_WHEN_RED =   0x010, /* must be and'ed with "red" */
	  SIGF_STR   =            0x020, /* must be and'ed with "red" */

	  SIGF_CODING =           0x100,
	  SIGF_HARDWIRE_COCLK =   0x200;


class SigHead {
public:
    char State;
    std::string Lights;
    std::string Plate;
    short height;
    SigHead (const std::string& lights, const std::string& plate);
    int	    DisplayWin (Signal*S, int xc, int y, HDC dc, int lights_only, int stp);
    int	    UnitHeight();
};
 

typedef std::vector<const std::string> HeadsArray;
class Signal
#ifndef XTG
   : public GraphicObject
#endif
{
public:
    std::vector<SigHead> Heads;
#ifdef XTG
    PanelSignal * PSignal;
#else
    TrackSec	*ForwardTS;
    short	Southp;
#endif
    int		StationNo;
    int		RealStationPos;
    short	XlkgNo;
    char	Selected;
    char	Coding;
    SC_cord	NumW;
    char	Fleeted;
    HWND	Window;

    char	HG, HVG, DG, DivG;
    short	MiscG;
    Relay 	*PB, *PBS, *FL, *COPB;
    void       *TrainLooking;
    Stop       *TStop;

    
#ifdef TLEDIT
    std::string HeadsString;
    Signal(int xlno, int stano, const char* headinfo);
    ~Signal();
    BOOL ExplicitID;
#endif

    Signal (int xno, int sno, HeadsArray& headarray);

    static void HReporter(BOOL state, void*);
    static void DReporter(BOOL state, void*);
    static void DivReporter(BOOL state, void* v);
    static void RReporter(BOOL state, void* v);
    static void SKReporter(BOOL state, void* v);
    static void DKReporter(BOOL state, void* v);
    static void COReporter(BOOL state, void* v);
    static void GKCoderReporter (void* v, BOOL state);
    static void CLKReporter(BOOL state, void *v);
    static void LunarReporter (BOOL state, void * v);
    static void LunarWhenRedReporter (BOOL state, void * v);
    static void STRReporter (BOOL state, void * v);
    static void SigStateExt(Signal * s);
           void FlagReporterCommon (unsigned int flag, BOOL state);

    _SIG_virtual void Display (HDC dc);
    _SIG_virtual void Invalidate();
    _SIG_virtual void Hit (int mb);
    _SIG_virtual void UnHit();
    _SIG_virtual int TypeID(), ObjIDp(long);

    void	Hook(void*v) {TrainLooking = v;};
    char	ComputeState();
    void	WinDisp (HDC dc, int lights_only);
    void	DisplayStop (HDC dc);
    void	UpdateStop();
    void	UpdateLights();
    void	GKCoder(BOOL state);
    void	ReportToHook();
    void	GetPlateData (std::string& routid, int& trkno, int& stano);
    HBRUSH	GetGKBrush();
    void	ProcessLoadComplete();
    BOOL	HomeP();
    long	CanonicalNumber();
    BOOL	InitiateClick();

    BOOL	Initiate();
    BOOL	Initiated();
    BOOL	Cancel();
    BOOL	CallOn();
    BOOL	Fleet(BOOL onoff);
    BOOL	ShowFullsigWindow (BOOL ShowNotHide);
    BOOL	ResetApproach();

    std::string CompactName();
    int		AK_p();
    int		UnitHeight();
private:
    BOOL	ShouldBeCoding();
    void	ContextMenu();
#ifndef TLEDIT
public:
    virtual void EditContextMenu(HMENU m);
private:
    std::string FormatPlate(TrackSec * ts, const std::string& lights, bool first);
#endif
};



#endif
