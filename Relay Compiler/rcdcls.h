#ifndef _RELAY_COMPILER_DCLS__
#define _RELAY_COMPILER_DCLS__

#include <stdarg.h>
#include "lisp.h"

typedef unsigned int RLID;
typedef unsigned int PCTR;

struct Jtag {
    Jtag() {
        pctr = tramp_pc = 0;
        memset(lab, 0, sizeof(lab));
        memset(tramp_lab, 0, sizeof(lab));
        defined = tramp_pc = tramp_defined = false;
    }
    PCTR pctr, tramp_pc;
    char lab[10];
    char tramp_lab[10];
    bool defined, have_pc, tramp_defined;
};

struct Timer {
    Timer (RLID id_, int time_) : id(id_), time(time_) {}
    RLID id;
    int time;
};

struct DepPair {
    DepPair (RLID tor, RLID ted) : affector(tor), affected(ted) {}
    RLID affector;
    RLID affected;
};

struct Fixup {
    Fixup(Jtag * t_, PCTR p_, short w_) : tag(t_), pc(p_), width(w_) {}
    Jtag * tag;
    PCTR pc;
    short width;
  };

struct RelayDef {
    RelayDef(Rlysym* s_, PCTR p_) : sym(s_), pc(p_), size(-1){}
    Rlysym *sym;
    PCTR pc;
    int size;
};

struct LabelEntry {
    LabelEntry(Sexpr s_, Sexpr sv_) : s(s_), Svalue(sv_) {}
    Sexpr s;
    Sexpr Svalue;
};
void RC_error (int fatal, const char* s, ...);

struct _TKO_INFO {
    long   time;			/* no C */
    struct RelayDef * Isd;
    short isd_count;
    struct Fixup * Fxt;
    int fixup_count;
    char unsigned * Code;
    struct DepPair * Dpt;
    int dpt_count;
    Rlysym** Esd;
    Timer * Tmr;
    const char** Ats;
    char unsigned * Frm;
    int tmr_count;
    int esd_count;
    int unsigned static_len;
    int frm_count;
    int ats_count;
    int unsigned code_len;
    int unsigned code_ct;
    int unsigned code_item_len;
    int bits;
    const char * Architecture;
    int arch_characterization;
    int compiler_version;
    const char * compiler;
};

typedef struct _TKO_INFO TKO_INFO;


void write_tko (const char * fname, TKO_INFO& info);
void write_tko32 (const char * fname, TKO_INFO& info);
void fasd_init (), fasd_finish();
void fasd_form (Sexpr);
unsigned char * fasd_data (int &fasd_count);
const char* * fasd_atsym_data (int &fasd_atsym_count);
PCTR RlsymOffset (Rlysym * r);
RLID RelayId (Sexpr s);
void list(const char *, ...);

#include <vector>

using CodeByte = unsigned char;

class CodeVector : public std::vector<CodeByte> {
    public:
    CodeVector() {
        reserve(12);
    }
    void Append(const std::vector<CodeByte> addendum) {
        insert(end(), addendum.begin(), addendum.end());
    }
    void operator += (const std::vector<CodeByte> addendum) {
        insert(end(), addendum.begin(), addendum.end());
    }
    void operator += (CodeByte c) {
        push_back(c);
    }
};

extern CodeVector Code;

#endif
