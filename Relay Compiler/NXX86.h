//
//  NXX86.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 1/15/25.
//  Copyright © 2025 BernardGreenberg. All rights reserved.
//

#ifndef NXX86_h
#define NXX86_h

#include <vector>
#include <string>

#include <stdio.h>

namespace NXX86 {

uint32_t collect_32(unsigned char* p); //collect low-endian-stored 32-bit number
std::string FmtInst(unsigned char * ip, int nbytes);
void EnsureDispatch();
extern unsigned char dispatch[256];

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
    std::vector<unsigned char> Data;  /* Bytes that must match exectly */
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

extern struct X86Pattern KnownPatterns[];

} // namespace NXX86

#endif /* NXX96_h */
