//
//  argparse.cpp
//  TJhtml
//
//  Created by Bernard Greenberg on 1/22/22.
//

#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <filesystem>
#include <memory>

#include "argparse.hpp"

constexpr short MAX_ARGS = 32767;
constexpr auto DIGITS = "0123456789";
constexpr auto HELP_COLUMN_WIDTH = 27;

using std::string;
using std::vector;
using std::map;
using std::set;
using std::unique_ptr;

typedef std::initializer_list<std::string> strarray_ilist;

namespace argparse {

template <class IC>
vector<IC*> pointer_vector (vector<IC>& input) {
    vector<IC*> output;
    for (auto& element : input)
        output.push_back(&element);
    return output;
}

template <class IC, class Fcn>
vector<IC*> pointer_vector_if (vector<IC>& input, Fcn predicate) {
    vector<IC*> output;
    for (auto& element : input)
        if (predicate(element))
            output.push_back(&element);
    return output;
}

template <class IC>
vector<IC*> U_deref_pointer_vector (vector<unique_ptr<IC> >& input) {
    vector<IC*> output;
    for (auto& element : input)
        output.push_back(element.get());
    return output;
}

class ArgDesc {
public:
    ArgDesc(class argset_i*, strarray_ilist&);
    vector<string> Params;
    bool optional;
    bool boolean;
    bool single_char;
    bool double_dash;
    bool positional;
    bool variadic;
    vector<char> chars;
    short v_min, v_max;
    int SortPrio;
    string Help;
    string Name;
    string Metavar;
    map<string, string> KeywordMap;
    void PreValidate();
    void ComputeKeywordMap();
    void Setup(class argset_i*);
    void SetupVariadicParms(string nargs_spec);
    void def_err(const char *, ...);
    string HelpLine() const;
};

class argset_i {
public:
    argset_i (string help) : Help(help) {}
    vector<unique_ptr<ArgDesc>> ArgDescs;
    vector<ArgDesc*> PositionalArgs;
    map<string, ArgDesc*> NameMap;
    map<char, ArgDesc*> SingleCharMap;
    string CommandName;
    string Help;
    void emplacer(strarray_ilist);
    ParsedArgs Parse(int argc, const char**argv);
    int collect_variadic_arg(ParsedArgs&, ArgDesc&, int i, int argc, const char **argv);

private:
    string GetNextArg(int i, int argc, const char**argv, ArgDesc* pAD);
    void uerr(const char* fmt, ...);
    void uerr2(const char* fmt, const string& arg) {
        uerr(fmt, arg.c_str());
    }
    void HelpMessage();
    void UsageMessage();
};

set<string> ArgKwds({"boolean", "help", "required", "optional", "metavar", "nargs"});

void argset_i::uerr(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    UsageMessage();
    vfprintf(stderr, fmt, ap);
    exit(4);
}

ArgSet::ArgSet (string help) {
    asp = new argset_i(help);
    def_arg(*this, {"--help", "-h", "-?", "help=Print this message.", "boolean="});
}

ArgSet::ArgSet(string help, std::initializer_list<strarray_ilist> llist) : ArgSet(help){
    for (auto& ilist : llist)
        def_arg(*this, ilist);
}

ArgSet::~ArgSet() {
    delete(asp);
}

void def_arg(class ArgSet& as, strarray_ilist ilist) {
    as.asp->emplacer(ilist);
}

void argset_i::emplacer(strarray_ilist ilist) {
    ArgDescs.emplace_back(new ArgDesc(this, ilist));
}



void ArgDesc::def_err(const char* fmt, ...) {
    fprintf(stderr, "argparse: command argument definition error:\n");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    string line = "arg spec: {";
    int i = 0;
    for (auto s : Params) {
        line += "\"" + s + "\"";
        if (++i < Params.size())
            line += ", ";
    }
    line += "}";
    fprintf(stderr, "%s\n", line.c_str());
    exit(5);
}

void ArgDesc::ComputeKeywordMap() {
    for (auto arg : Params) {
        auto eqp = arg.find('=');
        if (eqp != string::npos) {
            string kwd = arg.substr(0, eqp);
            if (ArgKwds.count(kwd)) {
                if (KeywordMap.count(kwd))
                    def_err("Duplicated keyword arg in arg spec: %s\n", kwd.c_str());
                KeywordMap[kwd] = arg.substr(eqp+1);
            }
            else
                def_err("Unknown arg description keyword: %s=\n", kwd.c_str());
        }
    }
}

void ArgDesc::PreValidate () {
    if (Params.size() == 0)
        def_err("Empty argument spec given; no descriptors.\n");
    for (auto& arg : Params) {
        if (arg == "")
            def_err("Empty string given in arg spec.\n");
        if (arg == "-" || arg == "--")
            def_err("- or -- without content given a descriptor arg spec.\n");
        if (arg[0] == '=')
            def_err("Arg descriptor can't start with =.\n");
    }
}


ArgDesc::ArgDesc(class argset_i* asp, strarray_ilist &ilist) : Params(ilist){

    PreValidate();

    ComputeKeywordMap();
    
    optional = KeywordMap.count("optional");
    boolean = KeywordMap.count("boolean");
    
    if (KeywordMap.count("nargs"))
        SetupVariadicParms(KeywordMap["nargs"]); // can set "optional"
    else {
        v_min = v_max = 0;
        variadic = false;
    }

    if (KeywordMap.count("help"))
        Help = KeywordMap["help"];
    else
        def_err("Help string missing in argument spec.\n");

    if (KeywordMap.count("metavar"))
        Metavar = KeywordMap["metavar"];
    else if ((single_char || double_dash) && !boolean)
        Metavar = "arg";

    Setup(asp);
}


void ArgDesc::Setup(argset_i* asp) {

    single_char = false;
    double_dash = false;
    positional = false;

    bool req = KeywordMap.count("required");

    for (auto& param : Params) {
        if (param.length() > 2 && param[0] == '-' && param[1] == '-') {
            if (double_dash)
                def_err("Multiple definition: --%s %s\n", Name.c_str(), param.c_str());
            if (Name.length())
                def_err ("Multiple name fields: %s %s\n", Name.c_str(), param.c_str());
            Name = param.substr(2);
            SortPrio = 3;
            double_dash = true;
            optional = !req;
            asp->NameMap[Name] = this;
        }
        else if (param.length() == 2 && param[0] == '-') {
            char c = param[1];
            chars.push_back(c);
            single_char = true;
            SortPrio = 3;
            optional = !req;
            asp->SingleCharMap[c] = this; //allow multiple
        } else if (param[0] == '-')
            def_err("Invalid argument spec element: %s\n", param.c_str());
        else if (param.find('=') != string::npos) {}
        else {
            if (Name.length())
                def_err ("Multiple name fields: %s %s\n", Name.c_str(), param.c_str());
            Name = param;
        }
    }
    if (Name.length() == 0)
        def_err("Arg definition with neither --name nor unqualified name\n");
    if (!(double_dash || single_char)) {
        positional = true;
        asp->PositionalArgs.push_back(this);
        SortPrio = optional ? 2 : 1;
    }
}

void ArgDesc::SetupVariadicParms (string spec) {
    variadic = true;
    auto compos = spec.find_first_of(",");
    if (compos != string::npos) {
        auto first = spec.substr(0,compos);
        if (first.find_first_not_of(DIGITS) != string::npos)
            def_err("Bad nargs min spec: %s\n", first.c_str());
        v_min = atoi(first.c_str());
        auto second = spec.substr(compos+1);
        if (second.find_first_not_of(DIGITS)!= string::npos)
            def_err("Bad nargs max spec: %s.\n", second.c_str());
        v_max = atoi(second.c_str());
        if (v_min > v_max)
            def_err("nargs %d,%d out of order.\n", v_min, v_max);
    }
    else {
        if (spec == "+") {
            v_min = 1;
            v_max = MAX_ARGS;
        } else if (spec == "?") {
            v_min= 0;
            v_max = 1;
        } else if (spec == "*") {
            v_min = 0;
            v_max = MAX_ARGS;
        } else if (spec.find_first_not_of(DIGITS) != string::npos)
            def_err("Bad nargs spec: %s\n", spec.c_str());
        else
            v_min = v_max = atoi(spec.c_str());
    }
    if (v_min == 0)
        optional = true;
}

/* --------------------------------------------------------------------------  */
/*                                                                             */
/*               Run time                                                      */
/*                                                                             */
/* --------------------------------------------------------------------------  */

ParsedArgs ArgSet::Parse(int argc, const char ** argv) {
    return asp->Parse(argc, argv);
}

string argset_i::GetNextArg(int iinc, int argc, const char**argv, ArgDesc* pAD) {
    if (iinc < argc) {
        string nxtarg = argv[iinc];
        if (!((nxtarg.length() && nxtarg[0] == '-')))
            return nxtarg;
    }
    string argdesc;
    if (pAD->single_char)
        argdesc = string("-") + pAD->chars[0];
    else
        argdesc = string("--") + pAD->Name;
    uerr2("Argument value missing after %s.\n", argdesc);
    return "";
}

int argset_i::collect_variadic_arg(ParsedArgs& PA, ArgDesc& D, int i, int argc, const char **argv) {
    vector<string> values;
    while (values.size() < D.v_max) {
        if (i == argc)
            break;
        if (strlen(argv[i]) && argv[i][0] == '-')
            break;
        values.push_back(argv[i]);
        i++;
    }

    if (values.size() < D.v_min)
        uerr("Not enough values for \"%s\": %d required.\n", D.Name.c_str(), D.v_min);
    
    PA.add(D.Name, "*VECTOR*");
    PA.VectorArgs.Add(D.Name, values);
    return (int)values.size();
}

ParsedArgs argset_i::Parse(int argc, const char ** argv) {
    CommandName = std::__fs::filesystem::path(argv[0]).filename().string();
    ParsedArgs PA;
    int unnamed_count = 0;
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg.length() > 2 && arg[0] == '-' && arg[1] == '-') {
            string rarg = arg.substr(2);
            if (NameMap.count(rarg) == 0)
                uerr2("Argument --%s not known.\n", rarg);
            ArgDesc& D = *NameMap[rarg];
            if (D.variadic) {
                int j = i + 1;
                i += collect_variadic_arg(PA, D, j, argc, argv);
            }
            else if (D.boolean)
                PA.add(D.Name, "true");
            else
                PA.add(D.Name, GetNextArg(++i, argc, argv, &D));
        } else if (arg.length() > 1 && arg[0] == '-') {
            char ch = arg[1];
            if (SingleCharMap.count(ch) == 0)
                uerr("Single-character control arg -%c not known.\n", ch);
            ArgDesc& D = *SingleCharMap[ch];
            if (D.variadic) {
                int j = i + 1;
                i += collect_variadic_arg (PA, D, j, argc, argv);
            }
            else if (D.boolean) {
                for (auto c : arg.substr(1))
                    if (SingleCharMap.count(c) && SingleCharMap[c]->boolean)
                        PA.add(SingleCharMap[c]->Name, "true");
                    else
                        uerr("Unknown single-char boolean arg: -%c\n", c);
            }
            else {
                string argval = arg.substr(2);
                if (argval.length() == 0)
                    PA.add(D.Name, GetNextArg(++i, argc, argv, &D));
                else
                    PA.add(D.Name, argval);
            }
        } else {
            /* non-control arg*/
            if (unnamed_count + 1> PositionalArgs.size())
                uerr2("Extra unwanted command arg: %s\n", arg);
            ArgDesc& D = *PositionalArgs[unnamed_count++];
            if (D.variadic)
                i += collect_variadic_arg(PA, D, i, argc, argv) - 1;
            else
                PA.add(D.Name, arg);
        }
    }
    if (PA["help"]) {
        fprintf(stderr, "%s\n", Help.c_str());
        UsageMessage();
        fprintf(stderr, "\n");
        HelpMessage();
        exit(0);
    }
    for (auto& pD : ArgDescs)
        if (!pD->optional && !PA[pD->Name])
            uerr2("Required arg \"%s\" not supplied.\n", pD->Name);
    return PA;
}

static string gen_pad(size_t n) {
    string pad;
    for (int i = 0; i < n; i++)
        pad += ' ';
    return pad;
}

static string gen_overflow_pad () {
    return "\n" + gen_pad(HELP_COLUMN_WIDTH);
}

int UTF8_char_count(const char *s) {
  int i = 0, j = 0;
  while (s[i]) {
    if ((s[i] & 0xc0) != 0x80) j++;
    i++;
  }
  return j;
}

static string PAD_OVERFLOW = gen_overflow_pad();

void argset_i::UsageMessage(){
    string line = "Usage: " + CommandName + " ";
    for (auto& pD : ArgDescs) {
        auto& D = *pD.get();
        if (D.single_char) {
            line += string("[-") + D.chars[0];
            if (D.variadic)
                line += " ...";
            else if (!D.boolean) {
                line += " ";
                line += D.Metavar;
            }
            line += "] ";
        }
    }
    if (PositionalArgs.size() && line.length() > HELP_COLUMN_WIDTH) {
        line += "\n" + gen_pad (8 + CommandName.length());
    }

    for (auto& pD : ArgDescs) {
        auto& D = *pD.get();
        if (D.positional) {
            if (D.variadic){
                if (D.optional)
                    line += string("[") + D.Name + " ...] ";
                else
                    line += D.Name + " ... ";
            }

            else if (D.optional)
                line += string("[") + D.Name + "] ";
            else
                line += D.Name + " ";
        }
    }
    
    fprintf(stderr, "%s\n", line.c_str());
}

void argset_i::HelpMessage() {
    vector<ArgDesc*> ADP = U_deref_pointer_vector(ArgDescs);
    std::sort(ADP.begin(), ADP.end(), [](auto& D1, auto& D2) {return D1->SortPrio < D2->SortPrio;});
    for (auto& D : ADP)
        fprintf(stderr, "%s\n", D->HelpLine().c_str());
}

string ArgDesc::HelpLine() const {
    string line = "   ";
    int i = 0;
    if (single_char) {
        for (auto c : chars) {
            line += "-";
            line += (string() + c);
            if (!boolean)
                line += " " + Metavar;
            if (++i < chars.size())
                line += ", ";
        }
    }

    if (double_dash) {
        if (single_char)
            line += ", ";
        line += "--" + Name;
        if (!boolean)
            line += " " + Metavar;
    }
    
    if (double_dash || single_char) {
        if (!optional)
            line += " [required]";
    }
    else
        line += Name + " ";
    
    if (variadic) {
        string second = v_max == MAX_ARGS ? "∞" : std::to_string(v_max);
        line += "(" + std::to_string(v_min) + "-" + second + ") ";
    }

    if (UTF8_char_count(line.c_str()) > HELP_COLUMN_WIDTH)
        line += PAD_OVERFLOW;
    else
        line += gen_pad(HELP_COLUMN_WIDTH - UTF8_char_count(line.c_str()));

    line += Help;

    return line;
}

}; // namespace argparse

