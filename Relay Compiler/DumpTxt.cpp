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
            auto RV = DisassembleX86(charp+Pctr, Pctr, nitems);
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
            return FormatString ("   %-4s    x0, #0, 0x%4lX", opstr, target);
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
            return "";
    }
}
/*
 TKS - "Table of Known Stuff".  All else is "unknown"
 
 *2400           and    al,0
 *0C01           or     al,1
 *3401           xor    al,1
 *0F95C0         setnz  al
 *480FB6C0       movzx  rax,al
 *4989C8         mov    r8,rcx
 *4989F8         mov    r8,rdi
 *498B5000       mov    rdx,QWORD PTR [r8+v$265NWZ]
 *498B9000040000 mov    rdx,QWORD PTR [r8+v$266PBS]
 *74FF           jz     g0000
 *7503           jnz    g0001
 *840A           test   BYTE PTR [rdx],cl
 *8A02           mov    al,BYTE PTR [rdx]
 *B901000000     mov    rcx,0x00000001
 *E9FFFFFFFF     jmp    long g0508
 *FFD2           call    rdx
 *FFD6           call    rsi
 *C3             ret

*/

static uint32_t collect_32(unsigned char* p) {
    uint32_t acc = 0;
    for (int i = 0; i < 4; i++) {
        acc <<= 8;
        acc |= p[3-i];
    }
    return acc;
}

static string FmtAB(unsigned char * ip, int nbytes) {
    string display;
    for (unsigned int i = 0; i < 7; i++) {
        if (i < nbytes)
            display += FormatString("%02X", ip[i]);
        else
            display += "  ";
    }
    display += " ";
    return display;
}

#define IMM_1B 1
#define IMM_4B 2
#define JMPDISP 4
#define RLYDISP 8
#define IMM_CONSTANT 0x10

struct eltdef {
    std::vector<unsigned char> Data;
    int min_bytes;
    const char * disassembly;
    unsigned int flags = 0;
    unsigned char next = 0;

    /* Can't skimp on the declaration of "howmany" -- it can be very, very big
       and "int"ing it can reduce it to zero and cause infinite looping.
       This happens in the "live" case. */
    bool match(unsigned char* p, uint64_t howmany) const {
        if (howmany < min_bytes)
            return false;
        for (auto u : Data)
            if (u != *p++)
                return false;
        return true;
    }
};

static char unsigned dispatch[256];

/* Scheme = expect less than 256 known items; at any rate, this is effectively a hash table
   on the first byte.  The value of dispatch[first_byte] is the INDEX into eltdef of the
   appropriate entry (hash-siblings threaded by "next"). Note that 0 means "no entry", not entry 0. */

struct eltdef defs [] {
    {{}, 0, "null entry 1DUMY ---  can't have 0"},
    {{0xc3}, 1, "ret" },
    {{0xff, 0xd6}, 2, "call\trsi" },
    {{0xff, 0xd2}, 2, "call\trdx"},
    {{0x24, 0x00}, 2, "and\tal,0"},
    {{0x34, 0x01}, 2, "xor\tal,1"},
    {{0x0C, 0x01}, 2, "or\tal,1"},
    {{0x84, 0x0A}, 2, "test\tBYTE PTR [rdx],cl"},
    {{0x8A, 0x02}, 2, "mov\tal, BYTE PTR [rdx]"},
    {{0x49, 0x89, 0xF8}, 3, "mov\tr8,rdi"},
    {{0x49, 0x89, 0xC8}, 3, "mov\tr8,rcx"},
    {{0x48, 0x0F, 0xB6, 0xC0}, 4, "movzx\trax,al"},
    {{0x0F, 0x95, 0xC0}, 3, "setnz\tal"},
    {{0x74}, 2, "jz \t0x%lX", IMM_1B | JMPDISP},
    {{0x75}, 2, "jnz\t0x%lX", IMM_1B | JMPDISP},
    {{0xEB}, 2, "jmp\t0x%lx", IMM_1B | JMPDISP},
    {{0xE9}, 5, "jmp\tlong\t0x%lX", IMM_4B | JMPDISP},
    {{0xB9}, 5, "mov\trcx,0x%X", IMM_4B | IMM_CONSTANT},
    {{0x49, 0x8B, 0x50}, 4, "mov\trdx,QWORD PTR [r8+0x%X]", IMM_1B | RLYDISP},
    {{0x49, 0x8B, 0x90}, 7, "mov\trdx,QWORD PTR [r8+0x%X]", IMM_4B | RLYDISP},
};

static void setup_x86_dispatch() {
    memset(dispatch, 0, sizeof(dispatch));
    int i = 0;
    for (eltdef& e : defs) {
        if (i != 0) {
            auto b = e.Data[0];
            e.next = dispatch[b];
            dispatch[b] = (unsigned char)i;
        }
        i++;
    }
}

struct X86DisRV DisassembleX86(unsigned char* ip, uint64_t Pctr, uint64_t nitems) {
    struct X86DisRV RV;
    static bool initted = false;
    assert(nitems > 0);
    if (!initted) {
        setup_x86_dispatch();
        initted = true;
    }
    auto b = *ip;
    auto d = dispatch[b];
    while (d != 0) {
        const eltdef& E = defs[d];
        if (E.match(ip, nitems)) {
            RV.disassembly = FmtAB(ip, E.min_bytes) + E.disassembly;
            RV.byte_count = E.min_bytes;
            int32_t accum = 0;
            if (E.flags & IMM_1B)
                accum = (int)(char)ip[E.Data.size()];
            else if (E.flags & IMM_4B)
                accum = collect_32(ip+E.Data.size());
            /* Note that both jumps and relay references come in 8 and 32 bit immediates. */
            if (E.flags & JMPDISP){
                LPCTR ea = Pctr + E.min_bytes + (int64_t)accum; // accum is signed 32
                RV.disassembly = FormatString(RV.disassembly.c_str(), ea);
                return RV;
            }
            if (E.flags & RLYDISP){
                RV.have_ref_relay = true;
                RV.relay_ref_index = accum;
                RV.disassembly = FormatString(RV.disassembly.c_str(), accum);
                return RV;
            }
            if (E.flags & IMM_CONSTANT)
                RV.disassembly = FormatString(RV.disassembly.c_str(), accum);
            return RV;
        }
        d = E.next;
    }
    RV.byte_count = std::min((unsigned)nitems, (unsigned)3);
    RV.disassembly = FmtAB(ip, RV.byte_count) + "Unknown";
    return RV;

}
