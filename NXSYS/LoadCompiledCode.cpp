
//
//  LoadCompiledCode.cpp
//  formerly CompiledCodeInterface.cpp
//  NXSYSMac (hope for Windows, too)
//
//  Created by Bernard Greenberg December 2024
//  Copyright © 2024 BernardGreenberg. All rights reserved.
//
//  Inspired by 1994 LoadOb32, but not copied from it, much more modern.
//  This compiles and works on an M3 Mac (_M_ARM64_) and Intel Mac now.
//


#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include <sys/stat.h>
#include <cassert>

#include "tkov3.h"
#include "RCArm64.h"
#include "relays.h"
#include "usermsg.h"
#include "cccint.h"
#include "MessageBox.h"
#include "CompiledReportInfo.h"

#include <string>
#include <vector>
#include <unordered_map>
using std::vector, std::string, std::unordered_map;

#ifdef NXSYSMac
#include <sys/mman.h>
#endif
#define MINIMUM_RELAY_COMPILER_VERSION 3

ArmInst*CodeText = nullptr;
static size_t CodeSize = 0;

static const char * RnamesTexts = nullptr;
static const short * RnamesTextPtrs = nullptr;
static vector<string>RTypeNames;
static unordered_map<Relay*, unsigned int>RelayFunctionLengths;

vector<string>FASLAtsyms;  //referenced by FASL decoder
vector<Relay*>ESD;

static bool CRPpresent = false;
static CompiledReportInfo TheCompiledReportInfo;

char** Compiled_Linkage_Sptr = nullptr;; //points to State cells.  Referenced by relay engine
#if WIN32 | !((defined(__aarch64__)) || defined(_M_ARM64))
CCC_Thunkptrtype CCC_Thunkptr;
#endif


bool RunningSimulatedCompiledCode = false;
void ReadFaslForms(unsigned char * data, const char* fname);
void CleanupObjectMemory(); //below

#if NXSYSMac

#include <sys/types.h>
#include <sys/sysctl.h>

int processIsTranslated() { // = "are we running under Rosetta?"
   int ret = 0;
   size_t size = sizeof(ret);
   if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) == -1)
   {
      if (errno == ENOENT)
         return 0;
      return -1;
   }
   return ret;
}

static void MacAllocCodeText(void* data, size_t count) {
    unsigned int map_flags = MAP_ANON | MAP_PRIVATE;
    unsigned int prot_flags = PROT_READ | PROT_WRITE;

    if (!RunningSimulatedCompiledCode) {
        map_flags |= MAP_JIT; // We will do the "JIT" jusu---
        prot_flags |= PROT_EXEC;
    }

    CodeText = (ArmInst*)mmap(NULL,
                              count,
                              prot_flags,
                              map_flags,
                              -1, 0
                              );
    assert(CodeText != nullptr);
    CodeSize = count;
    
    if (!RunningSimulatedCompiledCode)
        if (__builtin_available(macOS 11.0, *))
            pthread_jit_write_protect_np(0);

    memcpy(CodeText, data, count);
    
    if (!RunningSimulatedCompiledCode)
        if (__builtin_available(macOS 11.0, *))
            pthread_jit_write_protect_np(1);
    
}

#endif

#if WIN32
static void WinAllocCodeText(void* code, size_t length) {
    CodeText = (ArmInst*)VirtualAlloc(NULL, length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    assert(CodeText != NULL);

// Copy code to new memory.
    memcpy(CodeText, code, length);

// Change protection.
// We remove the write flag and set the execute flag on that page.
    DWORD oldProtection;
    BOOL pastafazool = VirtualProtect(CodeText, length, PAGE_EXECUTE, &oldProtection);
    assert(pastafazool);
}
#endif

static bool verify_header_ids(const _TKO_VERSION_3_HEADER& H, const char * path) {
    if ((H.magic != TKO_VERSION_3_MAGIC) ||
        !! memcmp(TKO_VERSION_3_STRING, &H.magic_string, strlen(TKO_VERSION_3_STRING)+1)
        || H.version != TKO_VERSION_3) {
        usermsgstop("%s is not a version 3 NXSYS Relay Compiler output file.", path);
        return false;
    }
    if (H.compiler_version < MINIMUM_RELAY_COMPILER_VERSION ) {
        usermsgstop("%s was not produced by version %d or better of the Relay Compiler.", path,
                    MINIMUM_RELAY_COMPILER_VERSION);
        return false;
    }
    if (H.bits != 64 ) {
        usermsgstop("%s was compiled for a %d bit environment, not 64 as required.", path, H.bits);
        return false;
    }
#if defined(__aarch64__) || defined(_M_ARM64)
    if (string(H.arch) =="INTEL x86") {
#if 0  // aimply assume simulation works
        int response = MessageBox(nullptr,
                                  "This interlocking definition was compiled for the Intel X86, but this Mac is "
                                  "running on an ARM64 processor. We can run this interlocking code "
                                  "in simulation; there should be no difference. Do you want to?",
                                  "Compiled file for wrong CPU type",
                                  MB_YESNOCANCEL);
        if (response != IDYES)
            return false;
#endif
        RunningSimulatedCompiledCode = true;
    }
    else if (string (H.arch) != "ARM64") {
        usermsgstop("%s was not compiled for the ARM64 Apple Silicon architecture, but for %s.",
                    path, H.arch);
        return false;
    }
#else
    if ((string (H.arch) != "INTEL x86") || H.bits != 64) {
        usermsgstop("%s was not compiled for this Intel CPU, but for %s.",
                    path, H.arch);
        return false;
    }
#if NXSYSMac
    // https://forums.developer.apple.com/forums/thread/659846
    if (processIsTranslated() == 1) {  // If Rosetta2ing, "this is fine."
        RunningSimulatedCompiledCode = false;
        return true;
    }
#endif
    RunningSimulatedCompiledCode = false;
#if 0  //this was useful once;  Comment it out.  A bug at TKDI_TXT: (first line) caused this
      //problem.
    auto msg = "This interlocking definition  was compiled for the Intel X86, but Intel Macs " \
    "have a problem running app-generated code. We can run this interlocking " \
    "code in simulation; there should be no difference. Do you want to? " \
    "Intel Mac JIT problem requires simulation.  \"Yes\" will simulate, " \
    "\"No\" will take the JIT path (probably crash), \"Cancel\" will do so.";
    
    int response = MessageBox(nullptr, msg, "Compiled Interlocking JIT issue", MB_YESNOCANCEL);
    if (response == IDCANCEL)
        return false;
    if (response == IDYES)
        RunningSimulatedCompiledCode = true;
    else
        RunningSimulatedCompiledCode = false;
#endif

#endif
        
    return true;
}
#if !(defined(__aarch64__) || defined(_M_ARM64))
static int FindThunkIndex(const vector<Relay*>& ISD, const char * s) {
    Sexpr Rsym = intern_rlysym_nocreate(0, s);
    if (Rsym != NIL) {
        for (int i = 0; i < (int)ISD.size(); i++)
            if (ISD[i]->RelaySym == Rsym)
                return i;
    }
    assert(!"Cant find thunk"); // Can't find symbol
    return -1;
}
#endif

bool LoadRelayObjectFile(const char*path, const char*) {
    CleanupObjectMemory();

    struct stat st;
    size_t file_length = 0;
    if (stat(path, &st) == 0)
        file_length = st.st_size;
    else {
        usermsgstop ("Can't get length of track object file: %s", path);
        return false;
    }


    vector<unsigned char> FileCtnr(file_length);
    auto const file_data = FileCtnr.data();
    auto dp = file_data;
    auto f = fopen (path, "rb");
    if (f == nullptr) {
        usermsgstop("Can't open track object file: %s", path);
        return false;
    }
    fread (file_data, 1, file_length, f);
    fclose(f);

    RunningSimulatedCompiledCode = false; // until we know we are
    auto hp = (_TKO_VERSION_3_HEADER*) dp;
    dp += hp->header_size;
    if (!verify_header_ids(*hp, path)) //diagnoses extensively
        return false;
    vector<Relay*> ISD;

    while(dp < file_data + file_length) {
        auto chp = (_TKO_VERSION_3_COMPONENT_HEADER*) dp;
        auto rdp = dp + sizeof(*chp);
        auto next_block = rdp + chp->length_of_block;
        switch(chp->compid) {
            case TKOI_CID:
                break;
            case TKOI_RTT:
                RnamesTexts = (const char *)rdp;
                break;
            case TKOI_RTD:
                {
                    auto ptrs = (const unsigned short*)rdp;
                    for (int i = 0; i < chp->number_of_items;i++)
                        RTypeNames.push_back(RnamesTexts + ptrs[i]);
                }
                break;
            case TKOI_TXT:
            {
                size_t code_bytes = chp->number_of_items * chp->length_of_item;
#if NXSYSMac
                MacAllocCodeText(rdp, code_bytes);
#else
                WinAllocCodeText(rdp, code_bytes);
#endif
                break;
            }
            case TKOI_ISD:
            {
                auto dtbp = (TKO_DEFBLOCK_3*)rdp;
                for (int i = 0; i < chp->number_of_items; i++, dtbp++){
                    string rlytype = RTypeNames[dtbp->type];
                    Sexpr rlysym = intern_rlysym(dtbp->n, rlytype.c_str());
                    Relay* relay = CreateReportingRelay(rlysym);  //"never know if reporting is needed"
                    relay->exp = (LNode*)((unsigned char*)CodeText + dtbp->data);
                    relay->Flags |= LF_CCExp;
                    ISD.push_back(relay);
                }
            }
                break;
            case TKOI_ESD:
            {
                assert(sizeof(char*) == 8);
                //This is the new "pointer" way...v3.
                Compiled_Linkage_Sptr = new char*[chp->number_of_items];
                auto dtbp = (TKO_DEFBLOCK_3*)rdp;
                for (int i = 0; i < chp->number_of_items; i++, dtbp++){
                    string rlytype = RTypeNames[dtbp->type];
                    Sexpr rlysym = intern_rlysym(dtbp->n, rlytype.c_str());
                    Relay* relay = CreateReportingRelay(rlysym);  //"never know if reporting is needed"
                    ESD.push_back(relay);
                    auto cellptr = Compiled_Linkage_Sptr + i; // rationed in pointers
                    *cellptr = &(relay->State);
                }
            }
                  break;
            case TKOI_ATS:
            {
                auto p = (const char*)rdp;
                while (p < (const char *)next_block) {
                    size_t len = (unsigned char)(*p++);
                    FASLAtsyms.emplace_back(p, p+len);
                    p += len;
                }
            }
                break;
            case TKOI_TMR:
            {
                auto tbp = (TKO_TIMER_DEF*) rdp;
                for (int i = 0; i < chp->number_of_items; i++, tbp++)
                /* the compiled relay, value referenced by code, is the
                "out(pu)ter".  Its exp, however, is the code for the "controller". */
                ISD[tbp->rlyisdid] = DefineTimerRelayFromObject
                           (ISD[tbp->rlyisdid], tbp->time);
            }
                break;
            case TKOI_DPD:
                 {
                    auto p = (char unsigned*)rdp;
                    for (int i = 0; i < chp->number_of_items;i++) {
                        auto ddp = (TKO_DPTE_HEADER*) p;
                        Relay* affector = ESD[ddp->affector];
                        p += sizeof(*ddp);
                        int* affp = (int*)p;
                        for (int j = 0; j < ddp->count; j++) {
                            Relay* affected = ISD[affp[j]];
                            affector->Dependents.push_back(affected);
                            p += sizeof(uint32_t);
                        }
                    }
                }
                break;
            case TKOI_FRM:
                ReadFaslForms((unsigned char *)rdp, path);
                break;
            default:
                break;
        }

        dp = next_block;
    }
    
    for (auto i = 0; i < ISD.size(); i++) {
        Relay* r = ISD[i];
        uint64_t next = (i == (ISD.size() - 1)) ? (uint64_t)((unsigned char*)CodeText + CodeSize) : (uint64_t)(ISD[i+1]->exp);
        int len = (int)(next - (uint64_t)(r->exp));
        RelayFunctionLengths[r] = len;
    }
#if WIN32
    int winthunk_index = FindThunkIndex(ISD, "_WINDOWS_ENTRY_THUNK");
    CCC_Thunkptr = (CCC_Thunkptrtype)(ISD[winthunk_index]->exp);
#elif !(((defined(__aarch64__)) || defined(_M_ARM64)))
    int macthunk_index = FindThunkIndex(ISD, "_MACOS_ENTRY_THUNK");
    CCC_Thunkptr = (CCC_Thunkptrtype)(ISD[macthunk_index]->exp);
#endif
    /* If ARM, there is no thunk and no thunkptr */

    RnamesTexts = nullptr; //points into vector
    RnamesTextPtrs = nullptr; //ditto
    
    auto& T = TheCompiledReportInfo;
    T.HeaderVersion = hp->version;
    T.Architecture = hp->arch;
    T.CompilationTime = hp->time;
    T.CompilerVersion = hp->compiler_version;
    T.CodeLen = hp->code_len;
    T.StaticLen = hp->static_len;
    T.Bits = hp->bits;
    T.User = hp->user;
    T.ISDCount = (int)ISD.size();
    T.ESDCount = (int)ESD.size();
    CRPpresent = true;
    
    return true;
}

const CompiledReportInfo* GetCompiledReportInfo() {
    if (CRPpresent)
        return &TheCompiledReportInfo;
    else
        return nullptr;
}

int GetRelayFunctionLength(Relay* r) {
    if(!RelayFunctionLengths.count(r))
        return -1;
    return RelayFunctionLengths[r];
}

void CleanupObjectMemory() {
    RnamesTexts = nullptr;
    RnamesTextPtrs = nullptr;
        FASLAtsyms.clear();
    RTypeNames.clear();
    RelayFunctionLengths.clear();
    ESD.clear();
    /* dum vivimus speramus */
    if (CodeText != nullptr) {
#if NXSYSMac    
        munmap(CodeText, CodeSize);
#else
        BOOL didit = VirtualFree(CodeText, 0, MEM_RELEASE);
        assert(didit);
#endif
    }
#if WIN32 | !((defined(__aarch64__)) || defined(_M_ARM64))
    CCC_Thunkptr = nullptr;
#endif
    CodeText = nullptr;
    CodeSize = 0;
    if (Compiled_Linkage_Sptr)
        delete Compiled_Linkage_Sptr;
    Compiled_Linkage_Sptr = nullptr;
    CRPpresent = false;
}
