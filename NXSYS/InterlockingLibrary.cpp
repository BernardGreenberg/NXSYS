//
//  InterlockingLibrary.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 2/21/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#include "InterlockingLibrary.hpp"
#include "GetResourceDirectoryPathname.h" // platform-dependent
#include "pugixml.hpp"
#include <regex>

using std::string;
namespace fs = std::filesystem;

InterlockingLibrary GetInterlockingLibrary () {

    fs::path resource_dir = GetResourceDirectoryPathname(),
             xml_path = resource_dir / "InterlockingLibrary.xml";

    pugi::xml_document doc;

    if (doc.load_file(xml_path.string().c_str())) {
        InterlockingLibrary Lib;
        auto library = doc.child("interlocking-library");
        for (auto interlocking : library.children()) {
            InterlockingLibraryEntry E;
            E.Title = interlocking.attribute("name").value();
            string spath = interlocking.attribute("path").value();
            string rpath = std::regex_replace(spath, std::regex(R"(\$\(here\))"), resource_dir.string());
            E.Pathname = rpath;
            Lib.push_back(E);
        }
        return Lib;
    } else {
        fprintf(stderr, "Expected XML file cannot be opened: %s\n", xml_path.string().c_str());
    }
    return InterlockingLibrary{};
}
