/*  Last mod in 20th century
   -rwxr-xr-x@ 1 bsg  staff   36807 Jan 18  1999 rlycomp.cpp
*/

/* Relay Compiler June 6-8, 1994, by BSG, -der Alte- */
/* Totally new OR/AND recurse (Ctxt) 4 July 1994 */
/* Dynamically allocate (still at fixed sizes, though) code & DPP & FXU table
   17 December 1994 */
/* 32-bit version 18 April 1996 */
/* 32-bit trampolines fixed 26 October 1996, made to use catbufs. */
/* made to expand paths 18 February 1997 */
/* catbufs -> dynarray's, 18 January 1999 */
/* Exchanged cat/dynarray technology for STL and C++11, consted char *'s,
   replaced vandalizing relay ptrs with longs by an std::unordered_map,
   made to compile on run on 64-bit Macintosh, but uselessly produce and list
   32-bit Windows code.  Sigh.  26 August 2019. */
/* Produce actual arm64 ("Apple Silicon") code, with architecture management 16 December 2024
   This V3 assumes that the static linkage is an array of pointers to relay value cells, not
   actual relays as in V2. */
/* Produce compatible Win/Mac X86-64 code, 3 January 2025 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <filesystem>
#include <limits>
#include <algorithm>
#include <iostream>
#include <format>
#include <unordered_map>
#include <unordered_set>
#include "STLExtensions.h"
#include "replace_filename.h"

using std::string, std::vector, std::unordered_set, std::unordered_map;
using std::cout, std::cerr, std::endl;

namespace fs = std::filesystem;

/* To be done -- 8 June 1994
   upd 28 October 1996
      Error catchers, errors to listing.
      Asm output
      Object file map
     V3 - load-time-fixup extrefs for all relays, dynacreate, multi objfiles.
*/

#include "lisp.h"
#include "rcdcls.h"
#include "RCArm64.h"
#include "opsintel.h"
enum OS {WINDOWS, MAC, UNIVERSAL};

class Architecture {
public:
    Architecture (string ct, vector<string>syn, int bits, OS o, int rbs, const char* de) {
        CanonToken = ct;
        Synonyms = syn;
        Bits = bits;
        Os = o;
        RelayBlockSize = rbs;
        Description = de;
        alist.push_back(this);
    }
    string CanonToken;
    vector<string> Synonyms;
    int Bits;
    enum OS Os;
    int RelayBlockSize;
    const char * Description;
    
    static vector<Architecture*> alist;
    bool IsMe(const string& s) {
        string upc = stoupper(s);
        if (upc == stoupper(CanonToken))
            return true;
        for(const auto& ts : Synonyms)
            if (ts == upc)
                return true;
        return false;
    }
    static Architecture* find(const string item) {
        for (auto ap : alist)
            if (ap->IsMe(item))
                return ap;
        cerr << "Unknown architecture: " << item <<endl;
        return nullptr;
    }
    static void print_known(Architecture* dft) {
        cerr << "Known architectures (default " << dft->CanonToken << "):" << endl;
        for (auto ap : Architecture::alist)
            cerr << ap->CanonToken << " ";
        cerr << endl;
    }
    bool operator == (Architecture* otherp) const {
        return otherp == this;
    }
};

vector<Architecture*> Architecture::alist;

static Architecture Intel16 {"X86-16", {"8086"}, 16, OS::WINDOWS, 28,
"Intel 8086 on MS-Windows"};
static Architecture Intel32 {"X86-32", {"80386", "IA32"}, 32, OS::WINDOWS, 32,
"Intel 32-bit 80x86 on MS-Windows"};
static Architecture macARM {"ARM64", {"ARM64"}, 64, OS::MAC, 8,
"64-bit Apple M1/...Mn on macOS"};
static Architecture Intel64 {"X86-64", {"X86"}, 64, OS::UNIVERSAL, 8,
"64-bit Intel X86 on Apple macOS or MS-Windows (compatible)"};



static Architecture* Arch;
#define IS_ARM64 (macARM == Arch)

#define COMPILER_VERSION 3
#define COMPILER_COPYRIGHT "Copyright (c) Bernard S. Greenberg 1994, 1996, 2019, 2024, 2025"

#define WINDOWS_ENTRY_THUNK_NAME "_windows_entry_thunk"
#define MAC_ENTRY_THUNK_NAME "_macos_entry_thunk"

/* Variables that change value for architecture */
#define SINGLE_WIDTH 1

static int FullWidth;
static const char * Ltabs;
int Ahex;
ArmInst insert_arm_bitfield(ArmInst inst, int displacement, int start_bit, int end_bit, int shift_down);
void verify_arm_bitfield_zero(ArmInst inst, int start_bit, int end_bit, int shift_down, PCTR target);

enum REG_X {X_AL = 0, X_CL, X_DL, X_BL, X_AH, X_CH, X_DY, X_BH, X_NONE,
    X_EAX= 0, X_ECX,X_EDX,X_EBX,X_ESP, X_EBP,X_ESI,X_EDI,
    X_RAX=16,  X_RCX, X_RDX, X_RBX,X_RSP, X_RBP,X_RSI, X_RDI,
    X_R8, X_R9, X_R10, X_R11, X_R12, X_R13, X_R14, X_R15};
enum OP_MOD {OPMOD_RPTR = 0, OPMOD_RP_DISP8, OPMOD_RP_DISPLONG, OPMOD_IMMED};
enum OPREG16 {OR16_BXSI=0, OR16_BXDI, OR16_BPSI, OR16_BPDI,
	    OR16_SI, OR16_DI, OR16_BP, OR16_BX, OR16_NONE};
enum OPREG16 MAPMOD1632[] = {OR16_NONE, OR16_NONE, OR16_NONE, OR16_BX,
			 OR16_NONE, OR16_NONE, OR16_SI, OR16_DI};
enum REG_X   MAPMOD3216[] = {X_NONE, X_NONE, X_NONE, X_NONE,
			     X_ESI, X_EDI, X_NONE, X_EBX};


static struct OPDEF Ops[] INTEL_OP_INFO_DATA;

const char *REG_NAMES[3][24]
   =
    { {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "???"},
        {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
            "rax", "rcx",  "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
            "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"},
      {"al", "cl", "dl", "bl", "ah", "dh", "ch", "bh", "???"}};

static enum MACH_OP RetOp;

/* global */
CodeVector Code;
PCTR Pctr = 0;

static int GensymCtr = 0;
static PCTR Lowest_Fix8_Unresolved;

static vector<RelayDef> RelayDefTable;
static unordered_set<Rlysym*> RelayDefQuickCheck;

//These two maps are inverses of each other
vector<Rlysym*> RelayRefTable ;
static unordered_map<Rlysym*, RLID> RIDMap;

static vector<Timer> Timers ;

static vector<DepPair> DependentPairTable;
static vector<struct Fixup> FixupTable;
static vector<LabelEntry> LabelTable;

static bool TraceOpt = false;
static bool CheckOpt = false;
static bool ListOpt = false;
static FILE *ListFile;
static char Label_Pending[20] = {0};
static Jtag RetCF, RetOne, RetZero;

Sexpr read_sexp (FILE * f);

DEFLSYM(AND);
DEFLSYM(OR);
DEFLSYM(NOT);
DEFLSYM2(T_ATOM,T);
DEFLSYM(LABEL);
DEFLSYM(RELAY);
DEFLSYM(LRET);
DEFLSYM(TIMER);
DEFLSYM(ZRET);
DEFLSYM(TRET);

DEFLSYM(INCLUDE);
DEFLSYM(COMMENT);
DEFLSYM2(EVAL_WHEN,EVAL-WHEN);
DEFLSYM(LOAD);

enum _Ctxt_Op {CT_VAL, CT_OR, CT_AND};
static char CTVAL[6] = "VOA??";
typedef enum _Ctxt_Op CtxtOp;

/*should be .h, but needs too much help */
void ARM64FixupFixup (Fixup & F, PCTR pc);
void OutputARMFunctionPrologue();
void outinst_raw_arm(MACH_OP op, const char * str, PCTR opd);

struct _Ctxt {
    _Ctxt(Jtag* j_, CtxtOp o_) : tag(j_), op(o_) {}
    Jtag * tag;
    CtxtOp op;
};


static bool dep_sorter (const DepPair& A, const DepPair& B) {
    if (A.affector < B.affector)
    return true;
    if (A.affector > B.affector)
    return false;
    if (A.affected < B.affected)
    return true;
    if (A.affected > B.affected)
        return false;
    return false;
}


void RC_error (int fatal, const char* s, ...) {
    va_list ap;
    va_start (ap, s);
    vfprintf (stderr, s, ap);
    fprintf(stderr, "\n");
    if (fatal)
	exit(3);
}

size_t get_file_size(const std::string& fname) {
    struct stat st;

    if (stat(fname.c_str(), &st) == 0)
        return st.st_size;
    else
        return -1;
}

//extern used in armgen stuff
// __GNUC__ arg checking in rcdcls.h
void list (const char* s, ...){
    if (ListOpt) {
	va_list ap;
	va_start (ap, s);
	vfprintf (ListFile, s, ap);
    }
}


void RecordFixup (Jtag& tag, PCTR pc, int width) {

    if (TraceOpt && ListOpt) {
	list (";***RecordFixup pc %X width = %d\n", pc, width);
	list ("; lowest fix8unr = %X\n", Lowest_Fix8_Unresolved);
    }
    FixupTable.emplace_back(&tag, pc, width);
    if (width == SINGLE_WIDTH) {
	if (pc < Lowest_Fix8_Unresolved) {
	    Lowest_Fix8_Unresolved = pc;
	    if (TraceOpt && ListOpt)
		list (";***RecordFixup sets lowest fix8 unr to %X\n", pc);
	}
    }
}

void ComputeLowestFix8Unresolved() {
    assert(!IS_ARM64);
    Lowest_Fix8_Unresolved = std::numeric_limits<PCTR>::max();
    for (Fixup& F : FixupTable) {
	if (F.tag != NULL && F.width == SINGLE_WIDTH && F.pc < Lowest_Fix8_Unresolved)
	    Lowest_Fix8_Unresolved = F.pc;
    }
    if (ListOpt && TraceOpt)
	list ("; LOWEST UNRESOLVED F.pc = %04X\n", Lowest_Fix8_Unresolved);
}

void GensymTag(Jtag& tg) {
    snprintf (tg.lab, sizeof(tg.lab), "g%04d", GensymCtr++);
}


void Outtag (Jtag& tg) {
    if (Label_Pending[0])
	list ("%0*X\t\t%s:\n", Ahex, Pctr, Label_Pending);
    strcpy (Label_Pending, tg.lab);
}

void DefineTag (Jtag& tg) {
    tg.defined = tg.have_pc = true;
    tg.tramp_defined = false;
    tg.pctr = Pctr;
    if(tg.lab[0] == 0)
        GensymTag(tg);
    Outtag (tg);
}

int disp_ok (PCTR pc, PCTR v) {
    if (IS_ARM64)  {
        int d = v - pc;
        bool ok =(d >= -32768) && (d <= 32767);
        return (int)ok;
    }
    int disp;
    if (v < pc) {
	disp = pc - v;
	disp = -disp;
    }
    else disp = v - pc;
    return (-126+FullWidth < disp && disp < 126-FullWidth);
//    return (-32 < disp && disp < 32);
}



typedef struct _Ctxt Ctxt;

static bool dep_list_sorter( const DepPair&A, const DepPair& B) {
    Rlysym *ar = RelayRefTable[A.affector];
    Rlysym *br = RelayRefTable[B.affector];
    if (ar->n < br->n)
	return true;
    if (ar->n > br->n)
	return false;
    if (ar->type != br->type)
	return strcmp (redeemRlsymId (ar->type), redeemRlsymId (br->type)) < 0;
    if (A.affected < B.affected)
	return true;
    if (A.affected > B.affected)
	return false;
    return false;
}

RLID RelayId (Sexpr s) {
    Rlysym* rlptr = s.u.r;
    if (RIDMap.count(rlptr))
        return RIDMap[rlptr];

    RLID relay_id = (RLID)RelayRefTable.size();
    RelayRefTable.emplace_back(rlptr);
    RIDMap[rlptr] = relay_id;

    if (Arch->Bits == 64)
        list ("%sv$%s equ\t0x0%X\n",
              Ltabs, s.PRep().c_str(), relay_id * Arch->RelayBlockSize);
    else
        list ("%sv$%s equ\tbyte ptr 0%Xh\n",
              Ltabs, s.PRep().c_str(), relay_id * Arch->RelayBlockSize);
    return relay_id;
}


PCTR RlsymOffset (Rlysym * r) {	/* called from writetko */
    return Arch->RelayBlockSize*RIDMap[r];
}

static PCTR RelayOffset (Sexpr s) {
    return Arch->RelayBlockSize*RelayId(s);
}

void OutputListingLine
   (const char* opmnem, const char unsigned * bytes, int bytect, const char* opd) {
again:

    int tcol = Arch->Bits == 16 ? 16 : 24;
    int llen = Label_Pending[0] ? (int)strlen (Label_Pending+1) : 0;
    int cc = Ahex+2;

    list ("%0*X  ", Ahex, Pctr);
    if (llen < 8)
	for (int i = 0; i < bytect; i++) {
            int j = i;
            if (IS_ARM64)
                j = bytect - i - 1;
	    list ("%02X", bytes[j]);
	    cc+=2;
	}
    bool oflw = false;
    if (tcol >= 24) {
        list(" ");
        oflw = true;
    }

    while (cc < tcol) {
	list ("\t");
	cc = (cc| 7) + 1;
    }
    if (llen > 0) {
	list ("%s:", Label_Pending);
	Label_Pending[0] = 0;
	if (llen >= 8) {
	    list ("\n");
	    goto again;
	}
	cc+= llen;
    }
    if (cc < tcol+8) {
        list ("\t");
        cc = tcol+8;
    }
    list ("%s\t%s\n", opmnem, opd);
}


void out_code (const char* opmnem, const CodeVector& code, const char* opd) {
    if (ListOpt)
        OutputListingLine (opmnem, code.data(), (int)code.size(), opd);
    if (Arch->Bits == 16 && Code.size() + code.size() > 0xFFFF)
        RC_error (2, "Code size exceeds 65K limit for 16-bit compilation.");
    Code += code;
    Pctr += static_cast<PCTR>(code.size());
}

CodeVector OutWord (int opd, int ct) {
    CodeVector code;
    for (int x = ct; x > 0; x--) {
        code += (CodeByte)(opd & 0xFF);
	opd >>= 8;
    }
    return code;
}

void FixupFixup (Fixup & F, PCTR pc) {
    if (IS_ARM64) {
        ARM64FixupFixup(F, pc);
        return;
    }
    int d = pc - F.pc - F.width;
    if (F.width == FullWidth) {
        assert(!IS_ARM64);
        list ((Arch->Bits >= 32) ? //64 significa 32...
              ";  FIXUP 32-bit %08X to %s = %08X, disp %08X\n"
              : ";  FIXUP 16-bit %04X to %s = %04X, disp %04X\n",
              F.pc, F.tag->lab, pc, d);
        *((uint32_t *) &Code[F.pc]) = d;
        F.tag = NULL;
    }
    else {
        assert(F.width == 1);
        Jtag& tag = *F.tag;
        list (";  FIXUP  8-bit %0*X to %s = %0*X, disp %02X\n",
              Ahex, F.pc, tag.lab, Ahex, pc, d);
//        if (disp_ok (pc, F.pc))//  F this baloney
        if ((d >= -128) && (d <= 127)) {
            Code[F.pc] = (unsigned char) d;
        }
        else {
            RC_error (0, "Fixup8 failure at 0x%0*X, d = 0x%X", Ahex, pc, d);
            list (";*!*!*!*!*Fixup8 failure at 0x%0*X d = 0x%X\n", Ahex, pc, d);
        }
        F.tag = NULL;
        ComputeLowestFix8Unresolved();
    }
}

void DefineTagPC (Jtag& tag) {
    tag.pctr = Pctr;
    tag.tramp_defined = 0;
    tag.have_pc = 1;
    Outtag (tag);
    list("%s:    ;****Defining %s as %06X\n", tag.lab, tag.lab, Pctr);
    for (Fixup& F : FixupTable) {
        if (&tag == F.tag)
            FixupFixup (F, Pctr);
    }
}


CodeVector EncodeOpd (int opd, REG_X R1, REG_X R2, int immed) {
    assert(!IS_ARM64);
    enum OP_MOD mod;
    CodeVector code;

    if (immed)
	mod = OPMOD_IMMED;
    else if (opd == 0)
	/* we dont do absolute number yet. */
	if (    (Arch->Bits == 32 && (R2 == X_EBP || R2 == X_ESP))
	     || (Arch->Bits == 16 && (R2 != X_ESI && R2 != X_EDI && R2 != X_EBX)))
	    mod = OPMOD_RP_DISP8;
	else 
	    mod = OPMOD_RPTR;
    else if (opd >= -128 && opd < 128)
	mod = OPMOD_RP_DISP8;
    else mod = OPMOD_RP_DISPLONG;
    
    if (Arch->Bits == 16 && !immed) {
	enum OPREG16 reg16 = MAPMOD1632 [R2];
	if (reg16 == OR16_NONE)
	    RC_error (2, "[%s] addressing not supported in 16-bit mode.",
		      REG_NAMES[1][R2]);
	R2 = (REG_X) reg16;
    }

    code += ((int) mod << 6) | ((int) R1 << 3) | (int) R2;

    if (mod != OPMOD_IMMED && mod != OPMOD_RPTR)
        code += OutWord ((int) opd, (mod == OPMOD_RP_DISP8) ? SINGLE_WIDTH : FullWidth);
    return code;
}

std::string DisasOpd (MACH_OP op, vector<unsigned char>& code,
                      const char *opd, const char *prefix) {
    assert (!IS_ARM64);
    if (!ListOpt)
	return "";
    if (op == MOP_MOVZX64)
        return opd;
    const CodeByte* b = code.data();
    std::string memopd;
    std::string buf;

    unsigned char rmbyte = *b++;
    int flags = Ops[op].flags;
    OP_MOD mode = (OP_MOD) (rmbyte >> 6);

    int R1 = (rmbyte >> 3) & 7;
    int R2 = rmbyte &  7;
    int op8bit = !!(flags & OPF_8BIT);

    if (Arch->Bits == 16 && mode != OPMOD_IMMED)
	R2 = MAPMOD3216[(int)R2];
    const char *r2name = REG_NAMES[(int)(Arch->Bits == 32)][R2];

    switch (mode) {
	case OPMOD_IMMED:
	    if (op8bit)
		r2name = REG_NAMES[2][R2];
            memopd = r2name;
	    break;
	case OPMOD_RPTR:
	/* we dont do absolute number (looks like [ebp]/[esp]) yet. */
	    if (!opd) {
                memopd = std::string("[]") + r2name + "]";
		break;
	    }				/* fall thru if opd given*/
	case OPMOD_RP_DISP8:
	case OPMOD_RP_DISPLONG:
	{
            std::string stropd;
	    if (opd)
                stropd = std::string("+") + (prefix ? prefix : "") +  opd;
	    else {
		long disp;
		if (mode == OPMOD_RP_DISP8)
		    disp = (char) *b;
		else
		    if (Arch->Bits > 16)
			disp = *((long *)b);	/* low-endian platform assumption! */
		    else
			disp = *((short *)b);/* low-endian platform assumption!*/
                stropd = std::string((disp < 0) ? "-" : "+") + std::to_string(disp);
	    }
            memopd = std::string("[") + r2name + stropd + "]";
	    break;
	}
    }

    if (flags & OPF_NOREGOP)
        buf = memopd;
    else {
	int r1bp = (int)(Arch->Bits == 32);
	if (op8bit && op != MOP_MOVZX8)
	    r1bp = 2;
	const char * r1name = REG_NAMES[r1bp][R1];

	if (flags & OPF_RVOPD)
            buf = memopd + "," + r1name;
	else
            buf = std::string(r1name) + "," + memopd;
    }
    return buf;
}


void outinst_general (MACH_OP op, int immed,
                      REG_X r_accum, REG_X r_base, int opd, const char * listopd) {
    assert(!IS_ARM64);
    CodeVector code;

    if (Ops[op].flags & OPF_48)
        code += 0x48;
    if (Ops[op].flags & OPF_0F)
        code += 0x0F;

    code += Ops[op].opcode;
    
    auto encoded_opd = EncodeOpd (opd, r_accum, r_base, immed);
    auto disassed_opd = DisasOpd(op, encoded_opd, listopd, NULL);
    code += encoded_opd;
    out_code (Ops[op].mnemonic, code, disassed_opd.c_str());
}

MACH_OP invert_jump (MACH_OP op) {
    if (op == MOP_JZ)
	return MOP_JNZ;
    else if (op == MOP_NOJUMP)
	return MOP_JMP;
    else return MOP_JZ;
}

void TrampJump (MACH_OP, Jtag&, int jumparound);

void outjmp (MACH_OP op, Jtag& tag);

void out64_alop(MACH_OP op) {
    /* thought that rdx is loaded, but we don't have register color management here.*/
    CodeVector code {0, 0x02};
    const char * opd = "al,BYTE PTR [rdx]";
    switch (op) {
    case MOP_AND:
        code[0] = 0x22;
        break;
    case MOP_OR:
        code[0]= 0x0A;
        break;
    case MOP_LDAL:
        code[0] = 0x8A;
        break;
    case MOP_TST:
        code[0] = 0x84;
        code[1] = 0x0A;
        opd = "BYTE PTR [rdx],cl";
        break;
    default:
        break;
    }

    out_code(Ops[op].mnemonic, code, opd);

}

void outinst_raw (MACH_OP op, const char * str, PCTR opd) {
    if (IS_ARM64) {
        outinst_raw_arm (op, str, opd);
        return;
    }
        
    CodeVector code;
    std::string dbuf;
    
    if (Ops[op].flags & OPF_0F)
        code += 0x0F;
    code += Ops[op].opcode;
    size_t xl = code.size();
    switch (op) {

    case MOP_AND:
    case MOP_OR:
    case MOP_LDAL:
    case MOP_TST:
        if (Intel64 == Arch) {
            out64_alop(op);
            return;
        }
        else {
            code += EncodeOpd ((int) opd, (op == MOP_TST) ? X_BL : X_AL, X_ESI, 0);
            auto tail = vector<CodeByte>(code.begin()+xl, code.end());
            dbuf = DisasOpd (op, tail, str, "v$");
        }
            break;
    case MOP_JZ:
    case MOP_JNZ:
    case MOP_JMP:
            code += (opd-Pctr-2) & 0xFF;
            dbuf = str;
            break;

    case MOP_JMPL:
            code += OutWord ((int) opd, FullWidth);
            dbuf = "long " + std::string(str);
            break;

    case MOP_XOR:
    case MOP_CLZ:
    case MOP_STZ:
            code += opd & 0xFF;
            dbuf = "al," + std::to_string(opd);
            break;
    case MOP_LDBLI8:
            code += opd & 0xFF;
            dbuf = "bl," + std::to_string(opd);
        break;

    case MOP_RPUSH:
    case MOP_RPOP:
        code[0] |=  opd;
            dbuf = REG_NAMES[(int)(Arch->Bits == 32)][opd];
        break;

    case MOP_RRET:
            code += OutWord ((int) opd, 2);
            dbuf = std::to_string(opd);
        break;

    case MOP_RETF:
    case MOP_RET:
    case MOP_LEAVE:
    default:
        break;
    }
    out_code (Ops[op].mnemonic, code, dbuf.c_str());
}



void check_fix8_overflows() {
    assert(!IS_ARM64);
    int jumps = 0;
    Jtag jump;
top:
    if (Lowest_Fix8_Unresolved < Pctr)
	if (!disp_ok (Pctr+5+FullWidth, Lowest_Fix8_Unresolved))
            for (Fixup & F : FixupTable) {
                Jtag * ftag = F.tag;
		if (F.tag != NULL)
		    if (F.pc == Lowest_Fix8_Unresolved) {
			list ("; TRAMP OUT %s, ref pc = %04X\n",
			      F.tag->lab, F.pc);
			if (jumps++ == 0) {
			    GensymTag(jump);
			    jump.defined = jump.have_pc = false;
			    jump.tramp_defined = false;
			    outinst_raw (MOP_JMP, jump.lab, Pctr+1);
			    RecordFixup (jump, Pctr - 1, SINGLE_WIDTH); // destroys iteration.
			}
			TrampJump (MOP_NOJUMP, *ftag, 0);
                        Jtag *t = ftag;
                        for (Fixup& f1 : FixupTable) {
			    if (f1.tag == t && f1.width == SINGLE_WIDTH)
				FixupFixup(f1, t->tramp_pc);
			}
			goto top;
		    }
	    }
    if (jumps > 0)
	DefineTagPC (jump);
}	


void outinst (MACH_OP op, Sexpr s, PCTR opd) {
    std::string str;
    if (!IS_ARM64)
        check_fix8_overflows();
    if (s.type == Lisp::RLYSYM)
        str = s.PRep();
    else if (s == T_ATOM)
	str = "1";
    else if (s == NIL)
	str = "0";
    else if (s == LRET) {
	if (op == MOP_JMP) {
	    DefineTag (RetCF);
	    outinst_raw (RetOp, "", 0);
	}
	else if (RetCF.have_pc && disp_ok (Pctr+FullWidth, RetCF.pctr))
	    outinst_raw (op, RetCF.lab, RetCF.pctr);
	else {
	    Jtag t;
	    GensymTag(t);
	    t.defined = t.have_pc = true;
	    t.tramp_defined = false;
	    t.pctr = Pctr+3;

	    outinst_raw (invert_jump (op), t.lab, t.pctr);
	    DefineTag (RetCF);
	    outinst_raw (RetOp, "", 0);
	    Outtag (t);
	}
	return;
    }
    else if (s == ZRET || s == TRET) {
	Jtag& tag = (s == ZRET) ? RetZero : RetOne;
	if (op == MOP_JMP) {
	    DefineTag (tag);
	    if (s == ZRET)
		outinst_raw (MOP_CLZ, "", 0);
	    else outinst_raw (MOP_STZ, "", 1);
	    DefineTag (RetCF);
	    outinst_raw (RetOp, "", 0);
	}
        else if (tag.have_pc && ((IS_ARM64 && disp_ok (Pctr, tag.pctr))
                                 || (!IS_ARM64 && disp_ok (Pctr+2, tag.pctr)))) {
            outinst_raw (op, tag.lab, tag.pctr);
        }
	else {
	    Jtag t;
	    GensymTag(t);
	    t.defined = t.have_pc = true;
	    t.tramp_defined = false;
            if (IS_ARM64)
                t.pctr = Pctr + 3*4;
            else
                t.pctr = Pctr+5;

	    outinst_raw (invert_jump (op), t.lab, t.pctr);
	    DefineTag (tag);
	    if (s == ZRET)
		outinst_raw (MOP_CLZ, "", 0);
	    else outinst_raw (MOP_STZ, "", 1);
	    DefineTag (RetCF);
	    outinst_raw (RetOp, "", 0);
	    Outtag (t);
	}
	return;
    }
    outinst_raw (op, str.c_str(), opd);
}

void TrampJump (MACH_OP op, Jtag& tag, int jumparound) {
    assert(!IS_ARM64);
    Jtag jump;
    if (op != MOP_JMP && jumparound) {
	GensymTag(jump);
	jump.defined = jump.have_pc = 1;
	jump.tramp_defined = 0;
	jump.pctr = Pctr+FullWidth+3;
	outinst_raw (invert_jump (op), jump.lab, jump.pctr);
    }
    Jtag tramp;
    DefineTag(tramp);
    strcpy (tag.tramp_lab, tramp.lab);
    tag.tramp_defined = 1;
    tag.tramp_pc = Pctr;
    outinst_raw (MOP_JMPL, tag.lab, (Arch->Bits==16) ? 0xFFFF: 0xFFFFFFFF);
    RecordFixup (tag, Pctr - FullWidth, FullWidth);
    if (op != MOP_JMP && jumparound)
	DefineTagPC (jump);
}

void outjmp (MACH_OP op, Jtag& tag) {
    if (!IS_ARM64)
        check_fix8_overflows();
    if (&tag == &RetCF){
	outinst (op, LRET, 0);
	return;
    }
    else if (&tag == &RetOne) {
	outinst (op, TRET, 1);
	return;
    }
    else if (&tag == &RetZero) {
	outinst (op, ZRET, 0);
	return;
    }
    else if (tag.have_pc)
	if (disp_ok (Pctr, tag.pctr)) {
	    outinst_raw (op, tag.lab, tag.pctr);
	    return;
	}
        else if (tag.tramp_defined) {
dot:	    if (disp_ok (Pctr, tag.tramp_pc)) {
		outinst_raw (op, tag.tramp_lab, tag.tramp_pc);
		return;
	    }
        }
	else;
    else if (tag.tramp_defined)
	goto dot;
    if (!tag.have_pc && !tag.tramp_defined) {
	outinst_raw (op, tag.lab, Pctr+1);
        if (IS_ARM64)
            RecordFixup (tag, Pctr - 4, 4);
        else
            RecordFixup (tag, Pctr - 1, SINGLE_WIDTH);
	return;
    }
    TrampJump (op, tag, 1);
}

void AddLabel (Sexpr s, Sexpr v) {
    for (auto& lp : LabelTable)
	if (lp.s == s)
	    RC_error (1, "Duplicate label: %s", s.u.a);
    LabelTable.emplace_back(s, v);
}

void RecordTimer (RLID id, int time) {
    Timers.emplace_back(id, time);
}
		


void CompileExpr (Sexpr s, Ctxt* ctxt);

static RLID DefiningRelay;
static std::unordered_set<RLID> DefiningRelayDependents;

void RecordDependent (RLID affector) {

    /* Elim duplicates - will speed up runtime and simplify obj seg writer. */
    if (DefiningRelayDependents.count(affector) > 0)
        return;
    DefiningRelayDependents.insert(affector);
    DependentPairTable.emplace_back(affector, DefiningRelay);
}


void outinstLDQW(int destr, int baser, PCTR offset, const char * opd_str) {
    assert(destr == X_RDX && baser == X_R8);  // for now...
    CodeVector code{0x49, 0x8B, 0x90};
    if (offset < 0x80) {
        code[2] = 0x50;
        code += CodeByte(offset);
    }
    else
        code += OutWord((uint32_t)offset, 4);
    out_code("mov", code, opd_str);
}

void CompileRlysym (Sexpr s, Ctxt * ctxt, int backf) {
    if (s.type != Lisp::RLYSYM)
	RC_error (3, "Non-Rlysym handed to CompileRlysym.");
    RecordDependent (RelayId(s));
    Jtag* tag = ctxt->tag;
    PCTR offset = RelayOffset(s);
    if (Intel64 == Arch) {
        string rname = s.PRep();
        outinstLDQW(X_RDX, X_R8, offset, FormatString("rdx,QWORD PTR [r8+v$%s]", rname.c_str()).c_str());
    }
    if (backf) {
	switch (ctxt->op) {
	    case CT_VAL:
		/* this -always- works, if the other thing gets broken. */
		outinst (MOP_LDAL, s, offset);
		outinst (MOP_XOR, T_ATOM, 1);
		break;
	    case CT_AND:
		outinst (MOP_TST, s, offset);
		if (tag == &RetCF)
		    tag = &RetZero;
		outjmp (MOP_JNZ, *tag);
		break;
	    case CT_OR:
		outinst (MOP_TST, s, offset);
		if (tag == &RetCF)
		    tag = &RetOne;
		outjmp (MOP_JZ, *tag);
		break;
	}
	return;
    }
    else
	outinst (MOP_TST, s, offset);

    switch (ctxt->op) {
	case CT_VAL:
	    break;
	case CT_AND:
	    outjmp (MOP_JZ, *tag);
	    break;
	case CT_OR:
	    outjmp (MOP_JNZ, *tag);
	    break;
    }
    return;
}

void CompileAndOr (Sexpr args, CtxtOp op, Ctxt* ctxt) {
    
    if (TraceOpt)
	list (";**CANDOR %c %c %s %s\n",
              CTVAL[op], CTVAL[ctxt->op], ctxt->tag->lab, args.PRep().c_str());
    if (args == NIL)
	return;
    if (CDR(args) == NIL) {
	CompileExpr (CAR(args), ctxt);
	return;
    }
    Jtag Et;
    Ctxt newctxt(nullptr, CT_VAL);

    int same = (op == ctxt->op);
    Et.defined = false;
    if (!same) {
	if (ctxt->op == CT_VAL)
	    newctxt.tag = ctxt->tag;
	else {
	    GensymTag(Et);
	    Et.defined = true;
	    Et.tramp_defined = false;
	    Et.have_pc = false;
	    newctxt.tag = &Et;
	}
	newctxt.op = op;
    }

    for (;CONSP(args);SPop(args))
	CompileExpr (CAR(args),((CDR(args) == NIL) || same) ? ctxt : &newctxt);

    if (Et.defined)
	DefineTagPC(Et);
}

static int LExpandLevel = 0;

void CompileExpr (Sexpr s, Ctxt* ctxt) {
    if (TraceOpt)
	list (";**CEXPR    %c %s %s\n",
              CTVAL[ctxt->op], ctxt->tag->lab, s.PRep().c_str());

    if (s.type == Lisp::tCONS) {
	Sexpr fn = CAR(s);
	if (fn == NOT) {
	    Sexpr ss = CADR (s);
	    if (!(ss.type == Lisp::RLYSYM))
		RC_error (1, "No non-atomic-relay NOT's.");
	    CompileRlysym (ss, ctxt, 1);
	}
	else if (fn == AND)
	    CompileAndOr (CDR(s), CT_AND, ctxt);
	else if (fn == OR)
	    CompileAndOr (CDR(s), CT_OR, ctxt);
	else if (fn == LABEL) {
	    if (CDR(s).type != Lisp::tCONS || CDDR(s).type != Lisp::tCONS)
		RC_error (1, "Bad Format LABEL clause.");
	    Sexpr ltag = CADR(s);
	    if (ltag.type != Lisp::ATOM)
		RC_error (1, "Label is not atom.");
	    Sexpr exp = CDDR(s);
	    CDDR(s) = NIL;
	    CAR(s) = AND;
	    if (CheckOpt && ctxt->op != CT_VAL)
		RC_error (0, "Label Not for val at def time: %s",
			  ltag.u.s);
	    if (LExpandLevel == 0)
		AddLabel (ltag, exp);
	    CompileAndOr (exp, CT_AND, ctxt);
	}
	else {
	    Sexpr f2 = MaybeExpandMacro (s);
	    if (f2 != EOFOBJ) {
		CompileExpr (f2, ctxt);
		dealloc_ncyclic_sexp (f2);
		return;
	    }

	    LispBarf (1, "Unknown form:", s);
	    RC_error (1, "Unknown Form in Relay Compiler.");
	}
						
    }			
    else if (s.type == Lisp::ATOM) {
        for (auto& lte : LabelTable)
	    if (lte.s == s) {
		if (CheckOpt && ctxt->op != CT_VAL)
		    RC_error (0, "Label Not for val at use time: %s",
			  s.u.s);
		LExpandLevel ++;
		CompileAndOr (lte.Svalue, CT_AND, ctxt);
		LExpandLevel --;
		return ;
	    }
	if (s == T_ATOM) goto tat;
	else if (s == NIL) goto nat;
	RC_error (1, "Label/Symbol not known as form: %s", s.u.a);
    }
    else if (s.type == Lisp::RLYSYM)
	CompileRlysym (s, ctxt, 0);
    else if (s.type == Lisp::NUM) {
	if (s.u.n == 1){
tat:
	    switch (ctxt->op) {
		case CT_VAL:
		    outinst (MOP_STZ, T_ATOM, 1);
		    break;
		case CT_OR:
		    outjmp (MOP_JMP, *ctxt->tag);
		    break;
		case CT_AND:
		    break;
	    }
	    return;
	}
	else if (s.u.n == 0) {
nat:
	    switch (ctxt->op) {
		case CT_VAL:
		    outinst (MOP_CLZ, NIL, 0);
		    break;
		case CT_OR:
		    break;
		case CT_AND:
		    outjmp (MOP_JMP, *ctxt->tag);
		    break;
	    }
	}
	else goto nogo;
    }
    else {
nogo:	LispBarf (1, "Mystery meat: ", s);
	RC_error (1, "Non-recognized object to be compiled.");
    }
    return;
}

void PushRelayDef (Sexpr rlysexpr) {
    Rlysym* relay_sym = rlysexpr.u.r;
    if (RelayDefQuickCheck.count(relay_sym))
        RC_error (1, "Relay already defined: %s", relay_sym->PRep().c_str());

    DefiningRelay = (int)RelayDefTable.size();
    DefiningRelayDependents.clear();
    RelayDefTable.emplace_back(relay_sym, Pctr);
    RelayDefQuickCheck.emplace(relay_sym);
    FixupTable.clear();
    if (!IS_ARM64)
        ComputeLowestFix8Unresolved();
}


void CompileRelayDef (Sexpr s) {
    Jtag t;
    Sexpr rlysexpr = CAR(s);
    snprintf (t.lab, sizeof(t.lab), "c$%s", rlysexpr.u.r->PRep().c_str());
    PushRelayDef (rlysexpr);
    list ("\n%s\tpublic\t%s\n", Ltabs, t.lab);
    DefineTagPC(t);
    Ctxt ctxt (&RetCF, CT_VAL);
    if (IS_ARM64)
        OutputARMFunctionPrologue();
    CompileAndOr (CDR(s), CT_AND, &ctxt);
    outinst (MOP_JMP, LRET, 0);
    RelayDef& rdef = RelayDefTable.back();
    rdef.size = Pctr - rdef.pc;
}

/* The entry thunk currently appears as a relay named {0 _entry_thunk}.
   If such exists, the loader will arrange to have the relay engine
   call it as int WINAPI _entry_thunk(void* linkage_ptr, void* code_ptr)
   when a compiled relay is to be called -- linkage pointer is the addr of the
   allocated "static" and code_ptr is the address of the code. The
   entry thunk is responsible for switching call conventions, saving
   and restoring such registers as the compiled code uses, and doing any
   other setup or cleanup or value conversion that the compiled code expects.
   This should reduce all architecture-dependence to this relay compiler.
 
   This works for the 64-bit Intel, Mac and windows, architecture as well
   (with different thunk names).

*/

void outCallReg64 (int, int reg, const char * str_opd) {
    CodeVector code {0xFF, 0xD0};
    if (reg >= X_RAX && reg < X_R8) {
        code[1] += (reg - X_RAX);
    }
    out_code("call", code, str_opd);
}

void outMovRR64(int, int regdest, int regsource, const char * str_opd) {
    CodeVector code {0x48, 0x89, 0xC0};
    if (regdest < X_R8)
        code[2] |= regdest - X_RAX;
    else {
        code[2] |= regdest - X_R8;
        code[0] |= 1;
    }
    if (regsource >= X_R8) {
        regsource -= X_R8;
        code[0] |= 4;
    } else {
        regsource -= X_RAX;
    }
    code[2] |= (regsource >> 1) << 4;
    if (regsource & 1)
        code[2] |= 8;
    /* qué michegas */
    out_code("mov", code, str_opd);
}

void outMovI32R64 (int, int reg, uint32_t val, const char * str_opd) {
    CodeVector code {0xB8};

    auto save_reg = reg;
    if (reg >= X_R8)
        reg -= X_R8;
    else
        reg -= X_RAX;
    code[0] |= reg & 7;
    if (save_reg >= X_R8)
        code.insert(code.begin(), 0x41);
    code += OutWord(val, 4);
    out_code("mov", code, str_opd);
}

short get_relay_type_index(const char * name);

void DefineBogoThunkRelay (const char * name) {
    int rtx = get_relay_type_index (name); // flushed "noncanonical" case-sensitive stuff
    Rlysym * r = new Rlysym(0, rtx, NULL);
    Sexpr rly;
    rly.u.r = r;
    PushRelayDef (rly);
    list ("\n%s\tpublic\t%s\n%s%s:\n", Ltabs, name, Ltabs, name);
}
void CompileIA32EntryThunk () {

    DefineBogoThunkRelay("_ENTRY_THUNK");
    list("%s;;;\targs = EBP+8, EBP+12 (linkage array, code ptr)\n", Ltabs);
    outinst         (MOP_RPUSH,  NIL, X_EBP);
    outinst_general (MOP_LOADWD, 1,   X_EBP, X_ESP, 0, NULL);
    /* vc4 generated "sub esp,4" here: why? */
    outinst         (MOP_RPUSH,  NIL, X_EBX);
    outinst         (MOP_RPUSH,  NIL, X_ESI);
    outinst_general (MOP_LOADWD, 0,   X_ESI, X_EBP, 8, "linkage_ptr");
    outinst         (MOP_LDBLI8, NIL, 1);
    outinst_general (MOP_CALLIND,0, (REG_X) 2, X_EBP, 12, "code_ptr");
    outinst_general (MOP_SETNZ,  1,   X_EAX, X_AL, 0, NULL);
    outinst_general (MOP_MOVZX8, 1,   X_EAX, X_AL, 0, NULL);
    outinst         (MOP_RPOP,   NIL, X_ESI);
    outinst         (MOP_RPOP,   NIL, X_EBX);
    outinst         (MOP_LEAVE,  NIL, 0);
    outinst         (MOP_RRET,   NIL, 8);
}


void CompileMacX86EntryThunk() {
    DefineBogoThunkRelay("_macos_entry_thunk");
    list("%s;;;\targs = RDI, RSI (linkage array, code ptr)\n", Ltabs);
    outMovRR64      (MOP_MOVRR64, X_R8, X_RDI, "r8,rdi     ;Arg 1 = linkage array");
    outMovI32R64    (MOP_LDR64I32, X_RCX, 1, "rcx,0x00000001");
    outCallReg64    (MOP_CALLREG, X_RSI, "rsi        ;Arg 2 = relay code");
    outinst_general (MOP_SETNZ,  1,   X_RAX, X_AL, 0, NULL);
    outinst_general (MOP_MOVZX64,1,   X_RAX, X_AL, 0, "rax,al");
    outinst         (MOP_RET,   NIL, 0);
}
void CompileWindowsX86EntryThunk() {
    DefineBogoThunkRelay("_windows_entry_thunk");
    list("%s;;;\targs = RCX, RDX (linkage array, code ptr)\n", Ltabs);
    outMovRR64      (MOP_MOVRR64, X_R8, X_RCX, "r8,rcx     ;Arg 1 = linkage array");
    outMovI32R64    (MOP_LDR64I32, X_RCX, 1, "rcx,0x00000001");
    outCallReg64    (MOP_CALLREG, X_RDX, "rdx        ;Arg 2 = relay code");
    outinst_general (MOP_SETNZ,  1,   X_RAX, X_AL, 0, NULL);
    outinst_general (MOP_MOVZX64,1,   X_RAX, X_AL, 0, "rax,al");
    outinst         (MOP_RET,   NIL, 0);
}


void CompileTimerRelayDef (Sexpr s) {
    Sexpr nam = CAR(s);
    SPop(s);
    long time = CAR(s).u.n;
    /* RPLACA s, actually */
    CAR(s) = nam;			/* compile as 422U */
    CompileRelayDef(s);
    RecordTimer (DefiningRelay, (int)time);
}


void CleanUpRelaySys () {
    for (auto& lte : LabelTable)
	dealloc_ncyclic_sexp (lte.Svalue);
}

void InitRelayCompiler () {
    
    SetLispBarfString ("Relay Compiler");
    snprintf(RetZero.lab, sizeof(RetZero.lab), "ret_0");
    RetZero.defined = true;
    snprintf(RetOne.lab, sizeof(RetOne.lab), "ret_1");
    RetOne.defined = true;
    snprintf(RetCF.lab, sizeof(RetOne.lab), "retval");
    RetCF.defined = true;
    RetCF.have_pc = RetOne.have_pc = RetZero.have_pc = 0;
    RetCF.tramp_defined = RetOne.tramp_defined = RetZero.tramp_defined = 0;
    Pctr = 0;

    fasd_init();
}

void PrintRelayTable () {
    list ("\f%zu relays defined by code:    "
	    "ISD (Internal symbol dictionary)\n\nIndex\tPC\tFn Size\tRelay\n",
	    RelayDefTable.size());
    int i = 0;
    for (auto& rdte : RelayDefTable) {
	list ("%4d\t%04X\t%4X\t%s\n",
              i++, rdte.pc, rdte.size , rdte.sym->PRep().c_str());
    }
    list ("\n");
    list ("\fRelay Table (%zu relays referenced):    "
	  "ESD (External symbol dictionary)\nIndex\tOffset\tRelay\n\n",
	  RelayRefTable.size());
    i = 0;
    for (auto rte : RelayRefTable) {
        list ("%4d\t%04X\t%s\n", i++, RlsymOffset (rte), rte->PRep().c_str());
    }
    list ("\n");
    list ("Code seg size 0x%04X = %u\n", Pctr, Pctr);
    auto rblock_size = Arch->RelayBlockSize*RelayRefTable.size();
    list ("Relay DS size 0x%04lX = %ld\n", rblock_size, rblock_size);
    int col = 0;
#ifdef print_fixups
    list ("\n%d fixups generated.\n", FixupCount);
    if (FixupTable.size() > 0) {
	list ("\n");
        for (Fixup& F : FixupTable) {
	    if (col == 0) {
		col = 1;
		list (" ");
	    }
	    if (col >= 72) {
		list ("\n ");
		col = 1;
	    }
	    list ("  %04X", F.pc);
	    col += 6;
	}
	list ("\n");
    }
#endif
    list ("\n%zu dependent pairs:\n", DependentPairTable.size());
    RLID lastid = (RLID)-1;
    col = 0;
    for (auto& D : DependentPairTable) {
	RLID affector = D.affector;
	RLID affected = D.affected;

	if (affector != lastid) {
	    Rlysym* rsp = RelayRefTable[affector];
            std::string SRep = rsp->PRep();
	    list ("\n%s: ", SRep.c_str());
	    col = (int)SRep.size()+2;
	    if (col < 8) {
		list ("\t");
		col = 8;
	    }
	    lastid = affector;
	}
	Rlysym* rsp1 = RelayDefTable[affected].sym;
        string SARep = rsp1->PRep();
        int l = (int)SARep.size();
	if (col+l+1 > 72) {
	    list ("\n\t");
	    col = 8;
	}
	if (col == 8) {
	    list ("%s", SARep.c_str());
	    col += l;
	}
	else {
	    list (" %s", SARep.c_str());
	    col += 1+l;
	}
    }
    list ("\n");
}

void CompileFile(FILE* f, const char * fname);

void CompileTopLevelForm (Sexpr s, const char * fname) {
    if (s.type != Lisp::tCONS)
	RC_error (1, "Item definition not a list?");
    if (CAR(s).type != Lisp::ATOM) {
	RC_error (1, "Top-level item doesn't start with atom.");}
    else {
	Sexpr fn = CAR(s);
	Sexpr f2 = MaybeExpandMacro (s);
	if (f2 != EOFOBJ) {
	    CompileTopLevelForm (f2, fname);
	    dealloc_ncyclic_sexp (f2);
	}
	else if (fn == DEFRMACRO)
	    defrmacro (s);		/* maybe FASDUMP macdef, too!? */
	else if (fn == FORMS) {
	    SPop (s);
forms:
	    while (s.type == Lisp::tCONS) {
		CompileTopLevelForm (CAR(s), fname);
		SPop (s);
	    }
	}
	else if (fn == RELAY)
	    CompileRelayDef (CDR(s));
	else if (fn == TIMER)
	    CompileTimerRelayDef (CDR(s));
	else if (fn == INCLUDE) {
            string path (replace_filename(fname, CADR(s).u.s));
	    FILE * ff = fopen (path.c_str(), "r");
	    if (ff == NULL)
		RC_error (1, "Cannot open include file %s", path.c_str());
	    CompileFile (ff, path.c_str());
	}
	else if (fn == COMMENT);
	else if (fn == EVAL_WHEN) {
	    SPop(s);
	    if (s.type == Lisp::tCONS && CAR(s).type == Lisp::tCONS) {
		Sexpr sc = CAR(s);
		SPop(s);
		for (; sc.type == Lisp::tCONS; SPop(sc))
		    if (CAR(sc) == LOAD)
			goto forms;
	    }
	}
	else fasd_form (s);
    }
}

void CompileFile (FILE* f, const char * fname) {
    for (;;) {
	Sexpr s = read_sexp (f);
	if (s == EOFOBJ)
	    break;
	CompileTopLevelForm (s, fname);
	dealloc_ncyclic_sexp (s);
    }
    fclose (f);
}


void CompileLayout (FILE* f, const char * fname) {
    InitRelayCompiler();
    if (Arch->Bits == 32)
	CompileIA32EntryThunk();
    if (!IS_ARM64 && Arch->Bits == 64) {
        CompileMacX86EntryThunk();
        CompileWindowsX86EntryThunk();
    }
    CompileFile (f, fname);
    list ("%s  end	\n", Ltabs);
}

string merge_ext(const char * input, const char* new_ext, bool force) {
    auto fspath = fs::path(input);
    if (force || !fspath.has_extension())
        fspath.replace_extension(new_ext);
    return fspath.string();
}

void CallWtko (const char * path, const char * opath, time_t timer,
	       int cversion, const char * compdesc) {
    
    TKO_INFO tki;
    tki.Isd = RelayDefTable.data();
    tki.isd_count = static_cast<int>(RelayDefTable.size());
    tki.Fxt = FixupTable.data();
    tki.fixup_count = 0;
    tki.Code = Code.data();
    tki.Dpt = DependentPairTable.data();
    tki.dpt_count = (int)DependentPairTable.size();
    tki.Esd = RelayRefTable.data();
    tki.esd_count = (int)RelayRefTable.size();
    tki.Tmr = Timers.data();
    tki.tmr_count = (int)Timers.size();
    tki.static_len = tki.esd_count*Arch->RelayBlockSize;
    tki.code_len = Pctr;
    tki.time = static_cast<long>(timer);
    tki.Frm = fasd_data (tki.frm_count);
    tki.Ats = fasd_atsym_data (tki.ats_count);
    tki.bits = Arch->Bits;
    if (IS_ARM64) {
        tki.Architecture = "ARM64";
        tki.code_item_len = 4;
        tki.code_ct = Pctr/tki.code_item_len;
    }
    else {
        tki.Architecture = "INTEL x86";
        tki.code_item_len = 1;
        tki.code_ct = Pctr;
    }
    tki.arch_characterization = 0;
    tki.compiler = compdesc;
    tki.compiler_version = cversion;
    string merged_path;
    if (opath[0] != '\0')
	merged_path = opath;
    else
        merged_path = merge_ext(path, ".tko", true);
    if (Arch->Bits > 16)
	write_tko32 (merged_path.c_str(), tki);
    else
	write_tko (merged_path.c_str(), tki);
    cout << std::format("{0} (0x{0:X}) code bytes generated\n", Pctr);
    cout << std::format("{} relay{} defined, {} referenced.\n",
            RelayDefTable.size(),
            (RelayDefTable.size() == 1) ? "" : "s",
	    RelayRefTable.size());
    cout << merged_path << " written, " << get_file_size(merged_path) << " bytes." << endl;
}

static bool OpenListing (const char * path, string& lpath) {
    if (lpath.empty())
        lpath = merge_ext(path, ".lst", true);
    ListFile = fopen (lpath.c_str(), "w");
    if (ListFile == NULL) {
        cerr << "Can't open listing file " << lpath << " for output." <<endl;
        return false;
    }
    cout << "Listing to " << lpath <<endl;
    return true;
}

int main (int argc, char ** argv) {
#if NXSYSMac
    auto OS_CHAR = "macOS";
    int compiler_bits = 64;   //not running on same platform as the object.
#else
    auto OS_CHAR = "Windows";
    int compiler_bits = sizeof(int*) * 8;
#endif
    
#if defined(__aarch64__) || defined(_M_ARM64)
    Arch = &macARM;
#else
    Arch = &Intel64;
#endif
    
    string opath;
    string lpath;
    string compdesc = string("BSG NXSYS Relay Compiler Version ") + std::to_string(COMPILER_VERSION) + " (";
    compdesc += std::to_string(compiler_bits) + "-bit of " + __DATE__ + " " __TIME__ + ")" +
       " (" + OS_CHAR + ")";
    cout << compdesc << endl;
    cout << COMPILER_COPYRIGHT <<endl;
    
#if NXSYSMac
    cout << "MacOS clang++ implementation\n";
#endif
    
    if (argc < 2) {
        usage:
        auto execpath = fs::path(argv[0]);
        cerr << "Usage: " << execpath << " source{.trk} {args}\nArgs:\n";
        cerr <<
                 "  -L    Make listing to source.lst\n"
                 "  -Fo:nondefault_outputpath (default is source.tko)\n"
                 "  -Fl:nondefault_listingpath (default is source.lst)\n"
                 "  -arch:ARCH, ARCH as below (default as per cmplr env)\n"
                 "  -C    Special debug checking\n"
                 "  -T    Internal compiler tracing to listing\n";
        Architecture::print_known(Arch);
        exit(2);
    }
    
    
    const char * fpath = NULL;
    for (int argno = 1; argno < argc; argno++) {
        const char * arg = argv[argno];
        if (
#if _MSC_VER       // Slashes in pathnames don't work as control arg introducers
            arg[0] == '/' ||
#endif
            arg[0] == '-') {
                string argval= stoupper(arg+1);
                if (argval == "L")
                    ListOpt = true;
                else if (argval == "T")
                    TraceOpt = true;
                else if (argval == "C")
                    CheckOpt = true;
                else if (!strncmp(argval.c_str(), "FO:", 3)) {
                    if (arg[3] == '\0') {
                        cerr << "Output pathname missing after /Fo:\n";
                        goto usage;
                    }
                    opath = arg+4;
                }
                else if (!strncmp (argval.c_str(), "FL:", 3)) {
                    if (arg[3] == '\0') {
                        cerr << "Listing pathname missing after /Fl:\n";
                        goto usage;
                    }
                    lpath = arg+4;
                    ListOpt = true;
                }
                else if (argval.find("ARCH:") == 0) {
                    auto ap = Architecture::find(argval.substr(5));
                    if (ap == nullptr) {
                        Architecture::print_known(Arch);
                        exit(6);
                    }
                    Arch = ap;
                }
                else {
                    cerr << "Unrecognized arg: " << argval <<endl;
                    goto usage;
                }
            }
        else fpath = arg;
        
    }
    if (fpath == NULL) {
        cerr << "No input pathname supplied." <<endl;
        goto usage;
    }

    cout << "Target architecture: " << Arch->Description <<endl;

    RetOp = MOP_RET;
    Ltabs = (Arch->Bits == 16) ? "\t\t" : "\t\t\t";
    FullWidth = std::min(4, Arch->Bits/8);
    Ahex = std::min(Arch->Bits/4,8);

    string input_path = merge_ext(fpath, ".trk", false);

    FILE* f = fopen (input_path.c_str(), "r");
    if (f == NULL) {
        cerr << "Cannot open " << input_path <<  " for reading." <<endl;
        return 3;
    }

    struct tm *tblock;
    time_t now = time(NULL);
    tblock = localtime (&now);
    if (ListOpt)
	if (!OpenListing (input_path.c_str(), lpath))
            return 2;

    list ("; Compilation of %s at %s", fpath, asctime(tblock));
    list (";  for %s\n", Arch->Description);
    list (";  by %s\n", compdesc.c_str());
    list (";  " COMPILER_COPYRIGHT  "\n");
    list (";  Bytes/Relay-linkage %d\n", Arch->RelayBlockSize);
    list ("\n");
    if (Arch->Bits == 64)
        list ("%s\tbits\t%d\n", Ltabs, Arch->Bits);
    list ("%s\tideal\n%s\tsegment\tcode\n", Ltabs, Ltabs);

    CompileLayout (f, fpath);

    if (ListOpt) {
        std::sort(DependentPairTable.begin(), DependentPairTable.end(), dep_list_sorter);
	PrintRelayTable();
    }
    fasd_finish();
    if (ListOpt) {
	int frm_count, ats_count;
	fasd_data (frm_count);
	fasd_atsym_data (ats_count);
	if (frm_count == 0)
	    list ("\nNo random forms.\n");
	else
	    list ("\n0x%X = %d bytes fasdumped forms, %d Atsyms.\n",
		  frm_count, frm_count, ats_count);
	fclose(ListFile);
    }
    std::sort(DependentPairTable.begin(), DependentPairTable.end(), dep_sorter);
    CallWtko (input_path.c_str(), opath.c_str(), now, COMPILER_VERSION, compdesc.c_str());
    return 0;
};

// I suppose this is right even on Windows.
void MessageBox(void*, const char* msg, const char* title, int) {
    cerr << msg << ": " << title <<endl;
}
