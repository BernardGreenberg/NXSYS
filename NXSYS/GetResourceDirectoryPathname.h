//
//  GetResourceDirectoryPathname.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 2/21/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#ifndef GetResourceDirectoryPathname_h
#define GetResourceDirectoryPathname_h

#include <filesystem>

/* different implementations on Mac and Windows */

std::filesystem::path GetResourceDirectoryPathname();

#endif /* GetResourceDirectoryPathname_h */
