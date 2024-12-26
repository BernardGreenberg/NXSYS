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
#include "ldgDisassemble.hpp"
#include "DisasUtil.h"
#include "relays.h"
#include "cccint.h"

#include "RCarm64.h"
using std::vector, std::string, std::set;

bool haveDisassembly = false; /* globally addressible */
static int Top = 0;
static vector<string> Lines;
static set<LPCTR> ForwardJumps;

void InitLdgDisassembly() {
    Lines.clear();
    ForwardJumps.clear();
    haveDisassembly = false;
    Top = 0;
}

void ldgDisassemble(Relay* r) {
    ForwardJumps.clear();
    haveDisassembly = true;
    Lines.push_back(r->RelaySym.PRep() + ":          (compiled relay)");
    unsigned char * p = (unsigned char*) (r->exp);
    /* We stop when we find a RET with no forward jumps pending. */
    for (int i = 0; i < 300; i++ ){
        LPCTR pctr = (LPCTR)p;
        ArmInst inst = *((ArmInst*)p);
        string addr = FormatString("%p %8X  ",p, inst);
        
        string S = addr + DisassembleARM(inst, (LPCTR)p);
        unsigned int opcode = TOPBYTE(inst);
        if (opcode == TOPBYTE(ARM::ldr_storage)) {
            int stat_index = extract_bits(inst, 20, 10);
            Relay** rptrarray = (Relay**)Compiled_Linkage_Sptr;
            Relay * r = rptrarray[stat_index];
            S += " " + r->RelaySym.PRep();
            if (r->State == 0)
                S += "   (DROPPED)";
            else
                S += "   (PICKED)";
        } else if ((opcode == TOPBYTE(ARM::tbz)) || (opcode == TOPBYTE(ARM::tbnz))) {
            int disp = extract_bits(inst, 18, 5) * sizeof(ArmInst);
            if ((disp & 0x8000) == 0) //forward!  No "sign bit".
                ForwardJumps.insert(pctr + disp); //note that duplicates are eliminated!
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
