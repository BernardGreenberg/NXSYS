#ifndef _NXSYS_RELAY_TRACE_API_H__
#define _NXSYS_RELAY_TRACE_API_H__

BOOL RegisterRelayTraceWindowClass (HINSTANCE hInstance);
BOOL RelayTraceExposedP();
void EnableRelayTrace(BOOL expose_it);
typedef void (*tRelayTracer) (const char * s, int state);
void SetRelayTrace (tRelayTracer);

#endif
