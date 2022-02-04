#ifndef _NXSYS_DEMO_API_H__
#define _NXSYS_DEMO_API_H__

void Demo (const char *);
void DemoSay(const char *);
void DemoPause (int);
void DemoBlurb (const char *);
void CreateDemoWindow (HWND hWndPar);
void ResizeDemoWindowParent (HWND hWndPar);
void HideDemoWindow();

#endif
