//
//  macros.cpp
//  TJhtml
//
//  Created by Bernard Greenberg on 1/16/22.
//

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include "CP437.h"
#include "tjdcls.h"
#include "tjio.h"

struct Macro {
    Macro(string id) {
        File = id;
        has_body_parm = false;
    }
    void PrepareParams() {
        for (auto& param : Params) {
            stdlwr(param);
            for (auto c : param)
                if (!isalnum(c))
                    fabort("Non-alphanum param name in macro %s: %s", Name.c_str(), param.c_str());
        }
        if (Params.size())
            has_body_parm = (Params.back() == "body");
    }
    string Name;
    string File;
    vector<string> Params;
    string DefinitionBody;
    bool has_body_parm;
};

struct MacroInvocation {
    MacroInvocation(::Macro* m) : Macro(m) {}
    void BindArgs (int cx);
    string GetBodyValue() {
        assert (Macro->has_body_parm);
        return Args.back();
    }
    vector<string> Args;
    Macro * Macro;
};

static map<string, Macro*> Macros;
static vector<MacroInvocation> Stack;

void defmacro (int cx) {
    Macro* m = new Macro(I->id());
    char argdelim = collect_name (m->Name, ' ');
    if (argdelim != '=') {
        m->Params = collect_variadic_args(get_matching_delim (argdelim));
        m->PrepareParams();
        if (I->getc() != '=')
            fabort ("Equal-sign missing for macro %s\n", m->Name.c_str());
    }

    m->DefinitionBody = collect_arg(cx, cx, true);
    Macros[m->Name] = m;
}

const string GetVarValue(const string& varname) {
    for (auto MI = Stack.rbegin(); MI != Stack.rend(); MI++) {
        auto& params = (*MI).Macro->Params;
        for (auto param = params.begin(); param != params.end(); param++)
            if (varname == *param)
                return (*MI).Args[param - params.begin()];
    }
    fabort("Can't find macro param named %s in macro stack.\n", varname.c_str());
    return ""; /* fabort aborts, though...*/
}

void OutputAllegedMacroCallBody(int cx) {
    collect_arg(cx);  // discard ()
    StringInputSource SIS(Stack.back().GetBodyValue());
    run_pushed_source(&SIS);
}

const string MacroSubstitute(const string& input) {
    string output;
    StringInputSource K (input);
    while (true) {
        int c = K.getc();
        if (c == EOF)
            break;
        if (c == CP437_SMILEY) {
            string varname;
            while (true) {
                c = K.getc();
                if (c == EOF || not (isalnum(c))) {
                    K.ungetc(c);
                    break;
                }
                varname += c;
            }
            output += GetVarValue(varname);
        } else
            output += c;
    }
    return output;
}

void MacroInvocation::BindArgs(int cx) {
    for (int i = 0; i < int(Macro->Params.size())-1; i++) //must be int to get -1 for 0
        Args.push_back(collect_arg(','));
    // Called in 0-arg (  ()   ) case, too.  Creates null Arg with no corresp, param.
    Args.push_back(collect_arg(cx));  // last arg always needs CX
}

void InvokeMacro(Macro* m, char cx) {
    Stack.emplace_back(m);
    Stack.back().BindArgs(cx);
    StringInputSource SIS(MacroSubstitute(m->DefinitionBody));
    run_pushed_source(&SIS);
    Stack.pop_back();
}


bool InterpretPossibleMacroCommand(const string& cmd, char cx) {
    if (cmd == "defmac" || cmd == "defmacro")
        defmacro(cx);
    else if (cmd == "body")
        OutputAllegedMacroCallBody(cx);
    else if (Macros.count(cmd))
        InvokeMacro(Macros[cmd], cx);
    else return false;
    
    return true;
}
