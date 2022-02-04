//
//  STLfnsplit.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/26/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

#ifndef __NXSYSMac__STLfnsplit__
#define __NXSYSMac__STLfnsplit__

#include <stdio.h>
#include <string>

void STLfnsplit(const char * path, std::string& drive, std::string& dir, std::string& name, std::string& ext);
std::string STLfnmerge (const std::string drive, const std::string dir,
                        const std::string fname, const std::string ext);


#endif /* defined(__NXSYSMac__STLfnsplit__) */
