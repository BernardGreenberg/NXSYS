#include <windows.h>
#include "relays.h"

int StartPrintRelays(char const* s, int i) {
	return 0;
}

int PageFrame(void) {
	return 0;
}

int FinishPrintRelays(void) {
	return 0;
}

HWND getRelayDrafterHWND(bool force) {
	(void)force;
	return nullptr;
}

int RegisterRelayLogicWindowClass(HINSTANCE hApp) {
	(void)hApp;
	return 1;  //must be nonzero for app to start up
}

Relay* RelayListDialog(int objNo, const char* typeName,
	Relay** theRelays, int nRelays,
	const char* op) {

	return nullptr;
}

void ShowStateRelay(Relay* r) {
	(void)r;
}

Relay* MacRlyDialog(const char* title) {
	(void)title;
	return nullptr;
}
