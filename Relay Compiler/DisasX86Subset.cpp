//
//  DisasX86Subset.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 1/8/25.
//  Copyright © 2025 BernardGreenberg. All rights reserved.
//
//  Disassembles 1 X86-64 (64-bit mode assumed) instruction, returning interpretation and length
//  in the X86DisRV resturn value structure.  Note that only the specific patterns generated
//  by the Relay Compiler are recognized and understood.  If the relay compiler is substantially
//  augmented, this table, or the whole strategy, will need to change.
//
//  Note that this is used by both DumpTko when applied to an X86 TKO, or the simulator itself
//  on an X86 platform, having loaded one.
//

#include <stdio.h>
#include <cassert>
#include <vector>
#include <string>

using std::vector, std::string;

#include "STLExtensions.h"

#include "NXX86.h"
#include "DisasUtil.h"

namespace NXX86 {

string FmtInst(unsigned char * ip, int nbytes) {
    string result;
    for (int i = 0; i < 7; i++) {
        if (i < nbytes)
            result += FormatString("%02X", ip[i]);
        else
            result += "  ";
    }
    result += " ";
    return result;
}

uint32_t collect_32(unsigned char* p) { //collect low-endian-stored 32-bit number
    uint32_t acc = 0;
    for (int i = 0; i < 4; i++) {
        acc <<= 8;
        acc |= p[3-i];
    }
    return acc;
}

/*
 KST - "Known Stuff Table" -- values copied from NASM assembler listing. Processor 64-bit mode assumed.
 
 2400           and    al,0
 0C01           or     al,1
 3401           xor    al,1
 0F95C0         setnz  al
 480FB6C0       movzx  rax,al
 4989C8         mov    r8,rcx
 4989F8         mov    r8,rdi
 498B5000       mov    rdx,QWORD PTR [r8+v$265NWZ]
 498B9000040000 mov    rdx,QWORD PTR [r8+v$266PBS]
 74FF           jz     g0000
 7503           jnz    g0001
 840A           test   BYTE PTR [rdx],cl
 8A02           mov    al,BYTE PTR [rdx]
 B901000000     mov    rcx,0x00000001
 E9FFFFFFFF     jmp    long g0508
 FFD2           call   rdx
 FFD6           call   rsi
 C3             ret
 */


/* Scheme - expect somewhat fewer than 256 known items; at any rate, this is effectively a hash table
 on the first byte.  The value of dispatch[first_byte] is the INDEX into KnownPatterns of the
 appropriate entry (hash-siblings threaded by "next"). Note that 0 means "no entry", not entry 0. */

char unsigned DispatchArray[256] = {}; /* guaranteed init to 0's */

struct X86Pattern KnownPatterns [] {
    {I_NULL, {}, 0, "null entry 1DUMY ---  can't have 0"},
    {I_RET,  {0xc3}, 1, "ret" },
    {I_CRSI, {0xff, 0xd6}, 2, "call\trsi" },
    {I_CRDX, {0xff, 0xd2}, 2, "call\trdx"},
    {I_ANDAL,{0x24, 0x00}, 2, "and\tal,0"},
    {I_XORAL,{0x34, 0x01}, 2, "xor\tal,1"},
    {I_ORAL, {0x0C, 0x01}, 2, "or \tal,1"},
    {I_TEST, {0x84, 0x0A}, 2, "test\tBYTE PTR [rdx],cl"},
    {I_LMOV, {0x8A, 0x02}, 2, "mov\tal, BYTE PTR [rdx]"},
    {I_M8DI, {0x49, 0x89, 0xF8}, 3, "mov\tr8,rdi"},
    {I_M8CX, {0x49, 0x89, 0xC8}, 3, "mov\tr8,rcx"},
    {I_MVZXA,{0x48, 0x0F, 0xB6, 0xC0}, 4, "movzx\trax,al"},
    {I_SETNZ,{0x0F, 0x95, 0xC0}, 3, "setnz\tal"},
    {I_JZ,   {0x74}, 2, "jz \t0x%llX", IMM_1B | JMPDISP},
    {I_JNZ,  {0x75}, 2, "jnz\t0x%llX", IMM_1B | JMPDISP},
    {I_JMPS, {0xEB}, 2, "jmp\t0x%llx", IMM_1B | JMPDISP},
    {I_JMPL, {0xE9}, 5, "jmp\tlong\t0x%llX", IMM_4B | JMPDISP},
    {I_C2CX, {0xB9}, 5, "mov\trcx,0x%X", IMM_4B | IMM_CONSTANT},
    {I_LDX1B,{0x49, 0x8B, 0x50}, 4, "mov\trdx,QWORD PTR [r8+0x%X]", IMM_1B | RLYDISP},
    {I_LDX4B,{0x49, 0x8B, 0x90}, 7, "mov\trdx,QWORD PTR [r8+0x%X]", IMM_4B | RLYDISP},
};

void EnsureDispatch() {
    static bool done = false;
    if (done)
        return;
    /* guaranteed zeros in static array */
    int i = 0;
    for (X86Pattern& e : KnownPatterns) {
        if (i != 0) {
            auto b = e.Data[0];
            e.next = DispatchArray[b];
            DispatchArray[b] = (unsigned char)i;
        }
        i++;
    }
    done = true;
}

struct X86DisRV DisassembleDecodedX86(unsigned char* ip, uint64_t Pctr, const X86Pattern& E) {
    X86DisRV RV;
    RV.disassembly = FmtInst(ip, E.must_exist_bytes) + E.disassembly;
    RV.byte_count = E.must_exist_bytes;
    int32_t accum = 0;
    if (E.flags & IMM_1B)
        accum = (int)(char)ip[E.Data.size()];
    else if (E.flags & IMM_4B)
        accum = collect_32(ip+E.Data.size());
    /* Note that both jumps and relay references come in 8 and 32 bit immediates. */
    if (E.flags & JMPDISP){
        LPCTR ea = Pctr + E.must_exist_bytes + (int64_t)accum; // accum is signed 32
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

struct X86DisRV DisassembleX86(unsigned char* ip, uint64_t Pctr, uint64_t nitems) {
    EnsureDispatch();
    assert(nitems > 0);
    
    for (auto d = DispatchArray[*ip]; d != 0; d = KnownPatterns[d].next) {
        const X86Pattern& E = KnownPatterns[d];
        if (E.match(ip, nitems))
            return DisassembleDecodedX86(ip, Pctr, E);
    }

    /* We get here if code is not in our array/map */
    struct X86DisRV RV;
    RV.byte_count = std::min((unsigned)nitems, (unsigned)3);
    RV.disassembly = FmtInst(ip, RV.byte_count) + "Unknown";
    return RV;
    
}

} //namespace NXX86
