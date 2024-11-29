//
//  STLExtensions.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/12/19.
//  Copyright © 2019 BernardGreenberg. All rights reserved.
//

#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <vector>
#include <cstdarg>

#include <stdio.h>

#include "STLExtensions.h"


std::string FormatStringVA(const char* fmt, va_list args) {
    va_list args1, args2;
    va_copy(args1, args);
    va_copy(args2, args);
    std::vector<char> result(1+vsnprintf(nullptr, 0, fmt, args1));
    va_end(args1);
    vsnprintf(result.data(), result.size(), fmt, args2);
    va_end(args2);
    return std::string(result.data(), result.size()-1);
}


std::string FormatString(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    return FormatStringVA(fmt, args);
}

/* http://www.cplusplus.com/forum/general/21215/ */

std::string stoupper( const std::string& s )
{
    std::string result;
    for (auto c : s) {
        result += std::toupper(c);
    }
    return result;
}

std::string stolower( const std::string& s )
{
    std::string result;
    for (auto c : s) {
        result += std::tolower(c);
    }
    return result;
}
