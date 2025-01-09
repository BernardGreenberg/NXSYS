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
//  augmented, this table, or the whole strategy, may need to change.
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
#include "DisasUtil.h"

/*
 KST - "Kmown Stuff Table" -- values copied from NASM assembler listing.
 
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

static uint32_t collect_32(unsigned char* p) { //collect low-endian-stored 32-bit number
    uint32_t acc = 0;
    for (int i = 0; i < 4; i++) {
        acc <<= 8;
        acc |= p[3-i];
    }
    return acc;
}

static string FmtInst(unsigned char * ip, int nbytes) {
    string result;
    for (unsigned int i = 0; i < 7; i++) {
        if (i < nbytes)
            result += FormatString("%02X", ip[i]);
        else
            result += "  ";
    }
    result += " ";
    return result;
}

#define IMM_1B 1
#define IMM_4B 2
#define JMPDISP 4
#define RLYDISP 8
#define IMM_CONSTANT 0x10

enum ICODE {I_NULL, I_RET, I_CRSI, I_CRDX, I_ANDAL, I_XORAL, I_ORAL, I_TEST, I_LMOV,
    I_M8DI, I_M8CX, I_MVZXA, I_SETNZ, I_JZ, I_JNZ, I_JMPS, I_JMPL,
    I_C2CX, I_LDX1B, I_LDX4B};

struct X86Pattern {




    enum ICODE icode;
    vector<unsigned char> Data;       /* Bytes that must match exectly */
    int must_exist_bytes;             /* count of bytes that have to exist, incl above */
    const char * disassembly;         /* The "answer" string */
    unsigned int flags = 0;           /* see #defines above */
    unsigned char next = 0;           /* table index of next X86Pattern with same leading byte*/

    /* Can't skimp on the declaration of "count_left" -- it can be very, very big
       and casting it can reduce it to zero and cause infinite looping.
       This happens in the "live" case. */

    /* "Does the byte seq at p match this pattern, in the length left?" */
    bool match(unsigned char* p, uint64_t count_left) const {
        if (count_left < must_exist_bytes)
            return false;
        for (auto u : Data)
            if (u != *p++)
                return false;
        return true;
    }
};

/* Scheme - expect somewhat fewer than 256 known items; at any rate, this is effectively a hash table
   on the first byte.  The value of dispatch[first_byte] is the INDEX into KnownPatterns of the
   appropriate entry (hash-siblings threaded by "next"). Note that 0 means "no entry", not entry 0. */

static char unsigned dispatch[256] = {}; /* guaranteed init to 0's */

static struct X86Pattern KnownPatterns [] {
    {I_NULL, {}, 0, "null entry 1DUMY ---  can't have 0"},
    {I_RET,  {0xc3}, 1, "ret" },
    {I_CRSI, {0xff, 0xd6}, 2, "call\trsi" },
    {I_CRDX, {0xff, 0xd2}, 2, "call\trdx"},
    {I_ANDAL,{0x24, 0x00}, 2, "and\tal,0"},
    {I_XORAL,{0x34, 0x01}, 2, "xor\tal,1"},
    {I_ORAL, {0x0C, 0x01}, 2, "or\tal,1"},
    {I_TEST, {0x84, 0x0A}, 2, "test\tBYTE PTR [rdx],cl"},
    {I_LMOV, {0x8A, 0x02}, 2, "mov\tal, BYTE PTR [rdx]"},
    {I_M8DI, {0x49, 0x89, 0xF8}, 3, "mov\tr8,rdi"},
    {I_M8CX, {0x49, 0x89, 0xC8}, 3, "mov\tr8,rcx"},
    {I_MVZXA,{0x48, 0x0F, 0xB6, 0xC0}, 4, "movzx\trax,al"},
    {I_SETNZ,{0x0F, 0x95, 0xC0}, 3, "setnz\tal"},
    {I_JZ,   {0x74}, 2, "jz \t0x%lX", IMM_1B | JMPDISP},
    {I_JNZ,  {0x75}, 2, "jnz\t0x%lX", IMM_1B | JMPDISP},
    {I_JMPS, {0xEB}, 2, "jmp\t0x%lx", IMM_1B | JMPDISP},
    {I_JMPL, {0xE9}, 5, "jmp\tlong\t0x%lX", IMM_4B | JMPDISP},
    {I_C2CX, {0xB9}, 5, "mov\trcx,0x%X", IMM_4B | IMM_CONSTANT},
    {I_LDX1B,{0x49, 0x8B, 0x50}, 4, "mov\trdx,QWORD PTR [r8+0x%X]", IMM_1B | RLYDISP},
    {I_LDX4B, {0x49, 0x8B, 0x90}, 7, "mov\trdx,QWORD PTR [r8+0x%X]", IMM_4B | RLYDISP},
};

static void setup_x86_dispatch() {
    /* guaranteed zeros in static array */
    int i = 0;
    for (X86Pattern& e : KnownPatterns) {
        if (i != 0) {
            auto b = e.Data[0];
            e.next = dispatch[b];
            dispatch[b] = (unsigned char)i;
        }
        i++;
    }
}

static struct X86DisRV DisassembleDecodedX86(unsigned char* ip, uint64_t Pctr, const X86Pattern& E) {
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
    static bool initted = false;
    if (!initted) {
        setup_x86_dispatch();
        initted = true;
    }
    assert(nitems > 0);

    for (auto d = dispatch[*ip]; d != 0; d = KnownPatterns[d].next) {
        const X86Pattern& E = KnownPatterns[d];
        if (E.match(ip, nitems))
            return DisassembleDecodedX86(ip, Pctr, E);
    }
    struct X86DisRV RV;
    RV.byte_count = std::min((unsigned)nitems, (unsigned)3);
    RV.disassembly = FmtInst(ip, RV.byte_count) + "Unknown";
    return RV;

}

#define NSTACK 10
#define STACKLAST (NSTACK-1)

uint64_t SimulateX86(void* linkptr, void* codeptr) {
    uint64_t RAX=0, RCX=0, RDX=0, RSI=0, RDI=0, R8=0, STACK[NSTACK];
    uint64_t *SP = STACK + STACKLAST;
    bool ZI = false;
    
    unsigned char * pc = (unsigned char*)codeptr;
    /* assume mac calling sequence */
    RDI = (uint64_t)linkptr;
    RCX = (uint64_t)codeptr;
    
    *SP-- = 0;
    
    while (SP < STACK+STACKLAST) {
        X86Pattern * Ep = nullptr;
        for (auto d = dispatch[*pc]; d != 0; d = KnownPatterns[d].next) {
            Ep = &(KnownPatterns[d]);
            if (Ep->match(pc, 1LL<<61))
                break;
        }
        assert(Ep != nullptr); // Can't decode
        
        pc += Ep->must_exist_bytes;
        
        switch(Ep->icode) {
            case I_NULL:
                assert(!"Null icode in interpreter");
                continue;
            case I_RET:
                assert(SP < STACK+STACKLAST);
                pc = (unsigned char*)(*++SP);
                continue;
            case I_CRSI:
                *SP-- = (uint64_t)pc;
                pc = (unsigned char*)RSI;
                continue;
            case I_CRDX:
                *SP-- = (uint64_t)pc;
                pc = (unsigned char*)RDX;
                continue;
            case I_ANDAL:
                RAX &= 0xFFFFFFFFFFFFFF00;
                ZI = true;
                continue;
            case I_XORAL:
                RAX = RAX ^ 1;
                ZI = (RAX == 0);
                continue;
            case I_ORAL:
                RAX |= 1;
                ZI = false;
                continue;
            case I_TEST: //test    BYTE PTR [rdx],cl
                ZI = ((*(unsigned char*)RDX) & (unsigned char)RCX) == 0;
                continue;
            case I_LMOV:
                RAX = (*(unsigned char*)RDX);
                continue;
            case I_M8CX:
                R8 = RCX;
                continue;
            case I_M8DI:
                R8 = RDI;
                continue;
            case I_MVZXA:
                RAX &= 0xFFFFFFFFFFFF00;
                continue;
            case I_SETNZ:
                ZI = (RAX & 0xFF) == 0;
                continue;

            case I_JZ:
                if (ZI)
                    pc += (int64_t)(char)(pc[-1]);
                continue;
            case I_JNZ:
                if (!ZI)
                    pc += (int64_t)(char)(pc[-1]);
                continue;
            case I_JMPS:
                pc += (int64_t)(char)(pc[-1]);
                continue;
            case I_JMPL:
                pc += (int64_t)collect_32(pc - 4);
                continue;
                
            case I_C2CX:
                RCX = collect_32(pc - 4);
                continue;
            case I_LDX1B:
                RDX = *(((uint64_t*)R8) + (int64_t)(char)(pc[-1]));
                continue;
            case I_LDX4B:
                RDX = *(((uint64_t*)R8) + (int64_t)collect_32(pc - 4));
                continue;
        }
    }
    return RAX;
}
       



