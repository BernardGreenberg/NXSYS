//
//  tjjuda.h
//  TJhtml
//
//  Created by Bernard Greenberg on 1/14/22.
//

#ifndef tjjuda_h
#define tjjuda_h

#include <string>

namespace tjjuda {

const string hebrew1(const std::string s, bool pointed = true);
const string yiddish1(const std::string s);
};

using tjjuda::hebrew1;
using tjjuda::yiddish1;

#endif /* tjjuda_h */
