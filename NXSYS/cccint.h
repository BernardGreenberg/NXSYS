#ifndef _NXSYS_CCC_INTERFACE_H__
#define _NXSYS_CCC_INTERFACE_H__

extern char** Compiled_Linkage_Sptr;

#ifdef WIN32

typedef int (WINAPI *CCC_Thunkptrtype) (void* linkage_base, void* code_addr);

extern CCC_Thunkptrtype CCC_Thunkptr;
#define CallCompiledCode(exp) (*CCC_Thunkptr) (Compiled_Linkage_Sptr, (void*)exp)

#elif NXSYSMac && (defined(__aarch64__) || defined(_M_ARM64))

typedef  unsigned char(CompiledFunction)(void*);

#define CallCompiledCode(codeptr) (reinterpret_cast<CompiledFunction*>(codeptr))(Compiled_Linkage_Sptr)

#else

typedef int (*CCC_Thunkptrtype) (void* linkage_base, void* code_addr);

extern CCC_Thunkptrtype CCC_Thunkptr;

#define CallCompiledCode(codeptr)  (*CCC_Thunkptr)(Compiled_Linkage_Sptr, codeptr)

#endif

extern bool RunningSimulatedCompiledCode;

namespace NXX86 {
int32_t SimulateX86(void* codeptr);
};




#endif
