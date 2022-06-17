//
//  AppBuildSignature.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 6/17/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#ifndef AppBuildSignature_h
#define AppBuildSignature_h

#include <string>
#include <time.h>

struct AppBuildSignature {
    time_t BuildTime;
    int VersionComponents [4];
    std::string ApplicationName;

    void Populate ();
    int Build () {return VersionComponents[3];}
    std::string OSBase();
    
    std::string VersionString() {
        return
        std::to_string(VersionComponents[0]) + "." +
        std::to_string(VersionComponents[1]) + "." +
        std::to_string(VersionComponents[2]);
    }
    
    std::string BuildString() {
        std::string desc;
        if (Build() != 0)
            desc += "build " + std::to_string(Build()) + " ";
    #if DEBUG
        desc += "(DEBUG) ";
    #endif
        char out_time[100];
        strftime(out_time, sizeof(out_time)/sizeof(out_time[0]), "of %m/%d/%y %H:%M", localtime(&BuildTime));
        return desc + out_time;
    }
    
    std::string TotalBuildString(bool with_os = false) {
        std::string desc = ApplicationName + " "  + VersionString();
        if (with_os)
            desc += "/" + OSBase();
        return desc  + " " + BuildString();
    }
};

#endif /* AppBuildSignature_h */
