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
 *2400            and   al,0
 *0C01            or    al,1
 *3401          xor    al,1
 *0F95C0         setnz    al
 *480FB6C0       movzx    rax,al
 *4989C8         mov    r8,rcx
 *4989F8         mov    r8,rdi
 *498B5000       mov    rdx,QWORD PTR [r8+v$265NWZ]
 *498B9000040000     mov    rdx,QWORD PTR [r8+v$266PBS]
 *74FF         jz    g0000
 *7503         jnz    g0001
 *840A         test    BYTE PTR [rdx],cl
 *8A02         mov    al,BYTE PTR [rdx]
 *B901000000     mov    rcx,0x00000001
 E9FFFFFFFF     jmp    long g0508
 *C3            ret
 *FFD6         call    rsi
*/

static uint32_t collect_32(unsigned char* p) {
    uint32_t acc = 0;
    for (int i = 0; i < 4; i++) {
        acc <<= 8;
        acc |= p[3-i];
    }
    return acc;
}

static string FmtAB(unsigned char * ip, uint32_t Pctr, int nbytes) {
    string display = FormatString("%08X  ", Pctr);
    for (unsigned int i = 0; i < 7; i++) {
        if (i < nbytes)
            display += FormatString("%02X", ip[i]);
        else
            display += "  ";
    }
    display += " ";
    return display;
}
struct X86DisRV DisassembleX86(unsigned char* ip, uint32_t Pctr, uint32_t nitems) {
    struct X86DisRV RV;
    if (Pctr >= nitems)
        return RV;

    if (ip[0] == 0xC3) {
        RV.disassembly = FmtAB(ip, Pctr, 1) + "ret";
        RV.byte_count = 1;
        return RV;
    }
        
    if (Pctr + 1 >= nitems) {
      unclear:
        RV.disassembly = FmtAB(ip, Pctr, 1) + "NOT KNOWN 1";
        RV.byte_count = 1;
        return RV;
    }

    if (ip[0] == 0xFF && ip[1] == 0xD6) {
        RV.disassembly = FmtAB(ip, Pctr, 2) + "call\trsi";
        RV.byte_count = 2;
        return RV;
    }
    
    if (ip[0] == 0xFF && ip[1] == 0xD2) {
        RV.disassembly = FmtAB(ip, Pctr, 2) + "call\trdx";
        RV.byte_count = 2;
        return RV;
    }

    if (ip[0] == 0x24 && ip[1] == 0x00) {
        RV.disassembly = FmtAB(ip, Pctr, 2) + "and\tal,0";
        RV.byte_count = 2;
        return RV;
    }

    if (ip[0] == 0x34 && ip[1] == 0x01) {
        RV.disassembly = FmtAB(ip, Pctr, 2) + "xor\tal,1";
        RV.byte_count = 2;
        return RV;
    }
    
    if (ip[0] == 0x0C && ip[1] == 0x01) {
        RV.disassembly = FmtAB(ip, Pctr, 2) + "or\tal,1";
        RV.byte_count = 2;
        return RV;
    }
    
    if (ip[0] == 0x84 && ip[1] == 0x0A) {
        RV.disassembly = FmtAB(ip, Pctr, 2) + "test\tBYTE PTR [rdx],cl";
        RV.byte_count = 2;
        return RV;
    }
    
    if (ip[0] == 0x8A && ip[1] == 0x02) {
        RV.disassembly = FmtAB(ip, Pctr, 2) + "mov\tal, BYTE PTR [rdx]";
        RV.byte_count = 2;
        return RV;
    }
    
    if (ip[0] == 0x74) {
        int disp = (char)ip[1];
        uint32_t ea = Pctr+2+disp;
        RV.disassembly = FmtAB(ip, Pctr, 2) + FormatString("jz\t0x%X", ea);
        RV.byte_count = 2;
        return RV;
    }
    
    if (ip[0] == 0x75) {
        int disp = (char)ip[1];
        uint32_t ea = Pctr+2+disp;
        RV.disassembly = FmtAB(ip, Pctr, 2) + FormatString("jnz\t0x%X", ea);
        RV.byte_count = 2;
        return RV;
    }
    
    if (ip[0] == 0xEB) {
        int disp = (char)ip[1];
        uint32_t ea = Pctr+2+disp;
        RV.disassembly = FmtAB(ip, Pctr, 2) + FormatString("jmp\t0x%X", ea);
        RV.byte_count = 2;
        return RV;
    }
    
    if (Pctr + 2 >= nitems) {
        RV.disassembly = FmtAB(ip, Pctr, 2) + "NOT KNOWN 2";
        RV.byte_count = 2;
        return RV;
    }

    if (ip[0] == 0x0F && ip[1] == 0x95 && ip[2] == 0xC0) {
        RV.disassembly = FmtAB(ip, Pctr, 3) + "setnz\tal";
        RV.byte_count = 3;
        return RV;
    }
    if (ip[0] == 0x49 && ip[1] == 0x89) {
        if (ip[2] == 0xC8) {
            RV.disassembly = FmtAB(ip, Pctr, 3) + "mov\tr8,rcx";
            RV.byte_count = 3;
            return RV;
        }
        else if (ip[2] == 0xF8){
            RV.disassembly = FmtAB(ip, Pctr, 3) + "mov\tr8,rdi";
            RV.byte_count = 3;
            return RV;
        }
    }
    if (Pctr + 4 <= nitems && ip[0] == 0x48 && ip[1] == 0x0F && ip[2] == 0xB6 && ip[3] == 0xC0) {
        RV.disassembly = FmtAB(ip, Pctr, 4) + "movzx\trax,al";
        RV.byte_count = 4;
        return RV;
    }
    if (ip[0] == 0xB9 && Pctr+5 <= nitems) {
        uint32_t constant = collect_32(ip+1);
        RV.disassembly = FmtAB(ip, Pctr, 5) + FormatString("mov\trcx,0x%X", constant);
        RV.byte_count = 5;
        return RV;
    }
    if (Pctr + 4 <= nitems && ip[0] == 0x49 && ip[1] == 0x8B && ip[2] == 0x50) {
        uint32_t disp = ip[3];
        RV.relay_ref_index = disp;
        RV.have_ref_relay = true;
        RV.byte_count = 4;
        RV.disassembly = FmtAB(ip, Pctr, 4) + FormatString("mov\trdx,QWORD PTR [r8+0x%X]", disp);
        return RV;
    }
    if (Pctr + 7 <= nitems && ip[0] == 0x49 && ip[1] == 0x8B && ip[2] == 0x90) {
        RV.byte_count = 7;
        uint32_t disp = collect_32(ip + 3);
        RV.relay_ref_index = disp;
        RV.have_ref_relay = true;
        RV.disassembly = FmtAB(ip, Pctr, 7) + FormatString("mov\trdx,QWORD PTR [r8+0x%X]", disp);
        return RV;
    }
    if (Pctr + 5 <= nitems && ip[0] == 0xE9) {
        int32_t disp = (int)collect_32(ip+1);  // signed!
        int64_t ldisp = (int64_t)disp;
        int64_t ea = ldisp + Pctr + 5;
        RV.byte_count = 5;
        RV.disassembly = FmtAB(ip, Pctr, 5) + FormatString("jmp\tlong\t0x%X", ea);
        return RV;
    }

    RV.byte_count = 3;
    RV.disassembly = FmtAB(ip, Pctr, 3) + "NOT KNOWN 3";
    return RV;
}
