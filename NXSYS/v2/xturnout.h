#ifndef _NXSYS_XTG_TURNOUT_H__
#define _NXSYS_XTG_TURNOUT_H__

/* better damned be xtgtrack.h */

#ifndef _NXSYS_EXTENDED_TRACK_GEOMETRY_H__
#error xturnout.h requires xtgtrack.h previously.
#endif

#define TN_AUXKEY_FORCE_NORMAL 1
#define TN_AUXKEY_FORCE_REVERSE 2

#define NKX_N 0
#define NKX_R 1

#define NKS_LineOfLite        0x01
#define NKS_IndicatingWhite   0x02
#define NKS_FlashingWhite     0x04
#define NKS_FlashingRed       0x08

#define NKS_Any (NKS_IndicatingWhite | NKS_FlashingWhite | NKS_FlashingRed)

class NKData {
public:
    ExitLight * ExLight;
    TrackCircuit * Circuit;
    int Status;
};


class Turnout {
public:

    int	XlkgNo;
    char Thrown;
    char LockSafe;
    char CLK_Coding;
    char AuxKeyForce;

    Relay *NWP, *RWP, *NWZ, *RWZ;
    Relay *NL, *RL;
    int NEnds;
    
    TrackJoint * Joints[2];
    NKData NK[2][2];

public:
    long MoveStartTime;
    char MovingPhase;
    char CLKCodingPhase;

    Turnout (int xno);
    void ThrowSwitch (int throwit);	/* name "Throw" is stolen already */

    void Timer ();
    void Hit (int mb, TrackSeg * ht);

    void StartMove();
    void CodingFlash (BOOL state);
    void CLKReport (BOOL state);
    void DisplayNoDC();
    void InvalidateAndTurnouts();
    void ProcessLoadComplete();
    int  AssignJoint (TrackJoint* tj);
    void SegConflict (Turnout* other_tn);
    void UpdateRoutings();
    void EditContextMenu(HMENU m);
    TrackSeg * GetOwningSeg();

  static void LSReporter(BOOL state, void*);
  static void TimeReporter(void*);
  static void CoderReporter(void* turnout, BOOL codestate);
  static void RWZReporter(BOOL state, void*);
  static void NWZReporter(BOOL state, void*);
  static void CLKReporter(BOOL state, void*);
};


int CreateTurnouts(TrackJoint ** joints, int njoints);

#endif
