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

#include <vector>
#include <map>


using std::string, std::unordered_map, std::vector;

static std::map<unsigned int, string> ESD;  //link_offset->relay name
static std::map<unsigned int, string> ISD; //text offset->relay name

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
        unsigned int inst = *instp;
        if (ISD.count(Pctr))
            printf("\n%s:\n", ISD[Pctr].c_str());
        printf("%06X  %08X", Pctr, inst);
        char unsigned opcode = inst >> (32-8);
        switch (opcode) {
        case 0x36:
        case 0x37:
            {
                int fld = ((inst >> 5) << 2) & 0x0000FFFF;
                if (fld &  0x00008000)
                    fld |= 0xFFFF0000;
                int target = (int)Pctr+fld;
                const char * opstr = (opcode == 0x37) ? "tbnz" : "tbz ";
                printf ("   %s %06X", opstr, target);
            }
                
            break;
        default:
            break;
        }
        
        printf("\n");
        if (i >= 200) break;
        instp++;
        Pctr += 4;
    }
}

