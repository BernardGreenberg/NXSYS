#ifdef WINDOWS
static const char* DLS= "\\";
#endif


#include <string>

#ifdef __APPLE__

static const char* DLS = "/";
#else
static const char* DLS = "\\";
#endif

//work this out on windows.
void fnmerge(std::string&, const char*, const char *, const char *, const char *);

#include <stdlib.h>
#include <string.h>
#include "incexppt.h"
#include "STLfnsplit.h"

const char * include_expand_path (const char * basepath, const char * path, std::string& answer) {
    std::string bpdrive, bpdir, pdrive, pdir, fname, ext;
    STLfnsplit (path, pdrive, pdir, fname, ext);
    if (!pdrive.empty()) {
use_given:
        answer = path;
	return answer.c_str();
    }
    if (!pdir.empty() && pdir[0] == DLS[0])
	goto use_given;
    std::string garbage1, garbage2;
    STLfnsplit (basepath, bpdrive, bpdir, garbage1, garbage2);
    if (!pdir.empty()) {
	/* no drive, pdir is not absolute, or omitted. must be rel. */
	if (!bpdir.empty()) {	/* if it is "", just cat pdir */
            if (!strcmp (bpdir.c_str(), DLS)) {
                bpdir.clear();
            } else {
                // we know that it's not empty.
                if (bpdir[bpdir.size() - 1] == DLS[0])
                    bpdir.resize(bpdir.size() - 1);
            }
            bpdir += DLS;
	}
        bpdir += pdir;

    }

    fnmerge (answer, bpdrive.c_str(), bpdir.c_str(), fname.c_str(), ext.c_str());
    return answer.c_str();
}
