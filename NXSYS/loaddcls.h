#ifndef _NXSYS_LAYOUT_LOAD_DCLS_H__
#define _NXSYS_LAYOUT_LOAD_DCLS_H__

#include <string>

extern std::string InterlockingName;
extern char InterlockingLoaded, InterpretedP;
void DropAllSignals(), DropAllApproach(), ClearAllTrackSecs(),
      NormalAllSwitches(), ClearAllAuxLevers();
void BobbleRGPs();
const char * ReadLayout (const char* lpstrFileName);


#endif
