#ifndef _NXSYS_LAYOUT_GLOBAL_H__
#define _NXSYS_LAYOUT_GLOBAL_H__

#include <string>


struct NXSYSLayoutGlobal {
    std::string RouteIdentifier;
    BOOL	TorontoStyle;
    double	PixelsPerSimFoot;
    double	TrainLengthFeet;
    BOOL	IRTStyle;
    int		AppBaseMajor, AppBaseMinor;
    BOOL        TrafficLeversTristate;
};

extern NXSYSLayoutGlobal Glb;

void NXSYSGlobalDataInit();
long compute_track_relayno (int trkno, int secno);

#endif
