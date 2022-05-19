#ifndef _NXSYS_TLEDIT_H__
#define _NXSYS_TLEDIT_H__

#ifndef _NX_GRAPHOBJ_H__
class GraphicObject;
#endif

#ifdef NXSYSMac
#define CBOOL BOOL
#else
#define CBOOL bool
#endif

#ifndef _NXSYS_EXTENDED_TRACK_GEOMETRY_H__
class PanelSignal;
class TrackJoint;
class TrackSeg;
class ExitLight;
#endif

void TrackLayoutRodentate (HWND hWnd, UINT message, int x, int y);
void InsulateJoint (TrackJoint * joint);

void DoHelpDialog ();
void DoAboutDialog ();
void usererr (const char * control_string, ...);
void StatusMessage (const char * control_string, ...);
BOOL SaveLayout (const char * path);
void TLEditCreateSignal (TrackJoint * joint, CBOOL upright);
void TLEditCreateSignalFromSignal (PanelSignal * ps, CBOOL upright);
void TLEditCreateExitLightFromSignal(PanelSignal * ps, CBOOL upright);
void FlipSignal (PanelSignal * ps);
BOOL DoShiftLayoutDlg(int&x, int&y);
void ShiftLayout(int delta_x, int delta_y);
void ShiftLayout_(int delta_x, int delta_y);
void AssignFixOrigin(WPPOINT coords);
BOOL RegisterTextSampleClass(HINSTANCE hInstance);

extern int FixOriginWPX, FixOriginWPY;
extern HWND G_mainwindow;
extern HINSTANCE app_instance;
extern const char app_name[];
extern BOOL ExitLightsShowing;


#ifndef CONST_PI
#include "pival.h"
#endif

#ifdef WIN32
#define stricmp _stricmp
#define strdup _strdup
#define strlwr _strlwr
#define PRODUCT_NAME "TLEdit (Windows)"
#endif

#define MoveTo(d,x,y) MoveToEx(d,x,y,NULL)

#endif

#ifndef Global
#define Global
#endif

#ifndef Virtual
#define Virtual
#endif
