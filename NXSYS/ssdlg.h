#ifndef _NXSYS_SHOW_STOP_DIALOG_H__
#define _NXSYS_SHOW_STOP_DIALOG_H__

#define SSDL_FIRST     200
#define SHOW_STOPS_RED SSDL_FIRST
#define SHOW_STOPS_ALWAYS SSDL_FIRST+1
#define SHOW_STOPS_NEVER SSDL_FIRST+2
#define SSDL_LAST SSDL_FIRST+2

void ImplementShowStopPolicy (int policy);
extern int ShowStopPolicy;

#endif
