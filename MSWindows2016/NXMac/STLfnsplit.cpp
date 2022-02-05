//
//  STLfnsplit.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/26/14.
//  Copyright (c) 2014 Bernard Greenberg. All rights reserved.
//

#include "STLfnsplit.h"

#ifdef __APPLE__
static const char pathDelim = '/';
#else
#include<algorithm>
static const char driveDelim = ':';
static const char pathDelim = '\\';
#endif

void STLfnsplit(const char * path, std::string& drive, std::string& dir, std::string& name, std::string& ext) {

    drive.clear();
    dir.clear();
    name.clear();
    ext.clear();

#ifndef __APPLE__
    const char * first_slash = strchr(path, pathDelim);
    if (first_slash != NULL) {
        const char * first_colon = strchr(path, driveDelim);
        if (first_colon != NULL) {
            first_colon += 1; //skip over/include
            drive.append(path, first_colon - path);
            path = first_colon;
        }
    }
	// platform-dep bug in STLfnsplit requiring this has been fixed (12/2/2016), but it's still wise...
	std::string cpyp(path);
	std::replace(cpyp.begin(), cpyp.end(), '/', '\\');
	path = cpyp.c_str();
#endif
    const char * last_slash = strrchr(path, pathDelim);
    if (last_slash == NULL) {
        last_slash = path;
    } else {
        size_t dirl = last_slash - path;
        if (dirl == 0) {
            dir = pathDelim;
        } else {
            dir.append(path, dirl);
        }
        last_slash += 1;
    }
    const char * last_period = strrchr(last_slash, '.');
    if (last_period == NULL) {
        name = last_slash;
    } else { // last period for real
        ext = last_period;
        size_t fnamel = last_period - last_slash;
        name.append(last_slash, fnamel);
    }
}
