#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>

#include "lisp.h"
#include "rcdcls.h"

#ifdef _WIN32
#ifdef _MSC_VER
#pragma pack(push,tkov1call)
#pragma pack(2)
#endif

#include "tkov1.h"
#ifdef _MSC_VER
#pragma pack(pop,tkov1call)
#endif
#else
#include "tkov1.h"
#endif

typedef  struct _TKO_VERSION_1_COMPONENT_HEADER COMPHDR;

const int N_RLTYPES = 200;
short type_translate_table[N_RLTYPES];
short rltypet[N_RLTYPES];
const char *type_name_table[N_RLTYPES];
static int RLTypes = 0;
static unsigned short RTypeHeapSize = 0;

static void Write_Header (FILE* f, TKO_INFO& inf) {
    struct _TKO_VERSION_1_HEADER h;
    h.magic = TKO_VERSION_1_MAGIC;
    strcpy (h.magic_string, TKO_VERSION_1_STRING);
    h.version = TKO_VERSION_1;
    h.header_size = sizeof (h);
    h.code_len = inf.code_len;
    h.static_len = inf.static_len;
    h.time = inf.time;
    char * g = getenv("user");
    if (g == NULL)
	 g = getenv("USER");
    memset (h.user, 0, sizeof (h.user));
    if (g != NULL)
	strncpy (h.user, g, sizeof(h.user)-1);
    fwrite (&h, 1, sizeof(h), f);
};

static void Write_Text (FILE* f, TKO_INFO& inf) {
    COMPHDR h;
    h.compid = TKOI_TXT;
    h.number_of_items = inf.code_len;
    h.length_of_item = 1;
    h.length_of_block = inf.code_len;
    fwrite (&h, 1, sizeof(h), f);    
    fwrite (inf.Code, 1, inf.code_len, f);
};

static void Write_Fixup_Table (FILE* f, TKO_INFO &inf) {
    COMPHDR h;
    h.compid = TKOI_RLD;
    h.number_of_items = inf.fixup_count;
    h.length_of_item = sizeof(PCTR);
    h.length_of_block = h.length_of_item*h.number_of_items;
    fwrite (&h, 1, sizeof(h), f);    
    for (int i = 0; i < inf.fixup_count; i++) {
	PCTR fx = inf.Fxt[i].pc;
	fwrite (&fx, 1, sizeof(PCTR), f);
    }
}

static void RegisterLispRelayID (short rlid) {
    if (type_translate_table[rlid] >= 0)
	return;
    if (RLTypes >= N_RLTYPES)
	RC_error (1, "Object Relay Type overflow.");
    int x = RLTypes++;
    type_translate_table[rlid] = x;
    const char * s = redeemRlsymId (rlid);
    type_name_table [x] = s;
    RTypeHeapSize+= strlen(s) + 1;
}

static void Compute_Relay_Types (TKO_INFO& inf) {
    for (int i = 0; i < N_RLTYPES; i++)
	type_translate_table[i] = -1;
    for (int i = 0; i < inf.isd_count;i++)
	RegisterLispRelayID (inf.Isd[i].sym->type);
    for (int i = 0; i < inf.esd_count;i++)
	RegisterLispRelayID (inf.Esd[i]->type);
}

static void Write_Relay_Types (FILE* f) {
    COMPHDR h;
    h.compid = TKOI_RTT;
    h.number_of_items = RLTypes;
    h.length_of_item = 1;
    h.length_of_block = RTypeHeapSize;
    fwrite (&h, 1, sizeof(h), f);
    for (int i = 0; i < RLTypes; i++) {
	const char * s = type_name_table [i];
	fwrite (s, 1, strlen(s)+1, f);
    }
    h.compid = TKOI_RTD;
    h.number_of_items = RLTypes;
    h.length_of_item = sizeof(short);
    h.length_of_block = RLTypes*h.length_of_item;
    fwrite (&h, 1, sizeof(h), f);
    short off = 0;
    for (int i = 0; i < RLTypes; i++) {
	fwrite (&off, sizeof(short), 1, f);
	off += strlen (type_name_table [i]) + 1;
    }
}

static void WriteRlysym (FILE* f, Rlysym* r, unsigned int data) {
    TKO_DEFBLOCK b;    
    b.n = r->n;
    b.type = type_translate_table [r->type];
    b.data = data;
    fwrite (&b, sizeof(b), 1, f);
}

static void Write_ESD (FILE*f, TKO_INFO& inf) {
    COMPHDR h;
    h.compid = TKOI_ESD;
    h.number_of_items = inf.esd_count;
    h.length_of_item = sizeof (TKO_DEFBLOCK);
    h.length_of_block = h.length_of_item * h.number_of_items;
    fwrite (&h, 1, sizeof(h), f);
    for (int i = 0; i < inf.esd_count; i++) {
	Rlysym * r = inf.Esd[i];
	WriteRlysym (f, r, RlsymOffset(r));
    }
}
    
static void Write_ISD (FILE*f, TKO_INFO& inf) {
    COMPHDR h;
    h.compid = TKOI_ISD;
    h.number_of_items = inf.isd_count;
    h.length_of_item = sizeof (TKO_DEFBLOCK);
    h.length_of_block = h.length_of_item * h.number_of_items;
    fwrite (&h, 1, sizeof(h), f);
    for (int i = 0; i < inf.isd_count; i++) {
	RelayDef *r = &inf.Isd[i];
	WriteRlysym (f, r->sym, r->pc);
    }
}

static void Write_DPD (FILE* f, TKO_INFO& inf) {
    COMPHDR h;
    h.compid = TKOI_DPD;
    h.number_of_items = 0;
    h.length_of_item = 0;
    h.length_of_block = 0;
    DepPair * Dpt = inf.Dpt;
    int Dpt_count = inf.dpt_count;
    RLID last_affector = 0xFFFF;
    int i, j;
    for (i = 0; i < Dpt_count; i++) {
	if (Dpt[i].affector != last_affector) {
	    last_affector = Dpt[i].affector;
	    h.number_of_items++;
	    h.length_of_block += sizeof (TKO_DPTE_HEADER);
	    for (j = i; j < Dpt_count && Dpt[j].affector ==last_affector;j++);
	    h.length_of_block += sizeof (short)*(j - i);
	}
    }
    fwrite (&h, 1, sizeof(h), f);
    last_affector = 0xFFFF;
    for (i = 0; i < Dpt_count; i++) {
	if (Dpt[i].affector != last_affector) {
	    last_affector = Dpt[i].affector;
	    TKO_DPTE_HEADER hh;
	    hh.affector = last_affector;
	    for (j = i; j <Dpt_count && Dpt[j].affector == last_affector; j++);
	    hh.count = j - i;
	    fwrite (&hh, 1, sizeof (TKO_DPTE_HEADER), f);
	    for (j = i; j < i + hh.count; j++) {    
		unsigned short x = Dpt[j].affected;
		fwrite (&x, 1, sizeof (unsigned short), f);
	    }
	}
    }
};

static void Write_TMR (FILE* f, TKO_INFO& inf) {
    COMPHDR h;
    h.compid = TKOI_TMR;
    h.number_of_items = inf.tmr_count;
    h.length_of_item = sizeof (TKO_TIMER_DEF);
    h.length_of_block = h.number_of_items * h.length_of_item;
    fwrite (&h, 1, sizeof(h), f);
    TKO_TIMER_DEF t;
    for (int i = 0; i < h.number_of_items;i++) {
	t.rlyisdid = inf.Tmr[i].id;
	t.time = inf.Tmr[i].time;
	fwrite (&t, 1, sizeof(TKO_TIMER_DEF), f);
    }
}

static void Write_ATS (FILE* f, TKO_INFO& inf) {
    COMPHDR h;
    h.compid = TKOI_ATS;
    h.number_of_items = inf.ats_count;
    h.length_of_item = 1;
    h.length_of_block = 0;
    for (int i = 0; i < inf.ats_count; i++)
	h.length_of_block += strlen(inf.Ats[i]) + 1;
    fwrite (&h, 1, sizeof(h), f);
    for (int j = 0; j < inf.ats_count; j++) {
	int sl = (int)strlen(inf.Ats[j]);
	if (sl > 255)
	    RC_error (1, "Atomic symbol name longer than 255, can't dump.");
	putc (sl, f);
	fwrite (inf.Ats[j], 1, sl, f);
    }
}

static void Write_FRM (FILE* f, TKO_INFO& inf) {
    COMPHDR h;
    h.compid = TKOI_FRM;
    h.number_of_items = inf.frm_count;
    h.length_of_item = 1;
    h.length_of_block = inf.frm_count;
    fwrite (&h, 1, sizeof(h), f);
    fwrite (inf.Frm, h.length_of_item, h.number_of_items, f);
}

static void Write_EOF (FILE* f) {
    COMPHDR h;
    h.compid = TKOI_EOF;
    h.number_of_items = 0;
    h.length_of_item = 0;
    h.length_of_block = 0;
    fwrite (&h, 1, sizeof(h), f);
};

void write_tko (const char * fname, TKO_INFO&inf) {
    FILE * f = fopen (fname, "wb");
    if (f == NULL)
	RC_error (2, "Cannot open %s for binary write.", fname);
    Write_Header (f, inf);
    Write_Text (f, inf);
    if (inf.fixup_count > 0)
	Write_Fixup_Table (f, inf);
    Compute_Relay_Types(inf);
    Write_Relay_Types (f);
    Write_ESD(f, inf);
    Write_ISD(f, inf);
    if (inf.tmr_count > 0)
	Write_TMR(f, inf);
    Write_DPD(f, inf);
    if (inf.ats_count > 0)
	Write_ATS (f, inf);
    if (inf.frm_count > 0)
	Write_FRM (f, inf);
    Write_EOF (f);
    fclose(f);
}
