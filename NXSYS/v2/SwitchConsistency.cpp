/* Created 6 Dec 2016 by BSG, Mac Env, to solve bug hit by Dave B */

#include <map>
#include <string>
#include <stdio.h>
#include "SwitchConsistency.h"
#include "STLExtensions.h"

using std::string;

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
    if (auto r = SwitchConsistencyDefine(ID, AB0)) {
        complaint = r.value;
        return false;
    }
    complaint.clear();
    return true;
}

ValidatingValue<string> SwitchConsistencyDefine(long ID, int AB0) {
    string complaint;
    if (!SwitchConsistencyDefineCheck(ID, AB0, complaint))
        return complaint;

    int current = 0;
    if (SwitchMap.count(ID) != 0) {
        current = SwitchMap[ID];
    }
    SwitchMap[ID] = current | massageTag(AB0);
    return {};
}

bool SwitchConsistencyDefineCheck(long ID, int AB0, std::string& complaint) {
    complaint.clear();
    if (auto r = SwitchConsistencyDefineCheck(ID, AB0)) {
        complaint = r.value;
        return false;
    }
    return true;
}


ValidatingValue<string> SwitchConsistencyDefineCheck(long ID, int AB0) {
    tSwitchMap::iterator it = SwitchMap.find(ID);
    if (it == SwitchMap.end())  // terrific!
        return {}; // no complaint
    int current = it->second;
    if (current == 0) { // this shouldn't happen, either
        return {}; // no complaint
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
    if ((current & tag) != 0)
        return FormatString ("Switch %ld %s is already defined elsewhere.", ID, descrip);
    if ((current | tag) == 3){ // ideal
        return {};
    }
    if (tag == ID_SINGLETON)
        return FormatString("Switch %ld already has A and/or B somewhere, can't create singleton.", ID);
    return FormatString("Switch %ld is already defined as a singleton, so can't create %ld %s.",
             ID, ID, descrip);
}

bool SwitchConsistencyTotalCheck (string& complaint) {
    if (auto r = SwitchConsistencyTotalCheck()) {
        complaint = r.value;
        return false;
    }
    complaint.clear();
    return true;
}

ValidatingValue<string> SwitchConsistencyTotalCheck() {
    for (const auto& it : SwitchMap) {
        long id = it.first;
        switch(it.second) {
            case 1:
                return FormatString("Switch half %ld A exists but there is no B.", id);
            case 2:
                return FormatString("Switch half %ld B exists, but there is no A.", id);
            default:
                if (it.second > ID_SINGLETON)
                    return FormatString("Switch %ld has both singleton and paired points (bug).", id);
                break;
        }
    }
    return{};
}

