//
//  DisasUtil.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 12/26/24.
//  Copyright © 2024 BernardGreenberg. All rights reserved.
//

#ifndef DisasUtil_h
#define DisasUtil_h

#include <string>

typedef uint64_t LPCTR;   /* L for "live", i.e. not compiler/disassembler */
int32_t extract_bits(uint32_t data, int high_bit_no, int low_bit_no);
/* until something more general */
std::string DisassembleARM(uint32_t instruction, LPCTR pctr);

struct X86DisRV {
    std::string disassembly;
    uint32_t byte_count;
    uint32_t relay_ref_index;
    bool have_ref_relay = false;
};

struct X86DisRV DisassembleX86(unsigned char * base, uint64_t start_pctr, uint64_t limit);

uint64_t SimulateX86(void* linkptr, void* codeptr);

#endif /* DisasUtil_h */
