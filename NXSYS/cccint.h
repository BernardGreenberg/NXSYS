#ifndef _NXSYS_CCC_INTERFACE_H__
#define _NXSYS_CCC_INTERFACE_H__

extern void* Compiled_Linkage_Sptr;

#ifdef WIN32

typedef int (WINAPI *CCC_Thunkptrtype) (void* linkage_base, void* code_addr);

extern CCC_Thunkptrtype CCC_Thunkptr;
#define CallCompiledCode (*CCC_Thunkptr)

#else
#if NXSYSMac

typedef  unsigned char(CompiledFunction)(void*);

#define CallCompiledCode(linkptr,codeaddr) (reinterpret_cast<CompiledFunction*>(codeaddr))(linkptr)

#endif

//extern "C" {int CallCompiledCode (void* linkage_base, void* code_addr);};

#endif

#endif
