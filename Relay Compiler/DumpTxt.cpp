//
//  Untitled.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 12/17/24.
//  Copyright © 2024 BernardGreenberg. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "tkov2.h"
#include "RCArm64.h"

#include <vector>
#include <map>

using std::string, std::unordered_map, std::vector;

static std::map<unsigned int, string> ESD;  //link_offset->relay name
static std::map<unsigned int, string> ISD; //text offset->relay name

/* Bits number in arm documentation style, 31 to 0 left to right in 32-bit dword */
static unsigned int extract_bits(unsigned int data, int high, int low) {
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
    if (txtchp->length_of_item != 4)
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
    for (int i = 0; i < txtchp->number_of_items; i++) {
        ArmInst inst = *instp;
        if (ISD.count(Pctr))
            printf("\n%s:\n", ISD[Pctr].c_str());
        printf("%06X  %08X", Pctr, inst);
        char unsigned opcode = TOPBYTE(inst);
        
        switch (opcode) {
            case TOPBYTE(ARM::tbz):
            case TOPBYTE(ARM::tbnz):
            {
                int fld = ((inst >> 5) << 2) & 0x0000FFFF;
                if (fld &  0x00008000)
                    fld |= 0xFFFF0000;
                int target = (int)Pctr+fld;
                const char * opstr = (opcode == 0x37) ? "tbnz" : "tbz ";
                printf ("   %-4s    x0, #0, %06X", opstr, target);
            }
                break;
                
            case TOPBYTE(ARM::ret):
                printf ("   ret     x%d", extract_bits(inst, 9, 5));
                printf ("\n");
                break;
            case TOPBYTE(ARM::movz_0):
                printf ("   mov     x0, #%d", (inst >> 5) & 1);
                break;
                
            case TOPBYTE(ARM::ldr_storage):
            {
                int ptrdisp = extract_bits(inst, 20, 10) * 8;
                printf("   ldr     x0, [x2, #0x%04x]   ; %s", ptrdisp, ESD[ptrdisp].c_str());
                break;
            }
                
            case TOPBYTE(ARM::ldrb_reg):
                printf("   ldrb    x0, [x0, #0]");
                break;
                
            case TOPBYTE(ARM::mov_rr):
                printf("   mov     x%d, x%d", extract_bits(inst, 4, 0), extract_bits(inst, 20, 16));
                break;

            case TOPBYTE(ARM::eor_imm):
            {
                int imms = extract_bits(inst, 15, 10);
                int immr = extract_bits(inst, 21, 16);
                int imm = (imms << 6) | immr;
                printf("   eor     x%d, x%d, #%d",
                       extract_bits(inst, 4, 0),
                       extract_bits(inst, 9, 5),
                       imm);
            }
                break;
            default:
                break;
        }
        
        printf("\n");
        instp++;
        Pctr += 4;
    }
}

