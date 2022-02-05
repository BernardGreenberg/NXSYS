#include <windows.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <memory.h>
#include <dir.h>
#include <dos.h>
#include "lisp.h"
#include "relays.h"
#include "tkov1.h"
#include "fasl.h"

extern int InterpretTopLevelForm (char * fname, Sexpr s);

typedef  struct _TKO_VERSION_1_COMPONENT_HEADER COMPHDR;
typedef  unsigned short PCTR;

static HGLOBAL Text_handle = 0;
static BYTE * Static_ptr = NULL;
static BYTE * Text_ptr = NULL;
static int Text_len, Static_len;
static char *StringTab;
static short *StringXTab;
static Relay **ESD, **ISD;
static UINT Exsel;
static short ISD_Count, ESD_Count;
void* Compiled_Linkage_Sptr;
static Sexpr *AtsymTable = NULL;
static int AtsymCount = 0;


static int Read_Header (FILE* f) {
    struct _TKO_VERSION_1_HEADER h;
    fread (&h, 1, sizeof(struct _TKO_VERSION_1_HEADER), f);
    if (h.magic != TKO_VERSION_1_MAGIC ||
	!!strcmp (h.magic_string, TKO_VERSION_1_STRING) ||
	h.version != 1) {
	MessageBox (0, "This is not a Relay Compiler object file.",
		    "Can't load code file", MB_OK|MB_ICONSTOP);
	return 0;
    }

    Text_handle = GlobalAlloc (0, h.code_len);
    if (Text_handle == NULL)
	FatalAppExit (0, "Can't alloc compiled code text.");
    Static_len = h.static_len;
    Static_ptr = new char unsigned[Static_len];
    if (Static_ptr == NULL) {
	MessageBox (0, "Can't alloc static for code seg.",
		    "Code File problem", MB_OK|MB_ICONSTOP);
	return 0;
    }
    Text_ptr = GlobalLock(Text_handle);
    Text_len = h.code_len;
    return 1;
}

void Read_RLD (FILE* f, int n) {
    PCTR p;
    for (int i = 0; i < n; i++) {
	fread (&p, 1, sizeof (PCTR), f);
	*((PCTR*) (Text_ptr+p)) += FP_OFF (Text_ptr);
    }
}

Sexpr tko_db_to_relay (TKO_DEFBLOCK& b) {
    return intern_rlysym (b.n, StringTab+StringXTab[b.type]);
}

void Read_ESD (FILE* f, int n) {
    ESD_Count = n;
    ESD = new Relay *[n];
    if (ESD == NULL)
	FatalAppExit (0, "Can't allocate external symbol dictionary.");
    TKO_DEFBLOCK b;
    for (int i = 0; i < n; i++) {
	fread (&b, 1, sizeof (TKO_DEFBLOCK), f);
	Relay * r = (Relay *) (Static_ptr+b.data);
	InitRelay (*r, tko_db_to_relay (b));
	r->Flags |= LF_Shref;
	ESD[i] = r;
    }
}

void Read_ISD (FILE* f, int n) {
    ISD_Count = n;
    ISD = new Relay *[n];
    if (ISD == NULL)
	FatalAppExit (0, "Can't allocate internal symbol dictionary.");
    TKO_DEFBLOCK b;
    for (int i = 0; i < n; i++) {
	fread (&b, 1, sizeof (TKO_DEFBLOCK), f);
	/* might be reporting -- who knows? */
	Relay * r = CreateReportingRelay (tko_db_to_relay (b));
	(void *) (r->exp) = MK_FP (FP_SEG (Text_ptr), b.data);
	r->Flags |= LF_CCExp;
	ISD[i] = r;

    }
}

void Read_TMR (FILE* f, int n) {
    TKO_TIMER_DEF d;
    for (int i = 0; i < n; i++) {
	fread (&d, 1, sizeof (TKO_TIMER_DEF), f);
	/* the compiled relay, value referenced by code, is the
	"out(pu)ter".  Its exp, however, is the code for the "controller". */
	ISD[d.rlyisdid] = DefineTimerRelayFromObject (ISD[d.rlyisdid], d.time);
    }
}

void Read_Dependents (FILE* f, int nblocks) {
    for (int i = 0; i < nblocks; i++) {
	struct TKO_DPTE_HEADER h;
	fread (&h, 1, sizeof (TKO_DPTE_HEADER), f);
	Relay * Affector = ESD[h.affector];
	Affector->Dependents = new Relay*[h.count];
	Affector->NDepArraySize = Affector -> NDependents = h.count;
	for (int j = 0; j < h.count; j++) {
	    short x;
	    fread (&x, 1, sizeof(x), f);
	    Affector -> Dependents[j] = ISD[x];
	}
    }
};

void ReadAtsymTable (FILE* f, int natoms) {
    char sbuf[257];
    AtsymCount = natoms;
    if (natoms == 0)
	return;
    AtsymTable = new Sexpr[natoms];
    if (AtsymTable == NULL)
	FatalAppExit (0, "Can't allocate FASL atomic symbol table.");
    for (int i = 0; i < natoms; i++) {
	int l = getc (f) & 0xFF;
	fread (sbuf, 1, l, f);
	sbuf[l] = '\0';
	AtsymTable[i] = intern (sbuf);
    }
}

short fasl_getw(FILE* f) {
    int blow = getc(f) & 0xFF;
    int bhigh = getc(f) & 0xFF;
    return (bhigh << 8) | blow;
}

Sexpr FaslForm (FILE* f) {
    int ctlb, w;
    Sexpr s, last, current;
    ctlb = getc (f) & 0xFF;
    switch (ctlb) {
	case FASD_EOF:
	    s.type = L_NULL;
	    s.u.s = NULL;
	    break;
	case FASD_VERSION:
	    if (getc(f) != 1)
		FatalAppExit (0, "FASD version not 1.");
	    s = NIL;
	    break;
	case FASD_CHAR:
	    s.type = L_CHAR;
	    s.u.c = getc(f);
	    break;
	case FASD_ATSYM:
	    s = AtsymTable [fasl_getw (f)];
	    break;
	case FASD_STRING:
	    w = fasl_getw(f);
	    s.type = L_STRING;
	    s.u.s = (char *) malloc (w+1);
	    fread (s.u.s, 1, w, f);
	    s.u.s[w] = '\0';
	    break;
	case FASD_1BNUM:
	    s.type = L_NUM;
	    s.u.n = getc(f) & 0xFF;
	    break;
	case FASD_2BNUM:
	    s.type = L_NUM;
	    s.u.n = fasl_getw (f);
	    break;
	case FASD_LIST:
	    w = fasl_getw (f);
	    last = NIL;
	    for (; w > 0; w--) {
		current = CONS (FaslForm (f), NIL);
		if (last == NIL)
		    s = current;
		else
		    CDR(last) = current;
		last = current;
	    }   
	    break;
	default:
	    FatalAppExit (0, "Non-understood FASL object.");
    }   
    return s;
}

void ReadFaslForms (FILE* f) {
    for (;;) {
	Sexpr s = FaslForm (f);
	if (s.type == L_NULL)
	    return;
	else if (!(s == NIL)) {
	    InterpretTopLevelForm (NULL, s);
	    dealloc_ncyclic_sexp (s);
	}
    }
}

void LoadObj_FinishUp () {
    delete ESD;
    ESD= NULL;
    Compiled_Linkage_Sptr = Static_ptr;
    GlobalFix(Text_handle);
    Exsel = AllocDStoCSAlias (FP_SEG (Text_ptr));
    for (int i = 0; i < ISD_Count; i++)
	if (FP_SEG (ISD[i]->exp) == FP_SEG (Text_ptr))
	    FP_SEG (ISD[i]->exp) = Exsel;
    delete ISD;
    if (AtsymTable != NULL)
	delete AtsymTable;
    AtsymTable = NULL;
    ISD = NULL;
    delete StringTab;
    StringTab = NULL;
    delete StringXTab;
    StringXTab = NULL;
}
    
static void MergePath (char * ref, char * fn, char * answer) {

    char dir[MAXDIR], drive[MAXDRIVE], file[MAXFILE], path [MAXPATH],
	rdir[MAXDIR], rdrive[MAXDRIVE], rfile[MAXFILE],  ext[MAXEXT];

    fnsplit (ref, rdrive, rdir, rfile, NULL);
    fnsplit (fn, drive, dir, file, ext);
    if (drive[0] == '\0') strcpy (drive, rdrive);
    if (dir[0] == '\0') strcpy (dir, rdir);
    if (file[0] == '\0') strcpy (file, rfile);
    if (ext[0] == '\0')
	strcpy (ext, ".TKO");
    fnmerge (answer, drive, dir, file, ext);
}

int LoadRelayObjectFile (char * ref, char * fn) {
    char path [MAXPATH];
    MergePath (ref, fn, path);
    FILE * f = fopen (path, "rb");
    if (f == NULL) {
	MessageBox(0, path, "Can't load code file.", MB_OK|MB_ICONSTOP);
	return 0;
    }
    Text_handle = 0;
    StringXTab = NULL;
    StringTab = NULL;
    Static_ptr = NULL;
    Text_ptr = NULL;
    if (!Read_Header (f))
	return 0;
    for (;;) {
	COMPHDR h;
	memset (&h, 1, sizeof (h));
	fread (&h, 1, sizeof (h), f);
	if (h.compid > TKOI_LAST) {
	    FatalAppExit (0, "Invalid section header in code file.");
	    exit (10);
	}
	if (h.compid == TKOI_EOF)
	    break;

	switch (h.compid) {
	    case TKOI_TXT:
		fread (Text_ptr, 1, h.length_of_block, f);
		break;
	    case TKOI_RLD:		/* relocation dict */
		Read_RLD (f, h.number_of_items);
		break;
	    case TKOI_RTT:
		StringTab = new char [h.length_of_block];
		fread (StringTab, 1, h.length_of_block, f);
		break;
	    case TKOI_RTD:
		StringXTab = new short [h.number_of_items];
		fread (StringXTab, h.length_of_item, h.number_of_items, f);
		break;
	    case TKOI_TMR:
		Read_TMR (f, h.number_of_items);
		break;
	    case TKOI_ESD:
		Read_ESD (f, h.number_of_items);
		break;
	    case TKOI_ISD:
		Read_ISD (f, h.number_of_items);
		break;
	    case TKOI_DPD:
		Read_Dependents (f, h.number_of_items);
		break;
	    case TKOI_ATS:
		ReadAtsymTable (f, h.number_of_items);
		break;
	    case TKOI_FRM:
		ReadFaslForms (f);
		break;
	    default:
		fseek (f, (long) h.length_of_block, SEEK_CUR);
	}

    }
    fclose(f);
    LoadObj_FinishUp();
    return 1;
}

void CleanupObjectMemory () {
    if (Static_ptr != NULL){
	delete Static_ptr;
	Static_ptr = NULL;
    }
    if (Text_handle != NULL) {
	GlobalUnfix(Text_handle);
	GlobalUnlock (Text_handle);
	GlobalFree (Text_handle);
	Text_handle = NULL;
	FreeSelector(Exsel);
	Exsel = 0;
    }
}
