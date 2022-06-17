//
//  MacAppBuildSignature.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 6/17/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#include <stdio.h>
#include "AppBuildSignature.h"
#include <string>
#include <regex>

using std::string, std::regex, std::regex_match, std::to_string;


static regex SIMPLE_DIGITS("^\\d+$");
static regex TRIPARTITE_VERSION (R"(^(\d+)\.(\d+)\.(\d+)$)");

void AppBuildSignature::Populate() {

    memset(&VersionComponents, 0, sizeof(VersionComponents));

    NSDictionary * dict = [[NSBundle mainBundle] infoDictionary];

    std::smatch matchv;
    NSString* nss_build_number = [dict objectForKey:@"CFBundleVersion"];  // "1478"
    if (nss_build_number != nil) {
        string sbuild_number = [nss_build_number UTF8String];
        if (regex_match(sbuild_number, matchv, SIMPLE_DIGITS)) {
            VersionComponents[3] = std::stoi(sbuild_number);
            assert(Build() > 1); // Shouldn't happen on Mac except for brand-new application -- deal with it.
        }
    }

    NSString* nss_version = [dict objectForKey:@"CFBundleShortVersionString"]; // "2.7.0"
    if (nss_version != nil) {
        string sversion = [nss_version UTF8String];
        if (regex_match(sversion, matchv, TRIPARTITE_VERSION)) {
            for (int i = 1; i < 4; i++)
                VersionComponents[i-1] = std::stoi(matchv[i].str());
        }
    }
    NSString* nss_app_name = [dict objectForKey:@"CFBundleDisplayName"];  // "NXSYS" or "TLEdit"
    if (nss_app_name != nil)
        ApplicationName = [nss_app_name UTF8String];

    BuildTime = 0;
    NSDate * nsd_date = [dict objectForKey:@"BundleBuildDate"];
    if (nsd_date != nil)
        BuildTime = (time_t)[nsd_date timeIntervalSince1970];
}

string AppBuildSignature::OSBase() {return "macOS";}

