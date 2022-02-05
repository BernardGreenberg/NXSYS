#define Signal AUTCLI_Signal
#define TrackSec AUTCLI_TrackSec
#define ExitLight AUTCLI_ExitLight
#define Train AUTCLI_Train
#define TrafficLever AUTCLI_TrafficLever
#define FErInt NXScriptInternalError
#define FErUsr NXScriptUserError

void NXScriptInternalError (void *, const char *, ...);
void NXScriptUserError (void *, const char *, ...);
void NXScriptUserPrintf(const char *, ...);
struct IDispatch;
IDispatch * GetOLEAutInstanceInternal();
