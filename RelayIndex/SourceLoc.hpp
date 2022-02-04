//
//  SourceLoc.hpp
//  RelayIndex
//
//  Created by Bernard Greenberg on 1/22/21.
//  Copyright © 2021 BernardGreenberg. All rights reserved.
//

#ifndef SourceLoc_hpp
#define SourceLoc_hpp

#include <stdio.h>
#include <string>

namespace SourceLoc {

struct Info {
    std::string file;
    long file_pos;
    long line_number;
};
void RecordFile(const char * pathname);
void RecordRelay(const char * pathname, const char * relayPRep, long pos);
void ComputeFileLines(const char * pathname, FILE* f);
void Correlate();
bool getSourceLoc(const char * pPrep, Info& info);
size_t get_file_size (const char * path);

void Clear();
bool WriteTagsFile(const char* pathname);

};

#endif /* SourceLoc_hpp */
