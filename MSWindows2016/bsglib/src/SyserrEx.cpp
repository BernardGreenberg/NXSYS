#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "SyserrEx.h"

#pragma comment(lib,"user32.lib")

char * SysLastErrstrToBufA(char * buf, int buflen) {
    return SysErrstrToBufA (buf, buflen, GetLastError());
}

char * SysErrstrToBufA (char * Buf, int buflen, DWORD ercode) {

    wsprintfA (Buf, "Error 0x%lX", ercode);

    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
		  NULL,
		  ercode,
		  LANG_USER_DEFAULT,
		  Buf, 
		  buflen,
		  NULL);
    return Buf;
}

WCHAR * SysLastErrstrToBufW(WCHAR * buf, int buflen) {
    return SysErrstrToBufW ((const WCHAR *)buf, buflen, GetLastError());
}

WCHAR * SysErrstrToBufW(WCHAR * Buf, int buflen, DWORD ercode) {

    wsprintfW (Buf, L"Error 0x%lX", ercode);

    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
		   NULL,
		   ercode,
		   LANG_USER_DEFAULT,
		   Buf, 
		   buflen,
		   NULL);
    return Buf;
}
