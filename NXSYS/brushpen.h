#ifndef _NXSYS_BRUSHES_AND_PENS_H__
#define _NXSYS_BRUSHES_AND_PENS_H__

extern HFONT Fnt;

extern HPEN YellowXPen, FleetGreenPen, FleetBlackPen, NullPen;

extern HBRUSH TrackDftBrush, TrackOccupiedBrush, TrackRoutedBrush,
    GKRedBrush, GKYellowBrush, GKOffBrush, GKGreenBrush, GKWhiteBrush,
    GKHalfRedBrush;
extern HPEN TrackPen, SigPen, BgPen, HighlightTrackPen,
    TrackDftPen, TrackOccupiedPen, TrackRoutedPen, TrackDftPenThrown,
    PlatformPen, ExitLightPen, SelectedExitLightPen, RedExitLightPen,
    BlackExitLightPen;

extern SC_cord GU1, GU2,
      Track_Width_Delta, Track_Side_Fudge, Track_Side_Nolight, Track_Seg_Len,
      Track_Normal_Halfwidth, Lit_Seg_HalfWidth, SectionInterstice,
      Unlit_Half_Len, RWXO, RWYO;

HBRUSH CreateSBrush (COLORREF);
HPEN   CreateSPen   (int width, COLORREF);

void InitTrackGDI (int main_window_width, int main_window_height);
void TrackGraphicsCleanup();
extern COLORREF TrackDftCol;

#endif
