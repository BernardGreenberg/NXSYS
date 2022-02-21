//
//  InterlockingLibrary.hpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 2/21/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#ifndef InterlockingLibrary_hpp
#define InterlockingLibrary_hpp

#include <string>
#include <vector>
#include <filesystem>

struct InterlockingLibraryEntry {
    std::string Title;
    std::filesystem::path Pathname;
};

typedef std::vector<InterlockingLibraryEntry> InterlockingLibrary;

InterlockingLibrary GetInterlockingLibrary();

#endif /* InterlockingLibrary_hpp */
