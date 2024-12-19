#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include <sys/stat.h>
#include <cassert>

#include "tkov2.h"
#include "RCArm64.h"
#include "relays.h"

#include <string>
#include <vector>

#include <sys/mman.h>


using std::vector, std::string;

void ReadFaslForms(unsigned char *);
ArmInst*CodeText = nullptr;
static const char * RnamesTexts = nullptr;
static const short * RnamesTextPtrs = nullptr;
vector<string>FASLAtsyms;
static vector<string>RTypeNames;
vector<Relay*>ESD;
char** Compiled_Linkage_Sptr; //points to State cells.
void ReadFaslForms(unsigned char * data);

void LoadRelayObjectFile(const char*path, const char*) {
    RnamesTexts = nullptr;
    RnamesTextPtrs = nullptr;
    FASLAtsyms.clear();
    RTypeNames.clear();
    ESD.clear();

    struct stat st;
    size_t file_length = 0;
    if (stat(path, &st) == 0) {
        file_length = st.st_size;
    }
    else {
        fprintf(stderr, "Can't get length of file: %s\n", path);
        exit(3);
    }
    vector<Relay*> ISD;

    vector<unsigned char> FileCtnr(file_length);
    auto const file_data = FileCtnr.data();
    auto dp = file_data;
    auto f = fopen (path, "rb");
    if (f == nullptr) {
        fprintf(stderr, "Can't open file: %s\n", path);
        exit(4);
    }
    fread (file_data, 1, file_length, f);
    fclose(f);

    auto hp = (_TKO_VERSION_2_HEADER*) dp;
    dp += hp->header_size;
    while(dp < file_data + file_length) {
        auto chp = (_TKO_VERSION_2_COMPONENT_HEADER*) dp;
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
                size_t code_bytes = chp->number_of_items * sizeof(ArmInst);
#if NXSYSMac
                CodeText = (ArmInst*)mmap(NULL,
                                          code_bytes,
                                          PROT_WRITE | PROT_READ | PROT_EXEC,
                                          MAP_ANON | MAP_PRIVATE | MAP_JIT,
                                          -1, 0
                                          );

                if (__builtin_available(macOS 11.0, *)) {
                    pthread_jit_write_protect_np(0);
                } else {
                    // Fallback on earlier versions
                } // Turn off so it is RW- (Apple only)
                memcpy(CodeText, rdp, chp->length_of_block);
                if (__builtin_available(macOS 11.0, *)) {
                    pthread_jit_write_protect_np(1);
                } else {
                    // Fallback on earlier versions
                } // Turn on so it is R-X (Apple only)
#endif
                break;
            }
            case TKOI_ISD:
            {
                auto dtbp = (TKO_DEFBLOCK*)rdp;
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
                auto dtbp = (TKO_DEFBLOCK*)rdp;
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
                ReadFaslForms((unsigned char *)rdp);
                break;
            default:
                break;
        }

        dp = next_block;
    }
    
}




void CleanupObjectMemory() {
    /* dum vivimus speramus */
}
