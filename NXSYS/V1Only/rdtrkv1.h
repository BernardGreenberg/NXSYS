#pragma once

#ifdef NXV2
error "rdtrkv1.h should not be used in NXV2"
#endif

#include <lisp.h>

int ProcessTrackForm(Sexpr s);
int ProcessSwitchForm (Sexpr s);
int ProcessSignalForm (Sexpr s);
int ProcessExitLightForm (Sexpr s);
int ProcessPlatForm (Sexpr s);
int ProcessTextForm (Sexpr s);
int ProcessTrafficleverForm (Sexpr s);
