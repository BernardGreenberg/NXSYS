//
//  ldgDisassemble.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 12/25/24.
//  Copyright © 2024 BernardGreenberg. All rights reserved.
//


#include <vector>
#include <string>
#include "STLExtensions.h"

#include "windows.h"
#include "ldgDisassemble.hpp"
#include "relays.h"
#include "cccint.h"

#include "RCarm64.h"
using std::vector, std::string;

static vector<string> Lines;
static int Top = 0;
string DisassembleARM (ArmInst inst, uint64_t pctr);


bool haveDisassembly = false;

void InitLdgDisassembly() {
    Lines.clear();
    haveDisassembly = false;
    Top = 0;
}

/* Bits number in arm documentation style, 31 to 0 left to right in 32-bit dword */
static unsigned int extract_bits(unsigned int data, int high, int low) {
    int answer = data >> low;
    int bits = high - low + 1;
    int mask = (1 << bits) - 1;
    return answer & mask;
}



#include <set>

std::set<int64_t> ForwardJumps;

void ldgDisassemble(Relay* r) {
    ForwardJumps.clear();
    haveDisassembly = true;
    Lines.push_back(r->RelaySym.PRep() + ":          (compiled relay)");
    unsigned char * p = (unsigned char*) (r->exp);
    /* We stop when we find a RET with no forward jumps pending. */
    for (int i = 0; i < 300; i++ ){
        int64_t pctr = (int64_t)p;
        ArmInst inst = *((ArmInst*)p);
        string addr = FormatString("%p %8X  ",p, inst);
        
        string S = addr + DisassembleARM(inst, (unsigned long)(p));
        unsigned int opcode = TOPBYTE(inst);
        if (opcode == TOPBYTE(ARM::ldr_storage)) {
            int stat_disp = extract_bits(inst, 20, 10);
            Relay**rptrarray = (Relay**)Compiled_Linkage_Sptr;
            Relay * r = rptrarray[stat_disp];
            S += r->RelaySym.PRep();
            if (r->State == 0)
                S += "   (DROPPED)";
            else
                S += "   (PICKED)";
        } else if ((opcode == TOPBYTE(ARM::tbz)) || (opcode == TOPBYTE(ARM::tbnz))) {
            int fld = ((inst >> 5) << 2) & 0x0000FFFF;
            if ((fld &  0x00008000) == 0) { //forward!  No "sign bit".
                int64_t target = pctr+fld;
                ForwardJumps.insert(target); //note that duplicates are eliminated!
            }
        }
        Lines.push_back(S);

        /* a RET is not really the end of function unless there are no
           pending forward jumps*/
        if (opcode == TOPBYTE(ARM::ret) && ForwardJumps.size() == 0) {
            break;
        }
        if (ForwardJumps.count(pctr))
            ForwardJumps.erase(pctr);
        p += 4;
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
    int y = Top;
    for (auto& s : Lines) {
        int height = DrawLine(s, y, dc);
        y += height;
    }
}
