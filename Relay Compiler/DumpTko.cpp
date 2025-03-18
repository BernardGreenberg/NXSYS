//
//  DumpTko.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 12/12/24.
//  Copyright © 2024 BernardGreenberg. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "tkov2.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <time.h>

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include "argparse.hpp"
#include "STLExtensions.h"

using std::string, std::unordered_set, std::vector;

const char * TKOI_STRINGS[] = TKOV2_COMPID_STRINGS;

void DumpText(_TKO_VERSION_2_COMPONENT_HEADER* chp, unsigned char* fdp, size_t flen);

class State {
    
public:
    const short * RnamesTextPtrs = nullptr;
    const char * RnamesTexts = nullptr;
    vector<string> maybe_dump_relay_list(void* vrdp, int nitems, bool print) {
        vector<string>D;
        auto dbp = (const TKO_DEFBLOCK*) vrdp;
        for (int i = 0; i < nitems; i++) {
            auto tdbp = dbp+i;
            D.emplace_back(std::to_string(tdbp->n) + (RnamesTexts+RnamesTextPtrs[tdbp->type]));
        }
        if (print) {
            for (int i = 0; i < nitems; i++) {
                auto tdbp = dbp+i;
                printf("%3d %-8s 0x%04X\n", i, D[i].c_str(), tdbp->data);
            }
        }
        return D;
    }
};

class longtime {
    std::string s;
public:
    time_t T;
    const char* cs;
    longtime(long L) {
        T = L;
        s = ctime(&T);
        cs = s.c_str();
    }
};

int main (int argc, const char ** argv) {
    argparse::ArgSet Aset ("DumpTko track object dumper",
                           {
        {"file", "help=Pathname of .tko file to be analyzed."},
        {"-H", "--header", "help=Print header (if explicit sections given)"},
        {"-v", "--verbose", "boolean=", "help=Dump detail for everything"},
        {"sections", "nargs=*", "help=Dump detail for only these sections (lowercase ok)"}});
    
    auto args = Aset.Parse(argc, argv);
    unordered_set<string> sections_wanted, sections_found;
    for (const auto& section : args.VectorArgs["sections"]) {
        sections_wanted.insert(stoupper(section));
    };
    bool summary = sections_wanted.size() == 0;
    bool verbose = args["verbose"];
    const char * path = args["file"];
    struct stat st;

    size_t file_length = 0;
    if (stat(path, &st) == 0) {
        file_length = st.st_size;
    }
    else {
        fprintf(stderr, "Can't get length of file: %s\n", path);
        exit(3);
    }
    
    State S;
    vector<string> ESD;
    vector<string> ISD;

    auto const file_data = new unsigned char[file_length];
    auto dp = file_data;
    auto f = fopen (path, "rb");
    if (f == nullptr) {
        fprintf(stderr, "Can't open file: %s\n", path);
        exit(4);
    }
    assert(f != NULL);
    fread (file_data, 1, file_length, f);
    fclose(f);

    auto hp = (_TKO_VERSION_2_HEADER*) dp;    
    if ((hp->magic != TKO_VERSION_2_MAGIC) ||
        !! memcmp(TKO_VERSION_2_STRING, &(hp->magic_string), strlen(TKO_VERSION_2_STRING)+1)
        || hp->version != TKO_VERSION_2) {
        fprintf(stderr, "%s is not a version 2 NXSYS Relay Compiler output file.\n", path);
        exit(4);
    }
    printf("File %s, len %zd=0x%zX\n", path, file_length, file_length);

    if (summary || args["header"]) {
        printf("Header\n");
        printf("  Magic %4X\n", hp->magic);
        printf("  Magic string \"%s\"\n", hp->magic_string);
        printf("  Version %d\n", hp->version);
        printf("  Header size %d = 0x%X\n", hp->header_size, hp->header_size);
        printf("  Compat code len %d\n", hp->compat_code_len);
        printf("  Compat static len %d\n", hp->compat_code_len);
        printf("  Compilation time %s", longtime(hp->time).cs);
        printf("  User \"%s\"\n", hp->user);
        printf("  Arch \"%s\"\n", hp->arch);
        printf("  Bits %d\n", hp->bits);
        printf("  Archindex %d\n", hp->archindex);
        printf("  Code len %d = 0x%x\n", hp->code_len, hp->code_len);
        printf("  Static len %d = 0x%X\n", hp->static_len, hp->static_len);
        printf("  Compiler Version %d.\n\n", hp->compiler_version);
    }
    else {
        string sctime(longtime(hp->time).cs);
        if (sctime.back() == '\n') sctime.pop_back();
        printf("Compiled at %s for %s (%d bits)\n", sctime.c_str(), hp->arch, hp->bits);
    }

    auto start_dp = dp;

    dp += hp->header_size;
    while(dp < file_data + file_length) {
        auto chp = (_TKO_VERSION_2_COMPONENT_HEADER*) dp;
        auto block_name = TKOI_STRINGS[chp->compid];
        sections_found.insert(block_name);
        if (summary || sections_wanted.contains(block_name))
            printf("Block %s len %d items %d, item_len %d, @file+0x%lX\n",
                   block_name,
                   static_cast<int>(chp->length_of_block),
                   static_cast<int>(chp->number_of_items),
                   static_cast<int>(chp->length_of_item),
                   static_cast<long>((const unsigned char*)chp - file_data));
        auto rdp = dp + sizeof(*chp);
        auto next_block = rdp + chp->length_of_block;
        bool print = sections_wanted.contains(block_name) || verbose;
        switch(chp->compid) {
            case TKOI_CID:
                if (print)
                    printf("  Compiler ID: %s\n", (char*)rdp);
                break;
            case TKOI_RTT:
                S.RnamesTexts = (const char *)rdp;
                break;
            case TKOI_RTD:
                S.RnamesTextPtrs = (const short*)rdp;
                if (print) {
                    for (int i = 0; i < chp->number_of_items; i++)
                        printf("%d: %s, ", i, S.RnamesTexts+S.RnamesTextPtrs[i]);
                    printf("\n");
                }
                break;
            case TKOI_TXT:
                if (print)
                    DumpText(chp, start_dp, file_length);
                break;
            case TKOI_ISD:
                ISD = S.maybe_dump_relay_list(rdp, chp->number_of_items, print);
                break;
            case TKOI_ESD:
                ESD = S.maybe_dump_relay_list(rdp, chp->number_of_items, print);
                if (summary || print) {
                    if ((hp->static_len % ESD.size()) != 0)
                        printf("   Static len not a multiple of ESD count.\n");
                    printf("  Inferred static bytes per ESD element %zd\n",
                           hp->static_len / ESD.size()
                           );
                }
                break;
            case TKOI_ATS:
                if (print) {
                    auto p = (const char*)rdp;
                    printf("  Atsyms: ");
                    while (p < (const char *)next_block) {
                        size_t len = (unsigned char)(*p++);
                        string s(p, p+len);
                        printf("%s ", s.c_str());
                        p += len;
                    }
                    printf("\n");
                }
                break;
            case TKOI_TMR:
                if (print) {
                    auto tp = (TKO_TIMER_DEF*)rdp;
                    for (int i = 0; i < chp->number_of_items;i++) {
                        printf("%3d %-8s %4d\n",  i,ISD[tp->rlyisdid].c_str(), tp->time);
                        tp++;
                    }
                }
                break;
            case TKOI_DPD:
                if (print) {
                    auto p = (char unsigned*)rdp;
                    for (int i = 0; i < chp->number_of_items;i++) {
                        auto ddp = (TKO_DPTE_HEADER*) p;
                        string affector = ESD[ddp->affector];
                        int naffected = ddp->count;
                        printf("%3d %-8s[%d]: ", i, affector.c_str(), naffected);

                        p += sizeof(*ddp);
                        int* affp = (int*)p;
                        for (int j = 0; j < naffected; j++) {
                            string affected = ISD[affp[j]];
                            printf("%s", affected.c_str());
                            if (j < naffected-1)
                                printf(" ");
                            p += sizeof(int);
                        }
                        printf("\n");
                    }
                }

                break;
            default:
                break;
        }

        dp = next_block;
    }
    unordered_set<string> unused_comp_args;
    for (auto& arg : sections_wanted)
        if (!sections_found.contains(arg))
            unused_comp_args.insert(arg);
    if (unused_comp_args.size() > 0) {
        fprintf(stderr, "Section(s) not found in file: ");
        for (auto& unarg : unused_comp_args)
            fprintf(stderr, "%s ", unarg.c_str());
        fprintf(stderr, "\n");
        return 1;
    }
    
    return 0;
}
