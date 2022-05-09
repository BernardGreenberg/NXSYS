#ifndef _NXSYS_EXT_TRACK_GEOMETRY_LOADER_H__
#define _NXSYS_EXT_TRACK_GEOMETRY_LOADER_H__

void XTGLoadInit();
void XTGLoadClose();

#include <stdio.h>

void InitXTGReader ();
void SwitchesLoadComplete();
void AuxKeysLoadComplete();
int /*BOOL*/ XTGLoad (FILE * f);

#ifdef _NX_LISP_SYS_H__
int ProcessLayoutForm (Sexpr s);
#endif

#endif

   
