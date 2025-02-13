//
//  DumpTxt.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 12/17/24.
//  Copyright © 2024 BernardGreenberg. All rights reserved.
//
//  "Limited disassemble" of both architectures.  The complete encodings for both
//   architectures are very, very complicated, and involve optional features, and
//   apparently, at the outer limits, "professional" disassemblers even show
//   discrepancies.  These disassemblers only disassemble things RelayCompiler is
//   known to produce, and all else is "unknown", which on X86 also includes mistaking
//   its length (always "3" for unknown).

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <cassert>
#include "tkov2.h"
#include "RCArm64.h"
#include "DisasUtil.h"

#include <vector>
#include <map>
#include "STLextensions.h"

using std::string, std::unordered_map, std::vector;

static std::map<unsigned int, string> ESD;  //link_offset->relay name
static std::map<unsigned int, string> ISD; //text offset->relay name

/* Bits number in arm documentation style, 31 to 0 left to right in 32-bit dword */
int extract_bits(unsigned int data, int high, int low) { /*global*/
    int answer = data >> low;
    int bits = high - low + 1;
    int mask = (1 << bits) - 1;
    return answer & mask;
}

static vector<string> CalculateRelayTypeArray(const char* texts, const short* textptrs, int nitems) {
    vector<string> Types;
    for (int i = 0; i < nitems; i++)
        Types.push_back(texts + textptrs[i]);
    return Types;
}

void DumpText(_TKO_VERSION_2_COMPONENT_HEADER* txtchp, unsigned char* fdp, size_t file_length) {
    auto start_dp = fdp;
    auto dp = start_dp + ((_TKO_VERSION_2_HEADER*)fdp)->header_size;
    if (txtchp->length_of_item != 4 && txtchp->length_of_item != 1)
        return;
    auto instp = (unsigned int*) (txtchp+1);
    const short * RnamesTextPtrs = nullptr;
    const char *  RnamesTexts = nullptr;
    vector<string>Types;
    int nstrings = 0;
    
    while(dp < fdp + file_length) {
        auto chp = (_TKO_VERSION_2_COMPONENT_HEADER*) dp;
        auto rdp = dp + sizeof(*chp);
        auto next_block = rdp + chp->length_of_block;
        auto dbp = (TKO_DEFBLOCK*)rdp;
        switch(chp->compid) {
            case TKOI_ISD:
            {
                for (int i = 0; i < chp->number_of_items; i++) {
                    string rname = std::to_string(dbp->n)+Types[dbp->type];
                    ISD[dbp->data] = rname;
                    dbp++;
                }
            }
                break;
                
            case TKOI_ESD:
            {
                for (int i = 0; i < chp->number_of_items; i++) {
                    string rname = std::to_string(dbp->n)+Types[dbp->type];
                    ESD[dbp->data] = rname;
                    dbp++;
                }
            }
                break;
            case TKOI_RTD:
                RnamesTextPtrs = (const short*)rdp;
                nstrings = chp->number_of_items;
                if ((RnamesTexts != nullptr) && Types.size() == 0)
                    Types = CalculateRelayTypeArray(RnamesTexts, RnamesTextPtrs, nstrings);
                break;
            case TKOI_RTT:
                RnamesTexts = (const char *)rdp;
                if ((RnamesTextPtrs != nullptr) && Types.size() == 0)
                    Types = CalculateRelayTypeArray(RnamesTexts, RnamesTextPtrs, nstrings);
                break;
            default:
                break;
        }
        dp = next_block;
    }
    
    unsigned int Pctr = 0;
    auto nitems = txtchp->number_of_items;
    if (txtchp->length_of_item == 1) {
        auto charp = (unsigned char *)instp;
        while (Pctr  < nitems) {
            if (ISD.count(Pctr))
                printf("\n%s:\n", ISD[Pctr].c_str());
            auto RV = NXX86::DisassembleX86(charp+Pctr, Pctr, nitems);
            printf("%06X   %s", (int)Pctr, RV.disassembly.c_str());
            if (RV.have_ref_relay)
                printf("    ; %s", ESD[RV.relay_ref_index].c_str());
            printf("\n");
            Pctr += RV.byte_count;
        }
        printf("\n");
    } else {
        for (int i = 0; i < nitems; i++) {
            ArmInst inst = *instp;
            if (ISD.count(Pctr))
                printf("\n%s:\n", ISD[Pctr].c_str());
            printf("%06X  %08X", Pctr, inst);
            puts( DisassembleARM(inst, Pctr).c_str());
            //printf("\n");
            instp++;
            Pctr += 4;
        }
    }
    
}

string DisassembleARM (ArmInst inst, LPCTR pctr) {
    char unsigned opcode = TOPBYTE(inst);
    switch (opcode) {

        case TOPBYTE(ARM::tbz):
        case TOPBYTE(ARM::tbnz):
        {
            int64_t disp = extract_bits(inst, 18, 5) * sizeof(ArmInst);
            if (disp &  0x8000)
                disp |= 0xFFFFFFFFFFFF0000;
            LPCTR target = pctr+disp;
            const char * opstr = (opcode == 0x37) ? "tbnz" : "tbz ";
            return FormatString ("   %-4s    x0, #0, 0x%4llX", opstr, target);
        }
            
        case TOPBYTE(ARM::ret):
            return FormatString ("   ret     x%d\n", extract_bits(inst, 9, 5));

        case TOPBYTE(ARM::movz_0):
            return FormatString ("   mov     x0, #%d", (inst >> 5) & 1);
            
        case TOPBYTE(ARM::ldr_storage):
        {
            int ptrdisp = extract_bits(inst, 21, 10) * 8;
            string s = FormatString("   ldr     x0, [x2, #0x%04x]   ;", ptrdisp);
            if (ESD.empty())
                return s;
            else
                return s + " " + ESD[ptrdisp];
        }
            
        case TOPBYTE(ARM::ldrb_reg):
            return FormatString("   ldrb    x0, [x0, #0]");
            
        case TOPBYTE(ARM::mov_rr):
            return FormatString("   mov     x%d, x%d", extract_bits(inst, 4, 0), extract_bits(inst, 20, 16));

        case TOPBYTE(ARM::eor_imm):
        {
            int imms = extract_bits(inst, 15, 10);
            int immr = extract_bits(inst, 21, 16);
            int imm = (imms << 6) | immr;
            string v = "#1";
            if (imm != 0) {
                char buf [10];
                snprintf(buf, sizeof(buf), "0x%06X", imm);
                v = string("(value enc as) ") + buf;
            }
            return FormatString("   eor     x%d, x%d, %s",
                                extract_bits(inst, 4, 0),
                                extract_bits(inst, 9, 5),
                                v.c_str());
        }

        default:
            return "Unknown";
    }
}

