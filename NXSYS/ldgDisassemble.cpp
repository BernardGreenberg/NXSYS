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

struct TbzUpdateRecord {
    TbzUpdateRecord(LPCTR _L, int _index) {pctr = _L; lineIndex = _index;}
    LPCTR  pctr;
    int    lineIndex;
};

struct RelayUpdateRecord {
    RelayUpdateRecord(Relay* _r, int _index) {relay = _r; lineIndex = _index;}
    Relay*  relay;
    int    lineIndex;
};

bool haveDisassembly = false; /* globally addressible */
static int Top = 0;
static vector<string> Lines;

static vector<TbzUpdateRecord> UpdateSchedule;
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

static string GenerateUpdatableLine(LPCTR pctr, bool record) {
    ArmInst inst = *((ArmInst*)pctr);

    string prefix = FormatString(" %012X  %8X  ", pctr, inst);
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
    return S;
}

string GenerateHeaderLine(Relay * r, int lenb, bool record) {
    if (record)
        ShownRelays.emplace_back(r, Lines.size());
    int lenw = lenb / sizeof(ArmInst);
    return (string(" ") + r->RelaySym.PRep() + ":          (compiled relay)   " +
            FormatString(" length %d(%d wds) = 0x%x(0x%02x) ", lenb, lenw, lenb, lenw) +
            InterpretState(r));
}



void ldgDisassemble(Relay* r) {
    int lenb = GetRelayFunctionLength(r);
    assert(lenb >= 0);
    int lenw = lenb/sizeof(ArmInst);
    haveDisassembly = true;
    Lines.push_back(GenerateHeaderLine(r, lenb, true));
    unsigned char * p = (unsigned char*) (r->exp);

    for (int i = 0; i < lenw; i++ ){
        LPCTR pctr = (LPCTR)p;
        Lines.push_back(GenerateUpdatableLine(pctr, true));
        p += sizeof(ArmInst);
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
    for (const auto& e : UpdateSchedule)
        Lines[e.lineIndex] = GenerateUpdatableLine(e.pctr, false);

    int y = Top;
    for (auto& s : Lines) {
        int height = DrawLine(s, y, dc);
        y += height;
    }
}
