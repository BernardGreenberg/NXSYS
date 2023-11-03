//
//  STLExtensions.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/12/19.
//  Copyright © 2019 BernardGreenberg. All rights reserved.
//

#ifndef STLExtensions_h
#define STLExtensions_h
#include <string>
#include <cstdarg>
#include <memory>
#include <algorithm>

std::string FormatStringVA(const char* fmt, va_list args);
std::string FormatString(const char* fmt, ...);
std::string stoupper( const std::string& s );
std::string stolower( const std::string& s );

// usage:  for (auto& foo : StartAtN(decltype(V), 1)(V))
template <class CT>
class Ranger {
    CT& rct;
private:
    int startx, endinc;
public:
    Ranger(CT& ct, int start_index, int end_increment = 0)  :
    rct(ct), startx(start_index), endinc(end_increment) {}
    typename CT::iterator begin() {
        return rct.begin() + startx;
    }
    typename CT::iterator end() {
        return rct.end() - endinc;
    }
};

// std::make_unique exists in C++ 14, not 11.  This is not std::, though.
// From https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

/* You would think that STL would have something like this, but it doesn't. Sort
   (a portion of) an array of pointers by the comparison operator of the objects
   pointed to.  No form with user-supplied function - if you want to supply your
   own function, just use std::sort and have your function do the dereferencing. */

template<class Iter>
void pointer_sort(Iter istart, Iter iend) {
    using ET = decltype(*istart);
    std::sort(istart, iend, [](ET p1, ET p2) {return *p1 < *p2;});
}
#endif /* STLExtensions_h */
