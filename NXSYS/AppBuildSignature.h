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
    time_t BuildTime = 0;
    int VersionComponents [4]{0, 0, 0, 0};
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
        if (Build() == 0)
            desc = "Build ";
        else
            desc += "build " + std::to_string(Build()) + " ";
#ifdef NXSYSMac
     #if defined(__aarch64__) || defined(_M_ARM64)
        desc += "(ARM64) ";
      #else
        desc += "(Intel) ";
      #endif
#endif
      #if defined(DEBUG) || defined(_DEBUG)
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
