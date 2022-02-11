#ifdef _WIN32
#define MoveTo(dc,x,y) MoveToEx(dc,x,y,NULL)
#define WNDPROC_DCL LRESULT APIENTRY
#define DLGPROC_DCL BOOL APIENTRY
#define NOTIFY_CODE(wParam,lParam) HIWORD(wParam)
#define ASMCHARDEF(x) extern "C" {extern char x[];}
#define SCROLLARGS(wParam,lParam) LOWORD(wParam), HIWORD(wParam)

#else
#ifndef __APPLE__
#define WNDPROC_DCL long FAR PASCAL _export
#ifndef DLGPROC_DCL
#define DLGPROC_DCL BOOL FAR PASCAL _export
#endif
#endif
#define ASMCHARDEF(x) extern char far x[]
#define NOTIFY_CODE(wParam,lParam) HIWORD(lParam)
#define SCROLLARGS(wParam,lParam) wParam,LOWORD(lParam)
#endif

#ifdef _MSC_VER

#include <stdlib.h>

#define strncmpi _strnicmp
#define _fstrcpy strcpy
#define fnsplit _splitpath
#define MAXEXT _MAX_EXT
#define MAXPATH _MAX_PATH
#define MAXFILE _MAX_FNAME
#define MAXDIR _MAX_DIR
#define MAXDRIVE _MAX_DRIVE
//#define fnmerge _makepath
#define fnsplit _splitpath
#else
//#include <dir.h>
#endif
