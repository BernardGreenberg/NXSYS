#ifndef _NXSYS_RELAY_API_H__
#define _NXSYS_RELAY_API_H__

#ifndef _NX_SYS_RELAYS_H__
class Relay;
class ReportingRelay;
typedef void (*RelRptFcn) (BOOL State, void *Obj);
ReportingRelay* SetReporterIfExists (long itemno, const char * s, RelRptFcn f, void * obj);
ReportingRelay * CreateAndSetReporter (long itemno, const char * s, RelRptFcn f, void * obj);
void RelaySetReporter (Relay * r, RelRptFcn f, void * obj);
void MoveReporterAssociatedObject(ReportingRelay* relay, void* object);
#endif

extern int RelayUseDefined (Relay * rr);
extern void ReportToRelay (Relay *, BOOL);
extern void PulseToRelay (Relay *);
extern Relay * CreateQuislingRelay (long, const char *);
extern Relay * GetRelay2NoCreate (long, const char *);
extern int RelayState (Relay * rr);
void InitRelaySys();
void CleanUpRelaySys();

#endif
