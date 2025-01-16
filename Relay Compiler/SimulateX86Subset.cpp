//
//  SimulateX86Subset.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 1/15/25.
//  Copyright © 2025 BernardGreenberg. All rights reserved.
//

#include <cassert>
#include "NXX86.h"
#include "cccint.h"

namespace  NXX86 {

/* This stoopid interpreter, which actually works, is here in order to execute X86 instructions
 on either Mac.  Intel Macs don't need it, but following a certain bug, they did, once.
 Currently it allows ARM Macs to emulate Intel. Note that Win64 is Intel, too. */

#define NSTACK 10
#define STACKLASTNO (NSTACK-1)
#define STACKLAST STACK+STACKLASTNO

int32_t SimulateX86(void* codeptr) {

    /* Simulated X866_64 registers and zero indicator */

    uint64_t RAX=0, RCX=0, RDX=0, RSI=0, RDI=0, R8=0, STACK[NSTACK];
    uint64_t *SP = STACKLAST;
    bool ZI = false;

    EnsureDispatch();  //Get the dispatch array filled, if it's not done yet.

    /* bypass Grand Thunk Railroad */
    /* must take care to do address arithmetic in bytes (unsigned chars)*/

    unsigned char * pc = (unsigned char*)codeptr;
    R8 = (uint64_t)Compiled_Linkage_Sptr;
    RCX = 1;

    *SP-- = 0; /* set a trap for a RET out of the whole function to be recognized */
    
    while (SP < STACKLAST) {
        X86Pattern * Ep = nullptr;
        for (auto d = DispatchArray[*pc]; d != 0; d = KnownPatterns[d].next) {
            auto tEp = &(KnownPatterns[d]);
            if (tEp->match(pc, 1LL<<61)) {
                Ep = tEp;
                break;
            }
        }
        assert(Ep != nullptr); // Can't decode

        /* uncomment to watch the interpreter in action */
        //  printf("%p %s\n", pc, Ep->disassembly);

        /* Advance the PC FIRST, so that rel refs are meaningful*/

        pc += Ep->must_exist_bytes;
        
        switch(Ep->icode) {
            case I_NULL:
                assert(!"Null icode in interpreter");
                continue;
            case I_RET:
                assert(SP < STACKLAST);
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
            case I_TEST:
                ZI = ((*(unsigned char*)RDX) & (unsigned char)RCX) == 0;
                continue;
            case I_LMOV:
                RAX = *(unsigned char*)RDX;
                continue;
            case I_M8CX:
                R8 = RCX;
                continue;
            case I_M8DI:
                R8 = RDI;
                continue;
            case I_MVZXA:
                RAX &= 0xFF;
                continue;
            case I_SETNZ:
                RAX = ZI ? 0 : 1;
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
                RDX = *(uint64_t*)((unsigned char*)R8 + (int64_t)(char)(pc[-1]));
                continue;
            case I_LDX4B:
                RDX = *(uint64_t*)(((unsigned char*)R8) + (int64_t)collect_32(pc - 4));
                continue;
        }
    }
    /* Since we dispensed with the thunk, convert the (non)-zero flag into AL 0 or 1 here in C++. */
    RAX = ZI ? 0 : 1;
    return (int32_t) RAX;
}
       
} //namespace NXX86
