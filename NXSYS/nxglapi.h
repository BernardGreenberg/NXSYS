#ifndef _NX_OPEN_GL_FEATURE_API_H__
#define _NX_OPEN_GL_FEATURE_API_H__

#ifdef WIN32

#ifndef _NX_TRACK_H__
class TrackDef;
#endif

#ifndef _NX_SIGNAL_H__
class Signal;
#endif

#ifndef _NXSYS_LAYOUT_GLOBAL_H__
struct NXSYSLayoutGlobal;
#endif

typedef void (*SigStateFcn)(Signal*);


BOOL NXGLMaybeInit(BOOL barf_if_unable);
void NXGLUnload();

void NXGLDefineSigStateFcn(SigStateFcn f);
void NXGLMaybeClose();

void NXGLDefineLayout (NXSYSLayoutGlobal*);
void NXGLUnloadLayout();
void NXGLReportHorizontalScalingFactor (double pixels_per_sim_foot);
void NXGLDefineSignal (Signal * sig);

void NXGLReportSignalStateChange (Signal * sig);
int  NXGLCreateView (TrackDef * track_def, long station_no, int southp);
void NXGLDestroyView (int view_tag);
void NXGLUpdateTrainPos (int view_tag, TrackDef * track_def,
			 long sno, int southp);
void NXGLReportMainHWnd (HWND h);



#else

#define NXGLMaybeInit(a) FALSE
#define NXGLDefineSigStateFcn(a)
#define NXGLMaybeClose()

#define NXGLDefineLayout(a)
#define NXGLReportHorizontalScalingFactor (a)
#define NXGLDefineSignal(a)
#define NXGLUnloadLayout()

#define NXGLReportSignalStateChange(a)
#define NXGLCreateView(a,b,c) 0
#define NXGLDestroyView(a)
#define NXGLUpdateTrainPos(a,b,c,d)
#define NXGLUnload()
#define NXGLReportMainHWnd(h)

#endif  /* WIN32 */

#endif
