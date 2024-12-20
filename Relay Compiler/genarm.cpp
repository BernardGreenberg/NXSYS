//
//  genarm.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 12/20/24.
//  Copyright © 2024 BernardGreenberg. All rights reserved.
//

#include <stdio.h>
#include <cassert>
#include <string>

using std::string;

#include "RCArm64.h"
#include "rcdcls.h"
#include "opsintel.h" //needed for MOP_foo definitions

//void CHECK();
#define CHECK()
extern int Ahex;
extern PCTR Pctr;
void outbytes_raw (const char * mnemonic, const unsigned char * bytes, int count, const char* comment);

void outarminst (ArmInst w, const char * mnem, const char* str) {
    auto p = (unsigned char *)&w;
    outbytes_raw (mnem, p, 4, str);
}

/* Yer bits are number 35 to 0 left-to-right, as in https://developer.arm.com/documentation/ddi0602/2024-09/Base-Instructions?lang=en
 */
ArmInst insert_arm_bitfield(ArmInst inst, int displacement, int start_bit, int end_bit, int shift_down) {
    displacement >>= shift_down;
    int fld_len = start_bit - end_bit + 1;
    int mask = (1 << fld_len)-1;
    displacement &= mask;
    displacement <<=  end_bit;
   // assert ((inst & mask) == 0);
    inst &= ~mask;  //override with later one.
    return inst | displacement;
}

bool verify_arm_bitfield_zero(ArmInst inst, int start_bit, int end_bit, int shift_down, PCTR target){
    int fld_len = start_bit - end_bit + 1;
    int mask = (1 << fld_len)-1;
    mask <<=  end_bit;
    if (inst & mask) {
        int unsigned val = (inst & mask) >> end_bit;
        int unsigned rval = val << shift_down;
        RC_error(0, "\nBUG *****: Arm instruction field %d-%d (len %d) nonzero before insert,"
                 " currently 0x%X, shifted up (by %d) = 0x%X. Pctr = 0x%06X, target location %06X, full inst as is %08x\n",
                 start_bit, end_bit, fld_len, val, shift_down, rval, Pctr, target, inst);
        return false;
    }
    return true;
}

static string pfmt(unsigned int x, const char* fmt) {
    char buf[24];
    snprintf(buf, sizeof(buf), fmt, x);
    return string(buf);
}

void outinst_raw_arm (MACH_OP op, const char * str, PCTR opd) {
    CHECK();
    ArmInst inst;
    switch (op) {
        case MOP_RET:
            inst = insert_arm_bitfield(ARM::ret, ARM::bl_return_reg, 9, 5, 0);
            outarminst(inst, "ret", "x30");
            return;
            
        case MOP_CLZ:
            outarminst(ARM::movz_0, "movz", "x0, #0");
            return;
            
        case MOP_STZ:
            /* means indicate low logic level in x0*/
            outarminst(ARM::movz_1, "movz", "x0, #1");
            return;
            
        case MOP_JMPL:  /* "In arm64, there is neither long nor short, east nor west ..." */
        case MOP_JMP:
            RC_error(1, "Attempt to generate uncond jump in Arm compilation.");
            outarminst(ARM::b, "b", str);
            return;
            
        case MOP_JNZ:
            inst = insert_arm_bitfield(ARM::tbnz, opd - Pctr, 18, 5, 2);
            outarminst(inst, "tbnz", (string("x0, #0, ") + str).c_str());
            return;
            
        case MOP_JZ:
            inst = insert_arm_bitfield(ARM::tbz, opd - Pctr, 18, 5, 2);
            outarminst(inst, "tbz", (string("x0, #0, ") + str).c_str());
            return;
            
        case MOP_LDAL:  /* what luck! */
        case MOP_TST:
        {
            string operand = string("x0, [x2, #0x") + pfmt(opd, "%04X") + "]   ; v$" + str;
            inst = insert_arm_bitfield(ARM::ldr_storage, opd, 20, 10, 3);
            outarminst(inst, "ldr", operand.c_str());
            outarminst(ARM::ldrb_reg, "ldrb", "x0, [x0, #0]");
            return;
        }
        case MOP_XOR:
            outarminst(ARM::eor_imm, "eor", "x0, x0, #1"); //...Balthasar
            return;

        default:
            RC_error(1, "Unknown internal op code for arm64: %d\n", op);
            return;
    }
}

void OutputARMFunctionPrologue() {
    outarminst(ARM::mov_rr, "mov", "x2, x0          ;linkage"); /* move single arg (linkage) to x2 */
}

void ARM64FixupFixup(Fixup &F, PCTR pc) {
    int d = pc - F.pc;
    list (";  ARM Fixup @%0*X to %s = %0*X, disp %04X sum %0*X\n",
          Ahex, F.pc, F.tag->lab, Ahex, pc, d, Ahex, F.pc + d);
    unsigned int* iptr = ((unsigned int*) &(Code[F.pc]));
    unsigned int inst = *iptr;
    char unsigned opcode = TOPBYTE(inst);
    assert(opcode == 0x36 || opcode == 0x37);
    CHECK();
    verify_arm_bitfield_zero(inst, 18, 5, 2, F.pc);
    inst = insert_arm_bitfield(inst, d, 18, 5, 2);
    *iptr = inst;
    CHECK();
}

