//
//  SourceLoc.cpp
//  RelayIndex
//
//  Created by Bernard Greenberg on 1/22/21.
//  Copyright © 2021 BernardGreenberg. All rights reserved.
//

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <regex>
#include <cassert>

using std::string;


#include "SourceLoc.hpp"

namespace SourceLoc {

struct RelayRecord {
    RelayRecord (const char* a_pRep, long pos) {
        pRep = a_pRep;
        file_pos = pos;
        line_number = 0;
    }
    string pRep;
    string signature;
    long file_pos;
    int  line_number;
    void Sign(const char * line_data, int line_no);
    string CTags() const;
    bool on_line (size_t start, size_t end) {
        return (file_pos >= start && file_pos < end);
    }
};

struct FileRecord {
    FileRecord(const char * fpn) {
        pathname = fpn;
    }
    FileRecord () {}
    string pathname;
    std::vector<RelayRecord> Relays;
    string CTags() const;
};

struct BackRef {
    BackRef(FileRecord* a_pfr, RelayRecord* a_prr) {
        pFileRec = a_pfr;
        pRelayRec = a_prr;
    }
    BackRef() {}; // not clear why needed
    FileRecord* pFileRec;
    RelayRecord* pRelayRec;
};

std::unordered_map<string, FileRecord> frmap;
std::unordered_map<string, BackRef> backmap;

void RecordFile(const char * pathname) {
    if (frmap.count(pathname) == 0) {
        frmap.emplace(pathname, FileRecord(pathname));
    }
};

void RecordRelay(const char * pathname, const char * relayPRep, long pos) {
    frmap[pathname].Relays.emplace_back(relayPRep, pos);
}

void Correlate() {
    for (auto& fmpair : frmap) {
        FileRecord& fme = fmpair.second;
        for (auto& rve : fme.Relays) {
            backmap[rve.pRep] = BackRef (&fme, &rve);
        }
    }
}

bool getSourceLoc(const char * pRep, Info& info) {
    if (backmap.count(pRep) == 0)
        return false;

    BackRef& B = backmap[pRep];
    info.file = B.pFileRec->pathname;
    info.file_pos = B.pRelayRec->file_pos;
    info.line_number = B.pRelayRec->line_number;

    return true;
}

size_t get_file_size(const char* filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;
    else
        return -1;
}

std::regex Defregexp   ("^(\\([A-Za-z]+\\s+[0-9]+[A-Za-z]*)(\\s|\\)).*");
std::regex DefregexpNL ("^(\\([A-Za-z]+\\s+[0-9]+[A-Za-z]*)");

void RelayRecord::Sign(const char * line_data, int line_no) {
    line_number = line_no;
    char buf[21];
    strncpy(buf, line_data, 20);
    buf[20] = 0;
    string r = buf;
    std::smatch match, match2;
    if (std::regex_match(r, match, Defregexp)) {
        signature = match.str(1) + match.str(2);
    }
    // don't know why recommended dotall solutions or $ or \n don't work.
    else if (std::regex_search(r, match2, DefregexpNL)) {
        signature = match2.str(1) + "\n";
    }
}

void ComputeFileLines(const char * pathname, FILE* f) {
    FileRecord& FR = frmap[pathname];
    auto& Relays = FR.Relays;
    size_t flen = get_file_size(pathname);
    rewind(f);
    std::vector<char>Buf(flen+1);
    size_t count = fread(&Buf.data()[0], 1, flen, f);
    assert(count == flen);
    Buf[count] = 0;
    const char* D = Buf.data();

    int line_no = 1;
    size_t line_cpos = 0;
    int rrx = 0;
    // We assume the array is in file order, with possible multiples on a line.
    while (rrx < Relays.size()) {
        assert(line_cpos < Buf.size());  // this will happen if not in order or out of range.
        const char * sresult = strchr(D + line_cpos, '\n');
        size_t next_line_cpos = sresult ? sresult - D + 1 : Buf.size();
        while (rrx < Relays.size() && Relays[rrx].on_line (line_cpos, next_line_cpos)) {
            Relays[rrx].Sign(D + line_cpos, line_no);
            rrx++;
        }
        line_no++;
        line_cpos = next_line_cpos;
    }
}

void Clear() {
    frmap.clear();
    backmap.clear();
}

string RelayRecord::CTags() const {
    std::stringstream ss;
    ss << signature << '\x7f' << pRep << '\x01' << line_number << "," << file_pos << '\n';
    return ss.str();
}

string FileRecord::CTags() const {
    string rsigs;
    for (const RelayRecord& rr : Relays)
        rsigs += rr.CTags();
    return "\f\n" + pathname + "," + std::to_string(rsigs.size()) + "\n" + rsigs;
}

bool WriteTagsFile(const char* pathname) {
    std::ofstream out(pathname);
    if (!out.is_open())
        return false;
    for (const auto& mpair : frmap) {
        const auto& frec = mpair.second;
        out << frec.CTags();
    }

    return true;
}

} // end namespace SourceLoc
