//
//  Unicode437.cpp
//  TJhtml
//
//  Created by Bernard Greenberg on 1/18/22.
//  All interesting content removed to build-time Make437Table.cpp 1/30-31/2022
//

#include "tjdcls.h"

#include <locale> //Need for codecvt to work.
#include <codecvt>
#include <numeric> /* accumulate */

#include "Unicode437.hpp"
#include "Table437.hpp" //Generated, hopefully...

const char* CP437CharToUTF8CharStar(unsigned char c) {
    return reinterpret_cast<const char*> (Table437ToUTF8[c]);
}

const string CP437toUTF8String(string s) {
    return string_accumulate(s, [](char c)
                             {return reinterpret_cast<const char*> (Table437ToUTF8[(CU)c]);});
}

//https://stackoverflow.com/questions/57639108/difference-between-codecvt-utf8-utf16-and-codecvt-utf8-for-converting-from-u
//std::wstring str = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes("some utf8 string");
//Has to handle 32-bit pig snouts and stuff (even though they're mistakes)
static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

string UnicodeTo437(const string s) {
    return string_accumulate(converter.from_bytes(s),
      [](wchar_t C) {
        if (TableUTF32To437.count(C))
            return (char)TableUTF32To437[C];
        fabort("Unicode437.cpp: Unicode point U+%04X='%s' has no CodePage 437 equivalent.\n",
               C, converter.to_bytes(std::wstring{C}).c_str());
        return '\0';
    });
}


