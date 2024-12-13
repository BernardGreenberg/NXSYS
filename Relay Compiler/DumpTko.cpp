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

#include <unordered_set>
#include "argparse.hpp"
using std::string, std::unordered_set, std::vector;

const char * TKOI_STRINGS[] = TKOV2_COMPID_STRINGS;

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

int main (int argc, const char ** argv) {
    argparse::ArgSet Aset ("DumpTko track object dumper",
                           {
        {"file", "help=Pathname of .tko file to be analyzed."},
        {"-H", "--header", "help=Print header (if not summary mode)"},
        {"-v", "--verbose", "boolean=", "help=Dump detail for everything"},
        {"sections", "nargs=*", "help=Dump detail for only these sections"}});
    
    auto args = Aset.Parse(argc, argv);
    unordered_set<string> sections;
    for (const auto& section : args.VectorArgs["sections"]) {
        sections.insert(section);
    };
    bool summary = sections.size() == 0;
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

    auto const file_data = new char[file_length];
    auto dp = file_data;
    auto f = fopen (path, "rb");
    if (f == nullptr) {
        fprintf(stderr, "Can't open file: %s\n", path);
        exit(4);
    }
    assert(f != NULL);
    fread (file_data, 1, file_length, f);
    fclose(f);

    printf("File %s, len %ld=0x%lX\n", path, file_length, file_length);
    auto hp = (_TKO_VERSION_2_HEADER*) dp;
    if (summary || args["header"]) {
        printf("Header\n");
        printf("  Magic %4X\n", hp->magic);
        printf("  Magic string \"%s\"\n", hp->magic_string);
        printf("  Version %d\n", hp->version);
        printf("  Header size %d = 0x%X\n", hp->header_size, hp->header_size);
        printf("  Compat code len %d\n", hp->compat_code_len);
        printf("  Compat static len %d\n", hp->compat_code_len);
        printf("  Time %s", ctime(&hp->time));
        printf("  User \"%s\"\n", hp->user);
        printf("  Arch \"%s\"\n", hp->arch);
        printf("  Bits %d\n", hp->bits);
        printf("  Archindex %d\n", hp->archindex);
        printf("  Code len %d = 0x%x\n", hp->code_len, hp->code_len);
        printf("  Static len %d = 0x%X\n", hp->static_len, hp->static_len);
        printf("  Compiler Version %d.\n\n", hp->compiler_version);
    }

    dp += hp->header_size;

    while(dp < file_data + file_length) {
        auto chp = (_TKO_VERSION_2_COMPONENT_HEADER*) dp;
        const char * block_name = TKOI_STRINGS[chp->compid];
        if (summary || sections.contains(block_name))
            printf("Block %s len %d items %d, item_len %d, @file+0x%lX\n",
                   block_name,
                   chp->length_of_block,
                   chp->number_of_items,
                   chp->length_of_item,
                   (const char*)chp - file_data);
        auto rdp = dp + sizeof(*chp);
        auto next_block = rdp + chp->length_of_block;
        bool print = sections.contains(block_name) || verbose;
        switch(chp->compid) {
            case TKOI_CID:
                if (print)
                    printf("  Compiler ID: %s\n", (char*)rdp);
                break;
            case TKOI_RTT:
                S.RnamesTexts = rdp;
                break;
            case TKOI_RTD:
                S.RnamesTextPtrs = (const short*)rdp;
                if (print) {
                    for (int i = 0; i < chp->number_of_items; i++)
                        printf("%d: %s, ", i, S.RnamesTexts+S.RnamesTextPtrs[i]);
                    printf("\n");
                }
                break;
            case TKOI_ISD:
                ISD = S.maybe_dump_relay_list(rdp, chp->number_of_items, print);
                break;
            case TKOI_ESD:
                ESD = S.maybe_dump_relay_list(rdp, chp->number_of_items, print);
                if (summary || print) {
                    if ((hp->static_len % ESD.size()) != 0)
                        printf("   Static len not a multiple of ESD count.\n");
                    printf("  Inferred static bytes per ESD element %ld\n",
                           hp->static_len / ESD.size()
                           );
                }
                break;
            case TKOI_ATS:
                if (print) {
                    const char * p = rdp;
                    printf("  Atsyms: ");
                    while (p < next_block) {
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
                        printf("%3d %-8s[%d]: ", i, affector.c_str(), naffected-8);

                        p += sizeof(*ddp);
                        int* affp = (int*)p;
                        for (int j = 0; j < naffected; j++) {
                            string affected = ISD[affp[j]];
                            printf("%s", affected.c_str());
                            if (j < naffected-1)
                                printf(" ");
                            p += sizeof(int);
                        }
                        printf(";\n");
                    }
                }

                break;
            default:
                break;
        }

        dp = next_block;
    }
    
    return 0;

}
