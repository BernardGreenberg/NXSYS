//
//  tjdcls.h
//  TJhtml
//
//  Created by Bernard Greenberg on 1/14/22.
//

#ifndef tjdcls_h
#define tjdcls_h

#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <map>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define ENVDELIM ';'
#else
#define ENVDELIM ':'
#endif

using std::string, std::vector, std::set, std::map;

#define CONTROL_Z 0x1A
#define RUBOUT_ESC 0xFF

#define UTF8_INVERSE_SMILEY "☻"
#define UTF8_SMILEY "☺"
#define UTF8_RAW_SNOUT "🐽"
#define UTF8_SUBSTITUTE "\uFFFD"

typedef char unsigned CU;

enum Fstate {FSTATE_NORMAL=0, FSTATE_ITALIC=1, FSTATE_BOLD=2, FSTATE_UNDERLINE=3};

char get_matching_delim (char c);
bool InterpretPossibleMacroCommand(const string&, char cx);

inline void stdlwr (string& data) {
    std::transform(data.begin(), data.end(), data.begin(),
                   [](CU c){ return std::tolower(c); });
}

inline void stdupr (string& data) {
    std::transform(data.begin(), data.end(), data.begin(),
                   [](CU c){ return std::toupper(c); });
}
char collect_arg (string& result, char xc, char xc2 = 0, bool no_iq = false);
string collect_arg(char xc, char xc2 = 0, bool no_iq = false);
char collect_name (string& result,  int c);
vector<string> collect_variadic_args(int cx);
void fndump();
extern int PageWidth;
void fabort(const char* fmt, ...);

template<class Input, class Transformer>
string string_accumulate(Input I, Transformer op)
{
    string _val;
    for (auto a : I) {
        _val += op(a);
    }
    return _val;
}

#endif /* tjdcls_h */
