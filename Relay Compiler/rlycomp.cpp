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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#ifdef _MSC_VER
#include "dircmpat.h"
#endif
#include <string>
#include <vector>
#include <filesystem>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include "STLExtensions.h"
#include "replace_filename.h"

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

#define ENTRY_THUNK_NAME "_entry_thunk"

/* Variables that change value for 16/32 bit compilations */
#define SINGLE_WIDTH 1
static int Bits;
static int B32p;
static int FullWidth;
static const char * Ltabs;
static int Ahex;
static int RelayBlockSize;

enum REG_X {X_AL = 0, X_CL, X_DL, X_BL, X_AH, X_CH, X_DY, X_BH, X_NONE,
            X_EAX= 0, X_ECX,X_EDX,X_EBX,X_ESP,X_EBP,X_ESI,X_EDI};
enum OP_MOD {OPMOD_RPTR = 0, OPMOD_RP_DISP8, OPMOD_RP_DISPLONG, OPMOD_IMMED};
enum OPREG16 {OR16_BXSI=0, OR16_BXDI, OR16_BPSI, OR16_BPDI,
	    OR16_SI, OR16_DI, OR16_BP, OR16_BX, OR16_NONE};
enum OPREG16 MAPMOD1632[] = {OR16_NONE, OR16_NONE, OR16_NONE, OR16_BX,
			 OR16_NONE, OR16_NONE, OR16_SI, OR16_DI};
enum REG_X   MAPMOD3216[] = {X_NONE, X_NONE, X_NONE, X_NONE,
			     X_ESI, X_EDI, X_NONE, X_EBX};

const char *REG_NAMES[3][9]
   =
    { {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "???"},
      {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "???"},
      {"al", "cl", "dl", "bl", "ah", "dh", "ch", "bh", "???"}};



#include "opsintel.h"


static enum MACH_OP RetOp;

static std::vector<unsigned char> Code;

static PCTR Pctr = 0;
static int GensymCtr = 0;
static PCTR Lowest_Fix8_Unresolved;

static int dep_sorter (const DepPair& A, const DepPair& B) {
    if (A.affector < B.affector)
	return -1;
    if (A.affector > B.affector)
	return 1;
    if (A.affected < B.affected)
	return -1;
    if (A.affected > B.affected)
	return 1;
    return 0;
}


std::vector<RelayDef> RelayDefTable ;//(350, 1.5f); //Internal definitions
std::unordered_set<Rlysym*> RelayDefQuickCheck;

//These two maps are inverses of each other
std::vector<Rlysym*> RelayRefTable ;// (500, 1.5f);  //External references
std::unordered_map<Rlysym*, RLID> RIDMap;

std::vector<Timer> Timers ;//(50, 1.5f);

std::vector<DepPair> DependentPairTable;// (2000, 1.5f);
std::vector<struct Fixup> FixupTable;// (100, 1.5f);
std::vector<LabelEntry> LabelTable;



void RC_error (int fatal, const char* s, ...) {
    va_list ap;
    va_start (ap, s);
    vfprintf (stderr, s, ap);
    fprintf(stderr, "\n");
    if (fatal)
	exit(3);
}

size_t get_file_size(const char* filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;
    else
        return -1;
}

static int TraceOpt = 0;
static int CheckOpt = 0;
static int ListOpt = 0;
static FILE *ListFile;

void list (const char* s, ...) {
    if (ListOpt) {
	va_list ap;
	va_start (ap, s);
	vfprintf (ListFile, s, ap);
    }
}


Jtag RetCF, RetOne, RetZero;

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
    Lowest_Fix8_Unresolved = std::numeric_limits<PCTR>::max();
    for (Fixup& F : FixupTable) {
	if (F.tag != NULL && F.width == SINGLE_WIDTH && F.pc < Lowest_Fix8_Unresolved)
	    Lowest_Fix8_Unresolved = F.pc;
    }
    if (ListOpt && TraceOpt)
	list ("; LOWEST UNRESOLVED F.pc = %04X\n", Lowest_Fix8_Unresolved);
}

void GensymTag(Jtag& tg) {
    sprintf (tg.lab, "g%04d", GensymCtr++);
}

static char Label_Pending[20] = {0};

void Outtag (Jtag& tg) {
    if (Label_Pending[0])
	list ("%0*X\t\t%s:\n", Ahex, Pctr, Label_Pending);
    strcpy (Label_Pending, tg.lab);
}

void DefineTag (Jtag& tg) {
    tg.defined = tg.have_pc =1;
    tg.tramp_defined = 0;
    tg.pctr = Pctr;
    GensymTag(tg);
    Outtag (tg);
}

int disp_ok (PCTR pc, PCTR v) {
    int disp;
    if (v < pc) {
	disp = pc - v;
	disp = -disp;
    }
    else disp = v - pc;
    return (-126+FullWidth < disp && disp < 126-FullWidth);
//    return (-32 < disp && disp < 32);
}

void FixupFixup (Fixup & F, PCTR pc) {
    int d;
    if (F.width == FullWidth) {
	d = pc-F.pc-FullWidth;
	list (B32p ?
	      ";  FIXUP 32-bit %08X to %s = %08X, disp %08X\n"
	      : ";  FIXUP 16-bit %04X to %s = %04X, disp %04X\n",
	      F.pc, F.tag->lab, pc, d);
	*((PCTR *) &Code[F.pc]) = d;
	F.tag = NULL;
    }
    else {
	d = pc-F.pc-1;
	Jtag& tag = *F.tag;
	list (";  FIXUP  8-bit %0*X to %s = %0*X, disp %02X\n",
	      Ahex, F.pc, tag.lab, Ahex, pc, d);
	if (disp_ok (pc, F.pc))
	    Code[F.pc] = d;
	else {
	    RC_error (0, "Fixup overflow at 0x%0*X", Ahex, pc);
	    list (";*****Fixup overflow at 0x%0*X\n", Ahex, pc);
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
    for (Fixup& F : FixupTable) {
	if (&tag == F.tag)
	    FixupFixup (F, Pctr);
    }
}

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

struct _Ctxt {
    _Ctxt(Jtag* j_, CtxtOp o_) : tag(j_), op(o_) {}
    Jtag * tag;
    CtxtOp op;
};

typedef struct _Ctxt Ctxt;

static int dep_list_sorter( const DepPair&A, const DepPair& B) {
    Rlysym *ar = RelayRefTable[A.affector];
    Rlysym *br = RelayRefTable[B.affector];
    if (ar->n < br->n)
	return -1;
    if (ar->n > br->n)
	return 1;
    if (ar->type != br->type)
	return strcmp (redeemRlsymId (ar->type), redeemRlsymId (br->type));
    if (A.affected < B.affected)
	return -1;
    if (A.affected > B.affected)
	return 1;
    return 0;
}

RLID RelayId (Sexpr s) {
    Rlysym* rlptr = s.u.r;
    if (RIDMap.count(rlptr))
        return RIDMap[rlptr];

    RLID relay_id = (RLID)RelayRefTable.size();
    RelayRefTable.emplace_back(rlptr);
    RIDMap[rlptr] = relay_id;

    list ("%sv$%s\tequ\tbyte ptr 0%Xh\n",
          Ltabs, s.PRep().c_str(), relay_id * RelayBlockSize);
    return relay_id;
}


PCTR RlsymOffset (Rlysym * r) {	/* called from writetko */
    return RelayBlockSize*RIDMap[r];
}

static PCTR RelayOffset (Sexpr s) {
    return RelayBlockSize*RelayId(s);
}

void OutputListingLine
   (const char* opmnem, const char unsigned * bytes, int bytect, const char* opd) {
again:

    int tcol = B32p ? 24 : 16;
    size_t llen = Label_Pending[0] ? strlen (Label_Pending+1) : 0;
    int cc = Ahex+2;

    list ("%0*X  ", Ahex, Pctr);
    if (llen < 8)
	for (int i = 0; i < bytect; i++) {
	    list ("%02X", bytes[i]);
	    cc+=2;
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

void outbytes_raw (const char* opmnem, char unsigned * bytes, int bytect, const char* opd) {
    if (ListOpt)
	OutputListingLine (opmnem, bytes, bytect, opd);
    unsigned long end = Pctr + bytect;
    if (Bits == 16 && end >= (unsigned long) 0xFFFF)
	RC_error (2, "Code size exceeds 65K limit for 16-bit compilation.");
    Code.insert(Code.end(), bytes, bytes+bytect); //poor man's iterators
    Pctr += bytect;
}

int Outdata (int opd, int ct, unsigned char * b) {
    for (int x = ct; x > 0; x--) {
	*b++ = opd & 0xFF;
	opd >>= 8;
    }
    return ct;
}

int EncodeOpd (unsigned char * b, int opd, REG_X R1, REG_X R2, int immed) {
    int bc = 0;
    enum OP_MOD mod;

    if (immed)
	mod = OPMOD_IMMED;
    else if (opd == 0)
	/* we dont do absolute number yet. */
	if (    (B32p && (R2 == X_EBP || R2 == X_ESP))
	     || (!B32p && (R2 != X_ESI && R2 != X_EDI && R2 != X_EBX)))
	    mod = OPMOD_RP_DISP8;
	else 
	    mod = OPMOD_RPTR;
    else if (opd >= -128 && opd < 128)
	mod = OPMOD_RP_DISP8;
    else mod = OPMOD_RP_DISPLONG;
    
    if (!B32p && !immed) {
	enum OPREG16 reg16 = MAPMOD1632 [R2];
	if (reg16 == OR16_NONE)
	    RC_error (2, "[%s] addressing not supported in 16-bit mode.",
		      REG_NAMES[1][R2]);
	R2 = (REG_X) reg16;
    }

    b[bc++] = ((int) mod << 6) | ((int) R1 << 3) | (int) R2;

    if (mod != OPMOD_IMMED && mod != OPMOD_RPTR)
	bc += Outdata ((int) opd,
		       (mod == OPMOD_RP_DISP8) ? SINGLE_WIDTH : FullWidth,
		       b+bc);
    return bc;
}

void DisasOpd (MACH_OP op, char *buf, unsigned char * b,
	       const char *opd, const char *prefix) {
    if (!ListOpt)
	return;

    char memopd[40];

    unsigned char rmbyte = *b++;
    int flags = Ops[op].flags;
    OP_MOD mode = (OP_MOD) (rmbyte >> 6);

    int R1 = (rmbyte >> 3) & 7;
    int R2 = rmbyte &  7;
    int op8bit = !!(flags & OPF_8BIT);

    if (!B32p && mode != OPMOD_IMMED)
	R2 = MAPMOD3216[(int)R2];
    const char *r2name = REG_NAMES[B32p][R2];

    switch (mode) {
	case OPMOD_IMMED:
	    if (op8bit)
		r2name = REG_NAMES[2][R2];
	    strcpy (memopd, r2name);
	    break;
	case OPMOD_RPTR:
	/* we dont do absolute number (looks like [ebp]/[esp]) yet. */
	    if (!opd) {
		sprintf (memopd, "[%s]", r2name);
		break;
	    }				/* fall thru if opd given*/
	case OPMOD_RP_DISP8:
	case OPMOD_RP_DISPLONG:
	{
	    char stropd[24];
	    if (opd)
		sprintf (stropd, "+%s%s", prefix ? prefix : "", opd);
	    else {
		long disp;
		if (mode == OPMOD_RP_DISP8)
		    disp = (char) *b;
		else
		    if (B32p)
			disp = *((long *)b);	/* low-endian platform assumption! */
		    else
			disp = *((short *)b);/* low-endian platform assumption!*/
		sprintf (stropd, "%c%ld", (disp < 0) ? '-' : '+', disp);
	    }
	    sprintf (memopd, "[%s%s]", r2name, stropd);
	    break;
	}
    }

    if (flags & OPF_NOREGOP)
	strcpy (buf, memopd);
    else {
	int r1bp = B32p;
	if (op8bit && op != MOP_MOVZX8)
	    r1bp = 2;
	const char * r1name = REG_NAMES[r1bp][R1];

	if (flags & OPF_RVOPD)
	    sprintf (buf, "%s,%s", memopd, r1name);
	else
	    sprintf (buf, "%s,%s", r1name, memopd);
    }
}


void outinst_general (MACH_OP op, int immed,
		      REG_X r_accum, REG_X r_base, int opd, const char * listopd) {
    unsigned char bytes[12];
    char dbuf[50] = "";
    int bc = 0;
    if (Ops[op].flags & OPF_0F)
	bytes[bc++] = 0x0F;
    bytes[bc++] = Ops[op].opcode;

    unsigned char * osave = bytes+bc;
    bc += EncodeOpd (bytes+bc, opd, r_accum, r_base, immed);
    DisasOpd (op, dbuf, osave, listopd, NULL);
    outbytes_raw (Ops[op].mnemonic, bytes, bc, dbuf);
}


void outinst_raw (MACH_OP op, const char * str, PCTR opd) {
    unsigned char bytes[12];
    char dbuf[50]= "";
    int bc = 0;
    if (Ops[op].flags & OPF_0F)
	bytes[bc++] = 0x0F;
    bytes[bc++] = Ops[op].opcode;
    switch (op) {

	case MOP_AND:
	case MOP_OR:
	case MOP_LDAL:
	case MOP_TST:
	    bc += EncodeOpd (bytes+bc, (int) opd,
			     (op == MOP_TST) ? X_BL : X_AL, X_ESI, 0);
	    DisasOpd (op, dbuf, bytes+1, str, "v$");
	    break;

	case MOP_JZ:
	case MOP_JNZ:
	case MOP_JMP:
	    bytes[bc++] = (opd-Pctr-2) &0xFF;
	    strcpy (dbuf, str);
	    break;

	case MOP_JMPL:
	    bc += Outdata ((int) opd, FullWidth, bytes+bc);
	    sprintf (dbuf, "long %s", str);
	    break;

	case MOP_XOR:
	case MOP_CLZ:
	case MOP_STZ:
	    bytes[bc++] = opd;
	    sprintf (dbuf, "al,%d", opd);
	    break;
	case MOP_LDBLI8:
	    bytes[bc++] = opd;
	    sprintf (dbuf, "bl,%d", opd);
	    break;

	case MOP_RPUSH:
	case MOP_RPOP:
	    bytes[0] |=  opd;
	    strcpy (dbuf, REG_NAMES[B32p][opd]);
	    break;

	case MOP_RRET:
	    bc += Outdata ((int) opd, 2, bytes+bc);
	    sprintf (dbuf, "%d", (int) opd);
	    break;

	case MOP_RETF:
	case MOP_RET:
	case MOP_LEAVE:
	default:
	    break;
    }
    outbytes_raw (Ops[op].mnemonic, bytes, bc, dbuf);
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


void check_fix8_overflows() {
    int jumps = 0;
    Jtag jump;
top:
    if (Lowest_Fix8_Unresolved < Pctr)
	if (!disp_ok (Pctr+5+FullWidth, Lowest_Fix8_Unresolved))
            for (Fixup & F : FixupTable) {
		if (F.tag != NULL)
		    if (F.pc == Lowest_Fix8_Unresolved) {
			list ("; TRAMP OUT %s, ref pc = %04X\n",
			      F.tag->lab, F.pc);
			if (jumps++ == 0) {
			    GensymTag(jump);
			    jump.defined = jump.have_pc = 0;
			    jump.tramp_defined = 0;
			    outinst_raw (MOP_JMP, jump.lab, Pctr+1);
			    RecordFixup (jump, Pctr - 1, SINGLE_WIDTH);
			}
			TrampJump (MOP_NOJUMP, *F.tag, 0);
			Jtag *t = F.tag;
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
	    t.defined = t.have_pc = 1;
	    t.tramp_defined = 0;
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
	else if (tag.have_pc && disp_ok (Pctr+2, tag.pctr))
	    outinst_raw (op, tag.lab, tag.pctr);
	else {
	    Jtag t;
	    GensymTag(t);
	    t.defined = t.have_pc = 1;
	    t.tramp_defined = 0;
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
    outinst_raw (MOP_JMPL, tag.lab, (Bits==32) ? 0xFFFFFFFF : 0xFFFF);
    RecordFixup (tag, Pctr - FullWidth, FullWidth);
    if (op != MOP_JMP && jumparound)
	DefineTagPC (jump);
}

void outjmp (MACH_OP op, Jtag& tag) {
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

void RecordDependent (RLID affector) {

    /* Elim duplicates - will speed up runtime and simplify obj seg writer. */
    for (auto& dp : DependentPairTable)
	if (dp.affector == affector)
	    return;
    DependentPairTable.emplace_back(affector, DefiningRelay);
}


void CompileRlysym (Sexpr s, Ctxt * ctxt, int backf) {
    if (s.type != Lisp::RLYSYM)
	RC_error (3, "Non-Rlysym handed to CompileRlysym.");
    RecordDependent (RelayId(s));
    Jtag* tag = ctxt->tag;
    PCTR offset = RelayOffset(s);
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
    Et.defined = 0;
    if (!same) {
	if (ctxt->op == CT_VAL)
	    newctxt.tag = ctxt->tag;
	else {
	    GensymTag(Et);
	    Et.defined = 1;
	    Et.tramp_defined = 0;
	    Et.have_pc = 0;
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
    RelayDefTable.emplace_back(relay_sym, Pctr);
    RelayDefQuickCheck.emplace(relay_sym);
    FixupTable.clear();
    ComputeLowestFix8Unresolved();
}


void CompileRelayDef (Sexpr s) {
    Jtag t;
    Sexpr rlysexpr = CAR(s);
    sprintf (t.lab, "c$%s", rlysexpr.u.r->PRep().c_str());
    PushRelayDef (rlysexpr);
    list ("\n%s\tpublic\t%s\n", Ltabs, t.lab);
    DefineTagPC(t);
    Ctxt ctxt (&RetCF, CT_VAL);
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

*/

short get_relay_type_index(const char * name);

void CompileEntryThunk () {

    const char * name = ENTRY_THUNK_NAME;
    int rtx = get_relay_type_index (name); // flushed "noncanonical" case-sensitive stuff
    Rlysym * r = new Rlysym(0, rtx, NULL);

    Sexpr rly;
    rly.u.r = r;
    PushRelayDef (rly);

    list ("\n%s\tpublic\t%s\n%s%s:\n", Ltabs, name, Ltabs, name);
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
    RetCF.defined = RetOne.defined = RetZero.defined = 0;
    RetCF.have_pc = RetOne.have_pc = RetZero.have_pc = 0;
    RetCF.tramp_defined = RetOne.tramp_defined = RetZero.tramp_defined = 0;
    Pctr = 0;

    fasd_init();
}

void PrintRelayTable () {
    list ("\f%d relays defined by code:    "
	    "ISD (Internal symbol dictionary)\n\nIndex\tPC\tFn Size\tRelay\n",
	    RelayDefTable.size());
    int i = 0;
    for (auto& rdte : RelayDefTable) {
	list ("%4d\t%04X\t%4X\t%s\n",
              i++, rdte.pc, rdte.size , rdte.sym->PRep().c_str());
    }
    list ("\n");
    list ("\fRelay Table (%d relays referenced):    "
	  "ESD (External symbol dictionary)\nIndex\tOffset\tRelay\n\n",
	  RelayRefTable.size());
    i = 0;
    for (auto rte : RelayRefTable) {
        list ("%4d\t%04X\t%s\n", i++, RlsymOffset (rte), rte->PRep().c_str());
    }
    list ("\n");
    list ("Code seg size %04X = %ud\n", Pctr, Pctr);
    auto rblock_size = RelayBlockSize*RelayRefTable.size();
    list ("Relay DS size %04X = %ud\n", rblock_size, rblock_size);
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
    list ("\n%d dependent pairs:\n", DependentPairTable.size());
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
        std::string SARep = rsp1->PRep();
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
            std::string path (replace_filename(fname, CADR(s).u.s));
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
    if (B32p)
	CompileEntryThunk();
    CompileFile (f, fname);
    list ("%s  end	\n", Ltabs);
}

std::string merge_ext(const char * input, const char* new_ext, bool force) {
    auto fspath = fs::path(input);
    if (force || !fspath.has_extension())
        fspath.replace_extension(new_ext);
    return fspath.string();
}

void CallWtko (const char * path, const char * opath, time_t timer,
	       int cversion, char * compdesc) {
    
    TKO_INFO tki;
    tki.Isd = RelayDefTable.data();
    tki.isd_count = RelayDefTable.size();
    tki.Fxt = FixupTable.data();
    tki.fixup_count = 0;
    tki.Code = Code.data();
    tki.Dpt = DependentPairTable.data();
    tki.dpt_count = (int)DependentPairTable.size();
    tki.Esd = RelayRefTable.data();
    tki.esd_count = (int)RelayRefTable.size();
    tki.Tmr = Timers.data();
    tki.tmr_count = (int)Timers.size();
    tki.static_len = tki.esd_count*RelayBlockSize;
    tki.code_len = Pctr;
    tki.time = timer;
    tki.Frm = fasd_data (tki.frm_count);
    tki.Ats = fasd_atsym_data (tki.ats_count);
    tki.Architecture = "INTEL x86";
    tki.arch_characterization = 0;
    tki.compiler = compdesc;
    tki.compiler_version = cversion;
    std::string merged_path;
    if (opath[0] != '\0')
	merged_path = opath;
    else
        merged_path = merge_ext(path, ".tko", true);
    if (B32p)
	write_tko32 (merged_path.c_str(), tki);
    else
	write_tko (merged_path.c_str(), tki);
    printf ("%d (0x%x) code bytes generated.\n", Pctr, Pctr);
    printf ("%ld relay%s defined, %ld referenced.\n",
	    RelayDefTable.size(), (RelayDefTable.size() == 1) ? "" : "s",
	    RelayRefTable.size());
    printf ("%s written, %ld bytes.\n", merged_path.c_str(), get_file_size(merged_path.c_str()));
}

static bool OpenListing (const char * path, std::string& lpath) {
    if (lpath.empty())
        lpath = merge_ext(path, ".lst", true);
    ListFile = fopen (lpath.c_str(), "w");
    if (ListFile == NULL) {
	fprintf (stderr, "Can't open listing file %s for output.\n", lpath.c_str());
        return false;
    }
    return true;
}

int main (int argc, char ** argv) {
#if NXSYSMac
    int compiler_bits = 64;   //not running on same platform as the object.
#else
    int compiler_bits = sizeof(int) * 8;
#endif
    B32p = (compiler_bits > 16);

    std::string opath;
    std::string lpath;
    char compdesc[100];
    sprintf (compdesc, "BSG Windows Relay Compiler Version 2 (%d-bit) of %s %s",
	     compiler_bits, __DATE__, __TIME__);

    fprintf (stdout, "%s\n", compdesc);
    fprintf (stdout, "Copyright (c) Bernard S. Greenberg 1994, 1996, 2019\n");

#if NXSYSMac
    fprintf(stdout, "Macintosh MacOS clang++ implementation\n");
#endif

    if (argc < 2) {
usage:
        auto execpath = fs::path(argv[0]);
        fprintf (stderr, "Usage: %s source{.trk} {args}\nArgs:\n", execpath.string().c_str());
	fprintf (stderr,
		 "  -L    Make listing to source.lst\n"
		 "  -32   Produce 32-bit object file%s\n"
		 "  -16   Produce 16-bit Version 1 object file%s\n"
		 "  -Fo:nondefault_outputpath (default is source.tko)\n"
		 "  -Fl:nondefault_listingpath (default is source.lst)\n"
		 "  -C    Special debug checking\n"
		 "  -T    Internal compiler tracing to listing\n",
		 B32p  ? " (default)" : "",
		 !B32p ? " (default)" : "");
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
            std::string argval= stoupper(arg+1);
	    if (argval == "L")
		ListOpt = 1;
	    else if (argval == "T")
		TraceOpt = 1;
	    else if (argval == "C")
		CheckOpt = 1;
	    else if (argval == "16")
		B32p = 0;
	    else if (argval == "32")
		B32p = 1;
	    else if (!strncmp(argval.c_str(), "FO:", 3)) {
		if (arg[3] == '\0') {
		    fprintf (stderr, "Output pathname missing after /Fo:\n");
		    goto usage;
		}
                opath = arg+4;
	    }
	    else if (!strncmp (argval.c_str(), "FL:", 3)) {
		if (arg[3] == '\0') {
		    fprintf (stderr, "Listing pathname missing after /Fl:\n");
		    goto usage;
		}
                lpath = arg+4;
		ListOpt = 1;
	    }
	    else goto usage;
	}
	else fpath = arg;

    }
    if (fpath == NULL)
	goto usage;

    if (B32p) {
	Bits = 32;
	Ltabs = "\t\t\t";
	RetOp = MOP_RET;
	RelayBlockSize = 32;
        printf("Output for 32-bit Windows environment.\n");
    }
    else {
	Bits = 16;
	FullWidth = 2;
	Ltabs = "\t\t";
	RetOp = MOP_RETF;
	RelayBlockSize = 28;
        printf("Output for 16-bit Windows environment. Good luck.\n");
    }
    FullWidth = Bits/8;
    Ahex = Bits/4;

    std::string input_path = merge_ext(fpath, ".trk", false);

    FILE* f = fopen (input_path.c_str(), "r");
    if (f == NULL) {
	fprintf (stderr, "Cannot open %s for reading.", input_path.c_str());
        return 3;
    }

    time_t timer;
    struct tm *tblock;
    timer = time(NULL);
    tblock = localtime (&timer);
    if (ListOpt)
	if (!OpenListing (input_path.c_str(), lpath))
            return 2;

    list ("; %d-bit compilation of %s at %s", Bits, fpath, asctime(tblock));
    list (";  by %s\n", compdesc);
    list (";  Copyright Bernard S. Greenberg (c) 1994, 1996\n\n");
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
    CallWtko (input_path.c_str(), opath.c_str(), timer, 2, compdesc);
    return 0;
};

// I suppose this is right even on Windows.
void MessageBox(void*, const char* msg, const char* title, int) {
    fprintf(stderr, "%s: %s\n",msg, title);
}
