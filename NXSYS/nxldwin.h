#ifndef _NXSYS_LOGIC_DRAW_WINDOW_API_H__
#define _NXSYS_LOGIC_DRAW_WINDOW_API_H__

BOOL RegisterRelayLogicWindowClass (HINSTANCE hInstance);
int RelayShowString (const char *rnm);
void AskForAndDrawRelay (HWND hWnd);
void AskForAndShowStateRelay (HWND hWnd);
void DraftsbeingCleanupForOneLayout();
void DraftsbeingCleanupForSystem();

#endif
