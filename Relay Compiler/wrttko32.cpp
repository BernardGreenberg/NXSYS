#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>

#include "lisp.h"
#include "rcdcls.h"
#include "tkov2.h"

#include <unordered_map>
#include <vector>

typedef  struct _TKO_VERSION_2_COMPONENT_HEADER COMPHDR;

const int N_RLTYPES = 100;
static int type_translate_table[N_RLTYPES];
//static int rltypet[N_RLTYPES];
static const char *type_name_table[N_RLTYPES];
static int RLTypes = 0;
static unsigned int RTypeHeapSize = 0;

static void Write_Header (FILE* f, TKO_INFO& inf) {
    struct _TKO_VERSION_2_HEADER h;
    h.magic = TKO_VERSION_2_MAGIC;
    strcpy (h.magic_string, TKO_VERSION_2_STRING);
    h.version = TKO_VERSION_2;
    h.header_size = sizeof (h);
    h.code_len = inf.code_len;
    h.static_len = inf.static_len;
    h.time = inf.time;
    h.compat_code_len = 0;
    h.compat_static_len = 0;
    h.archindex = inf.arch_characterization;
    h.compiler_version = inf.compiler_version;
    strcpy (h.arch, inf.Architecture);
    h.bits = inf.bits;
    const char * g = getenv("user");
    if (g == NULL)
	 g = getenv("USER");
    memset (h.user, 0, sizeof (h.user));
    if (g != NULL)
	strncpy (h.user, g, sizeof(h.user)-1);
    fwrite (&h, 1, sizeof(h), f);
};

static void Write_CID (FILE* f, TKO_INFO& inf) {
    COMPHDR h;
    h.compid = TKOI_CID;
    h.number_of_items = 1;
    h.length_of_item = (int)strlen (inf.compiler);
    h.length_of_block = h.length_of_item;
    fwrite (&h, 1, sizeof(h), f);    
    fwrite (inf.compiler, 1, h.length_of_item, f);
};

static void Write_Text (FILE* f, TKO_INFO& inf) {
    COMPHDR h;
    h.compid = TKOI_TXT;
    h.number_of_items = inf.code_ct;
    h.length_of_item = inf.code_item_len;
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

static void RegisterLispRelayID (int rlid) {
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
    COMPHDR h_comp;
    h_comp.compid = TKOI_DPD;
    h_comp.number_of_items = 0;
    h_comp.length_of_item = 0;
    h_comp.length_of_block = 0;
    const auto *dpt = inf.Dpt;
    int Dpt_count = inf.dpt_count;
    std::unordered_map<RLID, std::vector<RLID>> depend_map;
    for (int i = 0; i < Dpt_count; i++, dpt++) {
        depend_map[dpt->affector].push_back(dpt->affected);
        h_comp.length_of_block += sizeof(RLID);
    }
    h_comp.number_of_items = (int) depend_map.size();
    h_comp.length_of_block += h_comp.number_of_items * sizeof(TKO_DPTE_HEADER);
    fwrite(&h_comp, 1, sizeof(COMPHDR), f);

    for (const auto& e : depend_map) {
        TKO_DPTE_HEADER h_dpte;
        h_dpte.affector = e.first;
        h_dpte.count = (int) e.second.size();
        fwrite(&h_dpte, 1, sizeof(h_dpte), f);

        for (RLID affected : e.second)
            fwrite (&affected, 1, sizeof (RLID), f);
    }
}

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

void write_tko32 (const char * fname, TKO_INFO&inf) {
    FILE * f = fopen (fname, "wb");
    if (f == NULL)
	RC_error (2, "Cannot open %s for binary write.", fname);
    Write_Header (f, inf);
    Write_CID (f, inf);
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
