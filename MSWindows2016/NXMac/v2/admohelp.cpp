#include "windows.h"
#include "compat32.h"
#include "commands.h"
#include "nxsysapp.h"
#include <string>
#include <time.h>
#include "helpdlg.h"

#define ALPHAS \
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_"

extern "C" char V2HelpText[];

static std::string ExpandDocString (const char * s);

void ResourceScat (UINT id, std::string& B) {
    char buf [1000];
    LoadString (GetModuleHandle(NULL), id, buf, sizeof(buf));
    std::string s = ExpandDocString(buf);	/* better not recurse infinitely. */
    B += s;
}

static void
InterpretDocCmd (const char * p, int len, std::string & B) {
    char cmd[33];
    if (len == 0 || len > sizeof (cmd)-1) {
fail:
	B += '%';
	if (len > 0)
	    B.append(p, len);
	return;
    }
    strncpy (cmd, p, len)[len] = '\0';
    if (!_stricmp (cmd, "BasisTechAddr"))
		ResourceScat (IDS_BASIS_ADDRESS, B);
    else if (!_stricmp (cmd, "EvalWarning")) {
#ifdef EVALUATION_EDITION
		ResourceScat (IDS_EVAL_WARNING, B);
#endif
    }
#if 0
    else if (!stricmp (cmd, "Version"))
		B += RJ_Version;
    else if (!stricmp (cmd, "EncryptionError"))
		B += EncryptionErrorMessage;

#endif
#ifdef RT_PRODUCT
    else if (!stricmp (cmd, "ExpireTime")) {
	char b[50];
	strftime (b, sizeof(b), "%#m/%#d/%Y at %H:%M",
		  localtime(&RTExpireTime));
	B += b;
    }
#endif
    else
	goto fail;
}


static std::string ExpandDocString (const char * s) {
    std::string B;
    while (1) {
	const char * p = strchr (s, '%');
	if (p == NULL)
	    break;
	else if (p[1] == '%') {
	    if (p > s)
		B.append (s, p-s);
	    s = p + 2;
	    B += '%';
	}
	else {
	    if (p > s)
		B.append (s, p-s);
	    p++;
	    int len = (int)strspn (p, ALPHAS);
	    InterpretDocCmd (p, len, B);
	    s = p + len;
	    if (*s == ';')
		s++;
	}
    }
    B += s;
    return B;
}


void DoV2Dialog () {

#ifndef NXSYSMac
    const char * title =
#ifdef RT_PRODUCT
			"RT-Designer Help/Usage"
#else
			"NXSYS Version 2 Info"
#endif
			;
    std::string s = ExpandDocString(V2HelpText);
    HelpDialog (s.c_str(), title);
#endif
}

