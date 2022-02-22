//
//  HelpDirectory.hpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 2/22/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#ifndef HelpDirectory_hpp
#define HelpDirectory_hpp


#include <string>
#include <vector>
#include <filesystem>

struct HelpDirectoryEntry {
    std::string Title;
    std::string URL;
    std::filesystem::path LocalPathname;
    bool isLocalPath;
};

typedef std::vector<HelpDirectoryEntry> HelpDirectory;

HelpDirectory GetHelpDirectory();

#endif /* HelpDirectory_hpp */
