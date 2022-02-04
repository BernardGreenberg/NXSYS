/* Created 6 Dec 2016 by BSG, Mac Env, to solve bug hit by Dave B */

#include <map>
#include <string>
#include <stdio.h>
#include "SwitchConsistency.h"

typedef std::map<long, int>tSwitchMap;

static tSwitchMap SwitchMap;

#define ID_SINGLETON 4

static int massageTag(int AB0) {
    int tag = AB0;
    if (tag == 0) {
        tag = ID_SINGLETON;
    }
    return tag;
}

void SwitchConsistencyUndefine(long ID, int AB0) {
    tSwitchMap::const_iterator it = SwitchMap.find(ID);
    if (it == SwitchMap.end()) {
        return; // not found -- shouldn't really happen.
    }
    int tag = massageTag(AB0);
    int current = it->second;
    if (current == 0) { // this shouldn't happen, either
        SwitchMap.erase(it);
        return;
    }
    if (tag == current) {// we are deleting the only point remaining, or singleton
        SwitchMap.erase(it);
        return;
    }
    if ((current & AB0) != 0) {
        current = current ^ AB0;
        if (current == 0)
            SwitchMap.erase(it);
        else
            SwitchMap[ID] = current;
        return;
    }
    // otherwise (inconsistent) just leave it, I guess.
}

void SwitchConsistencyClear() {
    SwitchMap.clear();
}



bool SwitchConsistencyDefine(long ID, int AB0, std::string& complaint) {
    if (!SwitchConsistencyDefineCheck(ID, AB0, complaint)) {
        return false;
    }
    int current = 0;
    if (SwitchMap.count(ID) != 0) {
        current = SwitchMap[ID];
    }
    SwitchMap[ID] = current | massageTag(AB0);
    return true;
}

bool SwitchConsistencyDefineCheck(long ID, int AB0, std::string& complaint) {
    char buf [256];
    complaint.clear();
    tSwitchMap::iterator it = SwitchMap.find(ID);
    if (it == SwitchMap.end()) {  // terrific!
        return true;
    }
    int current = it->second;
    if (current == 0) { // this shouldn't happen, either
        return true;
    }
    int tag = massageTag(AB0);
    const char * descrip;
    switch (tag) {
        case ID_SINGLETON:
            descrip = "singleton";
            break;
        case 1:
            descrip = "A";
            break;
        case 2:
            descrip = "B";
            break;
        default:
            descrip = "??";
            break;
    }
    if ((current & tag) != 0) {
        sprintf (buf, "Switch %ld %s is already defined elsewhere.", ID, descrip);
        complaint = buf;
        return false;
    }
    if ((current | tag) == 3) { // ideal
        return true;
    }
    if (tag == ID_SINGLETON) {
        sprintf (buf, "Switch %ld already has A and/or B somewhere, can't create singleton.", ID);
        complaint = buf;
        return false;
    }
    sprintf (buf, "Switch %ld is already defined as a singleton, so can't create %ld %s.",
             ID, ID, descrip);
    complaint = buf;
    return false;
}

bool SwitchConsistencyTotalCheck (std::string& complaint) {
    complaint.clear();
    char buf[256];
    for (const auto& it : SwitchMap) {
        long id = it.first;
        int tag = it.second;
        if (tag == 1) {
            sprintf (buf, "Switch half %ld A exists but there is no B.", id);
            complaint = buf;
            return false;
        } else if (tag == 2) {
            sprintf (buf, "Switch half %ld B exists, but there is no A.", id);
            complaint = buf;
            return false;
        } else if (tag > ID_SINGLETON) {
            sprintf (buf, "Switch %ld has both singleton and paired points (bug).", id);
            complaint = buf;
            return false;
        }
    }
    return true;
}

