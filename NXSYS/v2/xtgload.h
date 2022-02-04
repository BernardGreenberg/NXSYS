#ifndef _NXSYS_EXT_TRACK_GEOMETRY_LOADER_H__
#define _NXSYS_EXT_TRACK_GEOMETRY_LOADER_H__

void XTGLoadInit();
void XTGLoadClose();

#ifdef _MSC_VER
#ifndef _FILE_DEFINED
struct FILE;
#endif
#endif

void InitXTGReader ();
void SwitchesLoadComplete();
void AuxKeysLoadComplete();
int /*BOOL*/ XTGLoad (FILE * f);

#ifdef _NX_LISP_SYS_H__
int ProcessLayoutForm (Sexpr s);
#endif

#endif

   