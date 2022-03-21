#ifndef _NXSYS_TRAIN_SYSTEM_INTERNAL_HEADER_H__
#define _NXSYS_TRAIN_SYSTEM_INTERNAL_HEADER_H__


#include <set>

typedef TrackSeg TrackUnit;
class Train;

struct _Pointpos {
    TrackSeg * ts;
    /* An arbitrary linear measure in hundreds of feet
       that gets bigger as train moves in the "front" direction. It
       can be negative.  But it always grows positively. */

    /* if train turns around, x has to be turned around, too */
    double x;
    /* what x value corresponds to the "entrance" end of this track_seg. */
    double x_at_seg_start;
    /* which end of that seg is ahead of us in present train motion direct. */
    TSEX facing_ex;

    /* Last IJ passed, formatted */
    char LastIJID [24];
    float FeetSinceLastIJ;
    float FSLIJatSegStart;
    BOOL IAmFront;
    int FindTrackSeg();
    void PassJoint (TrackJoint * tj);	/* hey, man! */
    void Reverse(double new_x);
    Train * Trn;
};

typedef struct _Pointpos Pointpos;

class Train {


public:
    Train(int train_id, GraphicObject* g, int options);
    void vanish();    //Deletes id from Trains STL map, which makes its held unique_ptr call dtor.
    Train*      IsThisMe(void* vptr);     //used for validating pointers that were given out. "this" m/b good.
    bool        CheckNextSigEQ(Signal* g) {return NextSig == g;};

    int		id;                      //"train number" (not 4 = Woodlawn) for dialog and STL lookup
    HWND	Dialog;


private:
    bool        TimerPending;

    bool	observant;

    Pointpos	front;
    Pointpos	back;
    double	Speed;
    double	Cruise;
    double	Length;
    double	LastTargetSpeed;
    long	Time;
    Signal*	NextSig;
    std::set<TrackSeg*> Occupied;
    double	X_Of_Next_Signal;
    
    bool    CODisplay;


    void    UpdatePositionReport();
    void    UpdateSwitches();
    Signal *FindNextSig();
    void    InstallNextSig();
    int     ComputeOccupations();
    void    StringFld (int id, const char* text);
    void    StringFld (int id, const std::string& str);
    void    StringFldF(int id, const char* fmt, ...);
    void    DecFld (int id, int val);
    void    ComputeWindowPlacement();
    void    ComputeNextMotion();
    void    Observance();
    void    SetTimer(), KillTimer();
    void    CheckHalted();
    void    Reverse();
    void    InitPosition();
    void    InitPositionTracking(TrackUnit * ts);
    void    ReportSig (Signal *, int, int);
    void    Trip(Signal * g);
public:
    int	    Command (WPARAM);
    void    Init (GraphicObject * g, long options);
    int     ScrollHandler (WPARAM, WORD);
    void    SetSpeed(double);
    void    SetSpeedFractional(double);
    void    ExplicitSpeed(double);
    double  GetSpeed() {return Speed;}
    BOOL    ShowWindowIfOwnTU (TrackUnit * tu);
    double  GetCruise() {return Cruise;}
    void    TimerHandler();

#ifdef NXOGL
    void    SetCabviewCheck (BOOL v);
#endif
    void    SetOccupied (TrackUnit * ts);
    void    SetUnoccupied (TrackUnit * ts);

    void    MaybeNoticeSignalChange (Signal * g);
            ~Train();

static void StaticTimerHandler(void *);

};


typedef class Train Train;

BOOL VerifyTrackSelectionAcceptability (TrackUnit * ts);
TrackUnit * FindTrainEntryTrackSectionByNomenclature (long nomenclatura);
BOOL WindowToTopFromTrackUnit(TrackUnit * ts);

#endif
