#include <string>

#ifdef __APPLE__
#define sPDelim "/"
#define cPDelim '/'
#else
#define sPDelim "\\"
#define cPDelim '\\'
#endif

void fnmerge (std::string& sresult, const char * drive, const char * dir, const char * fname, const char * ext) {
    if (drive != NULL) {
        size_t drvl = strlen(drive);
        sresult += drive;
        if (drvl > 0) {
            if (drive[drvl - 1] != ':') {
                sresult += ':';
            }
        }
    }
    if (dir != NULL) {
        size_t dirl = strlen(dir);
        sresult += dir;
        if (dirl > 0) {
            if (dir[dirl-1] != cPDelim) {
                sresult += cPDelim;
            }
        }
    }
    if (fname != NULL) {
        sresult += fname;
    }
    if (ext != NULL) {
        size_t extl = strlen(ext);
        if (extl > 0 && ext[0] != '.') {
            sresult += '.';
        }
        sresult += ext;
    }
}

void fnsplit(const char * path, char* pdrive, char* pdir, char* pfname, char* pext) {
    if (pdrive != NULL) {
        *pdrive = '\0';
    }
    if (pdir != NULL) {
        *pdir = '\0';
    }
    if (pfname != NULL) {
        *pfname = '\0';
    }
    if (pext != NULL) {
        *pext = '\0';
    }
    // no drive on mac
    const char * last_slash = strrchr(path, cPDelim);
    if (last_slash == NULL) {
        last_slash = path;
    } else {
        if (pdir != NULL) {
            size_t dirl = last_slash - path;
            if (dirl == 0) {
                strcpy(pdir, sPDelim);
            } else {
                strncpy(pdir, path, dirl);
                pdir[dirl] = '\0';
            }
        }
        last_slash += 1;
    }
    const char * last_period = strrchr(last_slash, '.');
    if (last_period == NULL) {
        if (pfname != NULL) {
            strcpy(pfname, last_slash);
        }
    } else { // last period for real
        if (pext != NULL) {
            strcpy(pext, last_period); // with period
        }
        if (pfname != NULL) {
            size_t fnamel = last_period - last_slash;
            strncpy(pfname, last_slash, fnamel);
            pfname[fnamel] = '\0';
        }
    }
}
