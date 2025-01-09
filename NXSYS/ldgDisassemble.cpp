//
//  ldgDisassemble.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 12/25/24.
//  Copyright © 2024 BernardGreenberg. All rights reserved.
//


#include <vector>
#include <string>
#include <set>  /* ordered */
#include "STLExtensions.h"

#include "windows.h"
#include "DisasUtil.h" // must precede next one
#include "ldgDisassemble.hpp"

#include "relays.h"
#include "cccint.h"

#include "RCarm64.h"
using std::vector, std::string, std::set;

#if (defined(__aarch64__) || defined(_M_ARM64))
#define ISARM 1
#endif

struct DisasUpdateRecord {
    DisasUpdateRecord(LPCTR _L, int _index) {pctr = _L; lineIndex = _index;}
    LPCTR  pctr;
    int    lineIndex;
};

struct RelayUpdateRecord {
    RelayUpdateRecord(Relay* _r, int _index) {relay = _r; lineIndex = _index;}
    Relay*  relay;
    int    lineIndex;
};

bool haveDisassembly = false; /* globally addressible, needed by Draftsperson window control */
static int Top = 0;
static vector<string> Lines;

static vector<DisasUpdateRecord> UpdateSchedule;
static vector<RelayUpdateRecord>ShownRelays;

void InitLdgDisassembly() {
    Lines.clear();
    UpdateSchedule.clear();
    ShownRelays.clear();
    haveDisassembly = false;
    Top = 0;
}

static string InterpretState(Relay * r) {
    if (r->State == 0)
        return "(DROPPED)";
    else
        return "(PICKED)";
}
static string GenerateUpdatableLineX86(LPCTR pctr, bool record, int& bytes) {

    auto RV = DisassembleX86((unsigned char *)pctr, pctr, 1LL << 60);
    string D = FormatString(" %012lX  ", pctr) + RV.disassembly;

    if (RV.have_ref_relay) {
        int stat_index = RV.relay_ref_index / sizeof(Relay*);
        Relay** rptrarray = (Relay**)Compiled_Linkage_Sptr;
        Relay * r = rptrarray[stat_index];
        D += " ;" + r->RelaySym.PRep() + "   " + InterpretState(r);
        if (record)
            UpdateSchedule.emplace_back(pctr, Lines.size());
    }
    bytes = RV.byte_count;
    return D;
}

static string GenerateUpdatableLineARM(LPCTR pctr, bool record, int& bytes) {
    ArmInst inst = *((ArmInst*)pctr);

    string prefix = FormatString(" %012lX  %8X  ", pctr, inst);
    string S = prefix + DisassembleARM(inst, pctr);
    unsigned char opcode = TOPBYTE(inst);
    if (opcode == TOPBYTE(ARM::ldr_storage)) {
        int stat_index = extract_bits(inst, 21, 10);
        Relay** rptrarray = (Relay**)Compiled_Linkage_Sptr;
        Relay * r = rptrarray[stat_index];
        S += " " + r->RelaySym.PRep() + "    " + InterpretState(r);
        if (record)
            UpdateSchedule.emplace_back(pctr, Lines.size());
    }
    bytes = sizeof(ArmInst);
    return S;
}

string GenerateHeaderLine(Relay * r, int lenb, bool record) {
    if (record)
        ShownRelays.emplace_back(r, Lines.size());
#if ISARM
    if (RunningSimulatedCompiledCode) {
        return (string(" ") + r->RelaySym.PRep() + ":    (Intel) (compiled relay)" +
                FormatString(" length %d = 0x%x bytes ", lenb, lenb) + InterpretState(r));

    } else {
        int lenw = lenb / sizeof(ArmInst);
        return (string(" ") + r->RelaySym.PRep() + ":    (arm64) (compiled relay)" +
                FormatString(" length %d(%d wds) = 0x%x(0x%02x) ", lenb, lenw, lenb, lenw) +
                InterpretState(r));
    }
#else
    return (string(" ") + r->RelaySym.PRep() + ":          (compiled relay)" +
            FormatString(" (Intel) length %d = 0x%x bytes ", lenb, lenb) +
            InterpretState(r));
#endif
}

void ldgDisassemble(Relay* r) {
    int lenb = GetRelayFunctionLength(r);
    assert(lenb >= 0);
    haveDisassembly = true;
    Lines.push_back(GenerateHeaderLine(r, lenb, true));
    unsigned char * p = (unsigned char*) (r->exp);

    for (int i = 0; i < lenb; ){
        LPCTR pctr = (LPCTR)p;
        int bytes = 0;
#if ISARM
        if (RunningSimulatedCompiledCode)
            Lines.push_back(GenerateUpdatableLineX86(pctr, true, bytes));
        else
            Lines.push_back(GenerateUpdatableLineARM(pctr, true, bytes));
#else
        Lines.push_back(GenerateUpdatableLineX86(pctr, true, bytes));
#endif
        p += bytes;
        i += bytes;
    }
}

static int DrawLine(const string& S, int y, HDC dc) {
    RECT txr;
    txr.left = txr.right =  txr.top = txr.bottom = 0;
    DrawText (dc, S.c_str(), (int)S.size(), &txr,
              DT_TOP | DT_LEFT |DT_SINGLELINE| DT_NOCLIP|DT_CALCRECT);
    int height = txr.bottom - txr.top; // windows orientation
    txr.top = y;
    txr.bottom = height+y;
    DrawText (dc, S.c_str(), (int)S.size(), &txr,
              DT_TOP | DT_LEFT |DT_SINGLELINE| DT_NOCLIP);
    return height;
}
    
static HFONT Font = NULL;

static void ensureFont() {
    if (Font == NULL) {
        LOGFONT lf;
        memset(&lf, 0, sizeof(LOGFONT));
        lf.lfHeight = 20;
        strcpy(lf.lfFaceName, "Courier");
        lf.lfWeight = FW_BOLD;
        Font = CreateFontIndirect(&lf);
    }
}

void ldgDisassembleDraw(HDC dc) {
    ensureFont();
    SelectObject(dc, Font);
    for (const auto& e : ShownRelays)
        Lines[e.lineIndex] = GenerateHeaderLine(e.relay,
                                                GetRelayFunctionLength(e.relay),
                                                false);
    int bytes = 0;  //dummy
    for (const auto& e : UpdateSchedule) {
#if ISARM
        if (RunningSimulatedCompiledCode)
            Lines[e.lineIndex] = GenerateUpdatableLineX86(e.pctr, false, bytes);
        else
            Lines[e.lineIndex] = GenerateUpdatableLineARM(e.pctr, false, bytes);
#else
        Lines[e.lineIndex] = GenerateUpdatableLineX86(e.pctr, false, bytes);
#endif
    }
    int y = Top;
    for (auto& s : Lines) {
        int height = DrawLine(s, y, dc);
        y += height;
    }
}
