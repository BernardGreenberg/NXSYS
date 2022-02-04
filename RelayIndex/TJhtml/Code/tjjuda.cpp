//
//  tjjuda.cpp
//  TJhtml
//
//  Created by Bernard Greenberg on 1/14/22.
//

#include "tjdcls.h"
#include "tjjuda.h"
#import <memory.h>
#import <unordered_map>

#include <locale> //Need for codecvt to work.
#include <codecvt>
#include "hebdefs.h"
#include "CP437.h"
#include "Unicode437.hpp"

namespace tjjuda {

typedef std::unordered_map<char, const char*> Charmap;
typedef unsigned short DIGK;
typedef std::unordered_map<DIGK, const char*> DigraphMap;
typedef set<unsigned char> CharSet;

#define TJJUDA_SPAN_HEAD "<span dir=\"rtl\" style=\"font-size:150%\">"
inline DIGK MDIGK (const char* s) {
    return ((CU)s[0] << 8) | (CU)s[1];
}

inline DIGK MDIGK (char c1, char c2) {
    return ((CU)c1 << 8) | (CU)c2;
}

/* http://www.cplusplus.com/reference/codecvt/codecvt_utf8_utf16/ */
std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> Utfer;
typedef vector<char16_t> wvec;

static string Linefixie(string);

static const char * defhc_helper(wvec wv) {
    wv.push_back(0);
    string utf8 = Utfer.to_bytes(wv.data());
    return strdup(utf8.c_str());
}
#define HLIST(...) defhc_helper(wvec{__VA_ARGS__})
#define DEFHC(name,...) static const char *HEBCHAR_##name= HLIST(__VA_ARGS__)
#define DEFYC(name,...) static const char *YIDCHAR_##name= HLIST(__VA_ARGS__)

DEFHC(test, 'a', 'b', 'c');

DEFHC(ALEF, HEBREW_LETTER_ALEF);
DEFHC(HETH, HEBREW_LETTER_HET);
DEFHC(FINAL_KAF,HEBREW_LETTER_FINAL_KAF);
DEFHC(SHEVA, HEBREW_POINT_SHEVA);
DEFHC(QOMETS, HEBREW_POINT_QAMATS);
DEFHC(PATAH, HEBREW_POINT_PATAH);
DEFHC(SEGOL, HEBREW_POINT_SEGOL);
DEFHC(HIRIQ, HEBREW_POINT_HIRIQ);
DEFHC(QUBUTS, HEBREW_POINT_QUBUTS);
DEFHC(TSERE, HEBREW_POINT_TSERE);
DEFHC(HOLAM, HEBREW_POINT_HOLAM);

DEFHC(VAV_MAPIQ, HEBREW_LETTER_VAV, HEBREW_POINT_DAGESH_OR_MAPIQ);
DEFHC(VAV_HOLAM, HEBREW_LETTER_VAV, HEBREW_POINT_HOLAM);
DEFHC(DAGESH, HEBREW_POINT_DAGESH_OR_MAPIQ);


DEFYC(AYY, HEBREW_LIGATURE_YIDDISH_DOUBLE_YOD, HEBREW_POINT_PATAH);
DEFYC(YY, HEBREW_LIGATURE_YIDDISH_DOUBLE_YOD);
DEFYC(VV, HEBREW_LIGATURE_YIDDISH_DOUBLE_VAV);
DEFYC(OY, HEBREW_LETTER_VAV, HEBREW_LETTER_YOD);


Charmap HebMap = {
    {CP437_APOSTROPHE, "א"}, //Special declaration to avoid confusing Xcode editor.
    {'B', HLIST(HEBREW_LETTER_BET, HEBREW_POINT_DAGESH_OR_MAPIQ)},
    {'V', "ב"},
    {'G', "ג"},
    {'D', "ד"},
    {'H', "ה"},
    {'W', "ו"},
    {'Z', "ז"},
    {'X', "ח"},
    {'T', "ט"},
    {'Y', "י"},
    {'K', "כּ"},
    {'L', "ל"},
    {'M', "מ"},
    {'N', "נ"},
    {'S', "ס"},
    {'P', "פּ"},
    {'F', "פ"},
    {CP437_BACKQUOTE, "ע"}, //Special declaration to avoid confusing Xcode editor.
    {'Q', "ק"},
    {'R', "ר"},
    
    {CP437_OCIRCUM, HEBCHAR_QOMETS},
    {CP437_UCIRCUM, HEBCHAR_QUBUTS},
    {'A', HEBCHAR_PATAH},
    {'E', HEBCHAR_SEGOL},
    {'I', HEBCHAR_HIRIQ},
    {'O', HEBCHAR_VAV_HOLAM},
    {'U', HEBCHAR_VAV_MAPIQ},
    {CP437_AUML, HEBCHAR_TSERE},
    {CP437_OGRAVE, HEBCHAR_HOLAM},
    {CP437_MIDDLE_DOT, HEBCHAR_SHEVA}
};

CharSet HebVowelSet = {
    'A', 'E', 'I', 'O', 'U',
    CP437_AUML,
    CP437_OCIRCUM,
    CP437_UCIRCUM,
    CP437_OGRAVE,
    CP437_MIDDLE_DOT
};

CharSet NPMakeWawSet = {'O', 'U'};

DigraphMap HebDigMap = {
    {MDIGK("TT"), "תּ"},
    {MDIGK("TH"), "ת"},
    {MDIGK("CH"), "כ"},
    {MDIGK("SH"), "שׁ"},
    {MDIGK("SZ"), "ש"},
    {MDIGK("TS"), "צ"},
    {MDIGK("TZ"), "צ"},
    {MDIGK("AX"), HLIST(HEBREW_LETTER_HET, HEBREW_POINT_PATAH)}
};

DigraphMap HebDigVowelMap = {
    {MDIGK("E|"), HLIST(HEBREW_POINT_HATAF_SEGOL)},
    {MDIGK(CP437_OCIRCUM, '|'), HLIST(HEBREW_POINT_HATAF_QAMATS)},
    {MDIGK("A|"), HLIST(HEBREW_POINT_HATAF_PATAH)},
    // Hataf tsere doesn't seem to be a real character.
    {MDIGK(CP437_AUML, '|'), HLIST(HEBREW_POINT_TSERE)}
};

DigraphMap FinalDigraphMap = {
    {MDIGK("CH"), HLIST(HEBREW_LETTER_FINAL_KAF)},
    {MDIGK("TS"), HLIST(HEBREW_LETTER_FINAL_TSADI)},
    {MDIGK("TZ"), HLIST(HEBREW_LETTER_FINAL_TSADI)}
};
static const DIGK FurtivePatahDigk = MDIGK("AX");
static const DIGK ChofDigk = MDIGK("CH");

Charmap FinalForms = {
    {'M', "ם"},
    {'N', "ן"},
    {'K', "ך"},
    {'F', HLIST(HEBREW_LETTER_FINAL_PE)},
    {'P', HLIST(HEBREW_LETTER_FINAL_PE, HEBREW_POINT_DAGESH_OR_MAPIQ)}
};

Charmap YidMap = {
    {'A', "א"},
    {'O', "אָ"},
    {'B', "ב"},
    {'V', "ב"},
    {'G', "ג"},
    {'D', "ד"},
    {'J', HLIST(HEBREW_LETTER_YOD)},
    {'I', HLIST(HEBREW_LETTER_YOD)},
    {'E', HLIST(HEBREW_LETTER_AYIN)},
    {'H', "ה"},
    {'W', YIDCHAR_VV},
    {'Z', "ז"},
    {'X', "ח"},
    {'T', "ט"},
    {'Y', "י"},
    {'L', "ל"},
    {'M', "מ"},
    {'N', "נ"},
    {'S', "ס"},
    {'P', "פּ"},
    {'F', HLIST(HEBREW_LETTER_PE, HEBREW_POINT_RAFE)},
    {'K', "ק"},
    {'R', "ר"},
    {'U', HLIST(HEBREW_LETTER_VAV)}, /*no mapiq */
    
    {CP437_MIDDLE_DOT, "ְ"},
};

DigraphMap YidDigVowelMap = {
    {MDIGK("AY"), YIDCHAR_AYY},
    {MDIGK("AJ"), YIDCHAR_AYY},
    {MDIGK("AI"), YIDCHAR_AYY},
    {MDIGK("EI"), YIDCHAR_AYY},
    {MDIGK("EY"), YIDCHAR_YY},
    {MDIGK("EJ"), YIDCHAR_YY},
    {MDIGK("OY"), YIDCHAR_OY},
    {MDIGK("OJ"), YIDCHAR_OY},
    {MDIGK("OI"), YIDCHAR_OY},
};

DigraphMap YidDigMap = {
    {MDIGK("TH"), "ת"},
    {MDIGK("CH"), "כ"},
    {MDIGK("SH"), "ש"},
    {MDIGK("SC"), "ש"},
    {MDIGK("TS"), "צ"},
    {MDIGK("TZ"), "צ"}
};

bool final_boundary(const string& s, int i) {
    if (i >= s.length())
        return true;
    char si = s[i];
    if (HebMap.count(si))
        return false;
    if (si == '/')
        return false;
    if (si == '*')
        return false;
    
    return true;
}

void DBNextWord (const string& s, int i) {
    string candidate = s.substr(i);
    size_t spx = candidate.find(' ');
    if (spx!=std::string::npos)
        candidate = candidate.substr(0, spx);
    printf("WORD %s\n", CP437toUTF8String(candidate).c_str());
}

static string hebrew1b(string s, bool pointed) {
    if (s[0] == CP437_PILCROW)
        s = s.substr(2);
    string output;
    stdupr(s);
//    DBNextWord(s, 0);
    bool wordstart = true;
    bool vowelled_state = true;
    size_t Slen = s.length();
    for (int i = 0; i < Slen; i++) {
        char c = s[i];
        char nc = (i == Slen-1) ? 0 : s[i+1];
        DIGK digk = MDIGK(c, nc);

        bool at_furtive_patah = (digk == FurtivePatahDigk);
        bool insert_alef = HebVowelSet.count(c) && (wordstart ||vowelled_state) && !at_furtive_patah;
        if (insert_alef) {
//            printf ("insert_alef: char = %s, ws =%d, vws = %d\n", DB437C(c), wordstart, vowelled_state);
            output += "א";
            vowelled_state = false;
        }
        bool false_furtive = at_furtive_patah && !final_boundary(s, i+2);
        if (HebDigMap.count(digk) && !false_furtive) {
            if (pointed && !vowelled_state &&!wordstart) {
                output += HEBCHAR_SHEVA;
                vowelled_state = true;
                wordstart = false;
            }
            if (digk == ChofDigk) {
                if (final_boundary(s, i+2)) { /* final chof, not qomets */
                    output += HEBCHAR_FINAL_KAF;
                    output += HEBCHAR_SHEVA;
                }
                else if (((CU)s[i+2]) == CP437_OCIRCUM && final_boundary(s, i+3))
                    output += HEBCHAR_FINAL_KAF;
                    /* the qomets will come up next and just work */
                else  /* regular chof, non-final*/
                    output += HebDigMap[digk];
            }
            else if (FinalDigraphMap.count(digk) && final_boundary(s, i+2)) {
                output += FinalDigraphMap[digk] ;
            }
            else if (at_furtive_patah && !pointed) {
                output += HEBCHAR_HETH;
            }
            else
                output += HebDigMap[digk];
            i++;                 /* It's a Digraph, no matter what */
            vowelled_state = false;
            wordstart = false;
        } else if (HebDigVowelMap.count(digk)) {
            if (pointed)
                output += HebDigVowelMap[digk];
            i++;
            vowelled_state = true;
            wordstart = false;
        } else if (HebMap.count(c)) {
            bool isvowel = (HebVowelSet.count(c) != 0);
//            printf ("%s isv %d vws %d\n", DB437C(c), isvowel, vowelled_state);
            bool last_alef = (i > 0) && (s[i-1] == CP437_APOSTROPHE);
            if (!isvowel && pointed && !vowelled_state && !last_alef && !wordstart) {
//                printf ("Insert schwa\n");
                output += HEBCHAR_SHEVA;
            }
            if (final_boundary(s, i+1) && FinalForms.count(c)) {
                output += FinalForms[c];
            }
            else {
                if (!pointed && isvowel && NPMakeWawSet.count(c))
                    output += HebMap['W'];
                if (!(isvowel && !pointed))
                    output += HebMap[c];
                if (nc != 0 && nc == c && !isvowel) {
                    if (pointed)
                        output += HEBCHAR_DAGESH;
                    i++;
                }

                if (c == 'Y')
                    isvowel = (i > 0 && (s[i-1] == 'I' || (CU)s[i-1] == CP437_AUML));
            }
            vowelled_state = isvowel;
            wordstart = false;
        } else {
            if ((CU)c == CP437_SUPERSCRIPT_TWO) {
                output += HEBCHAR_DAGESH;
            }
            else if (c == '*')
                vowelled_state = true;
            else {
                /* Default non-Hebrew case */
                
//                DBNextWord(s, i+1);
                output += c;

                vowelled_state = false;
                wordstart = true;
            }
        }
    }
    return output;
}

const string hebrew1(const string s, bool pointed) {
    return Linefixie(hebrew1b(s, pointed));
}

static const string isolate_hebrew_insertion(const string& s, int i) {
    for (int j = i; j < s.length(); j++) {
        unsigned char sj = s[j];
        if (sj == '/')
            return s.substr(i, j-i + 1);
        if (!(HebMap.count(sj) || sj == '*' || sj == CP437_SUPERSCRIPT_TWO))
            return s.substr(i, j-i);
    }
    return s.substr(i);
}

static const DIGK digk_SC = MDIGK("SC");
static const DIGK digk_CH = MDIGK("CH");

static const string yiddish1b(string s) {
    if (s[0] == CP437_PILCROW)
        s = s.substr(2);
    string output;
    stdupr(s);
    bool wordstart = true;
    size_t Slen = s.length();
    for (int i = 0; i < Slen; i++) {
        char c = s[i];
        if (c == '/') {
            wordstart = false;
            continue;
        }
        if (c == '#' || c == '^') {
            string hebrew_ins = isolate_hebrew_insertion(s, i+1);
            i += hebrew_ins.length() + 1 - 1;
            string hcv = hebrew1b(hebrew_ins, c == '^');
            size_t hcvl = hcv.length();
            if (hcv[hcvl-1] == '/')
                hcv = hcv.substr(0, hcvl-1);
            output += hcv;
            continue;
        }

        char nextc = (i == Slen-1) ? 0 : s[i+1];
        DIGK digk = MDIGK(c, nextc);

        if (YidDigVowelMap.count(digk)) {
            if (wordstart)
                output += HEBCHAR_ALEF;
            i++;
            wordstart = false;
            output += YidDigVowelMap[digk];
        }
        else if (YidDigMap.count(digk)) {
            if (digk == digk_SC) {
                if (s[i+2] == 'H')
                    i+=1;
            } else if (FinalDigraphMap.count(digk) && final_boundary(s, i+2)) {
                output += FinalDigraphMap[digk];
                i++;
                wordstart = false;
                continue;
            }
            output += YidDigMap[digk];
            i++;
            wordstart = false;
        } else  if (YidMap.count(c)) {
            if (final_boundary(s, i+1) && FinalForms.count(c) && c != 'K') {
                output += FinalForms[c];
                wordstart = true;
            }
            else {
                if (wordstart && strchr("IU", c))
                    output += HEBCHAR_ALEF;
                output += YidMap[c];
                wordstart = false;
            }
        }
        else if (c == '*' || c == '|') {}
        else {
            if (c != CP437_APOSTROPHE)
                output += c;
            wordstart = true;
        }
    }
    return output;
}

const string yiddish1(const string s) {
    return  Linefixie(yiddish1b(s));
}

static string convl_nbsp(string l) {
    string out;
    for (int i = 0; i < l.length(); i++)
        if (l[i] == ' ')
            out += "&nbsp;";
        else
            return out + l.substr(i);
    return l;
}

static string Linefixie(string s){
    vector<string> lines;
    while (true) {
        auto lfpos = s.find('\n');
        if (lfpos == string::npos) {
            lines.push_back(s);
            break;
        }
        lines.push_back(s.substr(0, lfpos));
        s = s.substr(lfpos+1);
    }
    
    if (lines.size() == 1)
        return string(TJJUDA_SPAN_HEAD) + lines[0] + "</span>";

    string output = "<div dir=\"rtl\" style=\"text-align:right;margin-bottom:0;margin-top:0;\">\n";
    int i = 0;
    for (auto l : lines) {
        if (l.length() && l[0] == ' ')
            l = convl_nbsp(l);
        output += l;
        if (i++ < lines.size() - 1)
            output += "<br/>\n";
    }
    output += "</div>\n";
    return output;
}

}; /* namespace tjjuda */
