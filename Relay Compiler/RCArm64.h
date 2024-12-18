//
//  RCArm64.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 12/17/24.
//  Copyright © 2024 BernardGreenberg. All rights reserved.
//

#pragma once
/* Source of all knowledge -
 https://developer.arm.com/documentation/ddi0602/2024-09/Base-Instructions?lang=en
 */
#include <cstdint>

enum ARM {
    ret      = 0xD65F0000,
    movz_0   = 0xD2400000,
    movz_1   = 0xD2400020,
    b        = 0x14000000,
    tbz      = 0x36000000,
    tbnz     = 0x37000000,
    ldr_storage = 0x58000000,
    ldrb_reg = 0x39400000,
    eor_imm  = 0x52010000,
    mov_rr   = 0xAA0003E2
};

using ArmInst = uint32_t;
#define TOPBYTE(u32) ((u32 >> 24) &0xFF)

