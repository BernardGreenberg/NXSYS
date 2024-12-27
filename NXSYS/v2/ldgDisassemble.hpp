//
//  ldgDisassemble.hpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 12/25/24.
//  Copyright © 2024 BernardGreenberg. All rights reserved.
//

#ifndef ldgDisassemble_hpp
#define ldgDisassemble_hpp

#include <stdio.h>

class Relay;
extern bool haveDisassembly;
void InitLdgDisassembly();
void ldgDisassemble(Relay* r);
void ldgDisassembleDraw(HDC dc);

int GetRelayFunctionLength(Relay* r);
#endif /* ldgDisassemble_hpp */
