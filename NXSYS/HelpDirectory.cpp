//
//  HelpDirectory.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 2/22/22.  (!!!!)
//  The late Bishop Desmond Tutu wore a size two tutu, too, or maybe didn't....
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#include <regex>
#include "HelpDirectory.hpp"
#include "pugixml.hpp"
#include "GetResourceDirectoryPathname.h"
#include "STLExtensions.h"

using std::string;
namespace fs = std::filesystem;

#ifdef NXSYSMac
static bool iAmMac = true;
#else
static bool iAmMac = false;
#endif
static bool iAmWindows = !iAmMac;

HelpDirectory GetHelpDirectory () {

    fs::path resource_dir = GetResourceDirectoryPathname(),
             xml_path = resource_dir / "Help.xml";

    pugi::xml_document doc;

    if (doc.load_file(xml_path.string().c_str())) {
        HelpDirectory Help;
        auto library = doc.child("help");
        for (auto help : library.children()) {
            string include = stolower(help.attribute("url").value());
            if ((include == "mac" && !iAmMac) || (include == "windows" && !iAmWindows)) continue;
            HelpDirectoryEntry E;
            E.Title = help.attribute("title").value();
            string url = help.attribute("url").value();
            E.isLocalPath = false;
            if (url.find("file://", 0) == 0) {
                E.isLocalPath = true;
                string path = url.substr(7);
                if (path.length() && path[0] == '/') // always slash in xml file
                    E.LocalPathname = path;
                else
                    E.LocalPathname = resource_dir / path;
            }
            else
                E.URL = url;

            Help.push_back(E);
        }
        return Help;
    } else {
        fprintf(stderr, "Expected Help Directory XML file cannot be opened: %s\n", xml_path.string().c_str());
    }
    return HelpDirectory{};
}
