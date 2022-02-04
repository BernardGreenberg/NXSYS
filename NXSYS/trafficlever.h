#ifndef _NX_TRAFFICLEVER_H__
#define _NX_TRAFFICLEVER_H__

#ifndef NXV2
#ifndef TLEDIT
#include "track.h"
#endif
#endif

#ifdef TLEDIT
#ifndef _FILE_DEFINED
#include <stdio.h>
#define _FILE_DEFINED
#endif
#endif

class TrafficLever;

#define TRAFLEV_LEFT  0
#define TRAFLEV_RIGHT 1

class TrafficLeverIndicator {
public:
    TrafficLeverIndicator (int plusorminus) : PlusMinusOne(plusorminus) {
        White = Coding = Red = FALSE;
    }
    static void DataInit();
    void Draw(HDC hdc, int xcen, int ycen);
    TrafficLever * Lever;
private:
    BOOL White, Red, Coding;
    int PlusMinusOne;
    static int rad, arht;
    static double doff;

#ifndef TLEDIT
public:
    static void WhiteReporter(BOOL state, void*);
    static void RedReporter(BOOL state, void*);
    void SetWhite (BOOL on);
    void SetRed (BOOL on);
#endif
};

class TrafficLever : public GraphicObject {
  public:
    int XlkgNo;

    TrafficLever (int xlkg_no, WP_cord p_wpx, WP_cord p_wpy, int right_normal);

    void NormalizeTrafficLever();
    void SetXlkgNo(int xno);
    virtual void Display (HDC hdc);
    virtual int TypeID(), ObjIDp(long);
    void InitState();
#ifndef TLEDIT
    BOOL Throw (BOOL reverse_wanted);
    virtual void Hit (int mb);
    virtual void UnHit ();
    void ProcessLoadComplete();

#else

    virtual void EditClick(int x, int y);
    virtual ~TrafficLever();
    virtual BOOL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
    virtual int Dump (FILE * f);
#endif
private:
    void DrawKnob(HDC hdc);
    void SetNormalReverseStatus (int right_normal);
    void LocateKnobPoint(float radius, float angle, int& x, int& y);

    int  NormalIndex;
    int  ReverseIndex;
    BOOL Normal, Reverse;
    std::vector<TrafficLeverIndicator> Indicators {-1, 1};
    bool KnobLeft, KnobRight;

    bool TriState;
    std::string NumString;
    Relay *NL, *RL;


};

void InitTrafficLeverData();

#endif
