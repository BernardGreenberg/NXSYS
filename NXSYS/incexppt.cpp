#include <stdlib.h>
#include <string.h>
#include "incexppt.h"
#include "STLfnsplit.h"

#ifdef __APPLE__
static const char* DLS = "/";
#else
static const char* DLS = "\\";
#endif

using std::string;
//STLfnmerge needs inclusion on Windows

const char * include_expand_path (const char * basepath, const char * path, string& answer) {
    string bpdrive, bpdir, pdrive, pdir, fname, ext;
    STLfnsplit (path, pdrive, pdir, fname, ext);
    if (!pdrive.empty()) {
use_given:
        answer = path;
	return answer.c_str();
    }
    if (!pdir.empty() && pdir[0] == DLS[0])
	goto use_given;
    string garbage1, garbage2;
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

    answer = STLfnmerge (bpdrive, bpdir, fname, ext);
    return answer.c_str();
}

string STLincexppath(const string& basepath, const string& path) {
    string result;
    include_expand_path (basepath.c_str(), path.c_str(), result);
    return result;
}

    
