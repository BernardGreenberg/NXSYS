/* Win32 get my build date function  -- in C */

#include <windows.h>
#include <time.h>	/* this unix-based garbage is suboptimal... */
#include <winnt.h>
#include <getmodtm.h>

/* Stolen from Pygmailer util.c 25 February 1996 */

static time_t Cache = (time_t) 0;

static time_t KludgeW32GMT (HANDLE h) {
    char buf[200];

    DWORD aa;
    HANDLE rh;
    if (Cache != (time_t) 0)
       return Cache;
    GetModuleFileName (h, buf, sizeof(buf)-1);

    rh = CreateFile (buf, GENERIC_READ, FILE_SHARE_READ,
		    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (rh == INVALID_HANDLE_VALUE) {
rn:	if (rh != INVALID_HANDLE_VALUE)
	    CloseHandle (rh);
	return time(NULL);
    }
    if (!ReadFile(rh, buf, sizeof(IMAGE_DOS_HEADER), &aa,NULL) ||
        0xFFFFFFFF == SetFilePointer (rh, ((PIMAGE_DOS_HEADER) buf)->e_lfanew,
                                       NULL, FILE_BEGIN) ||
	!ReadFile(rh, buf, sizeof(IMAGE_FILE_HEADER) + 4, &aa,NULL))
	goto rn;
    CloseHandle (rh);
    return Cache = (time_t) (((PIMAGE_NT_HEADERS)buf)->FileHeader.TimeDateStamp);
}

time_t GetModuleTime (HANDLE h) {
    if (h == NULL)
	h = GetModuleHandle (NULL);

    if ((((int) h) & 0xFFFF0000) == 0)
	return KludgeW32GMT(h);
    else {
       unsigned char * p = (unsigned char *) h;
       PIMAGE_DOS_HEADER dh = (PIMAGE_DOS_HEADER) p;
       PIMAGE_NT_HEADERS inth = (PIMAGE_NT_HEADERS) (p + dh->e_lfanew);
       return (time_t) (inth->FileHeader.TimeDateStamp);
    }
}
