//
//  CompiledReportInfo.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 1/11/25.
//  Copyright © 2025 BernardGreenberg. All rights reserved.
//

#ifndef CompiledReportInfo_h
#define CompiledReportInfo_h


#include <string>
#include <time.h>

typedef struct {
    int HeaderVersion;
    std::string Architecture;
    std::string Compiler;
    time_t CompilationTime;
    int CompilerVersion;
    unsigned int CodeLen;
    unsigned int StaticLen;
    int Bits;
    std::string User;
    int ISDCount;
    int ESDCount;
} CompiledReportInfo;

const CompiledReportInfo* GetCompiledReportInfo();

#endif /* CompiledReportInfo_h */
