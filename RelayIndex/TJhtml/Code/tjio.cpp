//
//  tjio.cpp
//  TJhtml
//
//  Created by Bernard Greenberg on 1/15/22.
//

#include "tjdcls.h"
#include "tjio.h"
   
FileInputSource::FileInputSource (const fs::path path) {
    Path = path;
    F = fopen(path.c_str(), "r");
    if (!F) {
        fprintf(stderr, "Can't open input file %s.\n", path.c_str());
        exit(2);
    }
    lastc = 0;
}

void FileInputSource::close() {
    fclose(F);
    F = nullptr;
}

int FileInputSource::getc() {
    assert (F != nullptr);
    lastc =  ::getc(F);
    return lastc;
}

void FileInputSource::ungetc(int c) {
    assert (c == lastc);
    ::ungetc(c, F);
    lastc = 0;
}

long FileInputSource::tell() {
    return ftell(F);
}

void FileInputSource::seek(long pos) {
    fseek(F, pos, SEEK_SET);
}

long FileInputSource::read_bytes (char* buf, long count) {
    return fread(buf, 1, count, F);
}

const string FileInputSource::id() {
    return Path.string();
}

FileInputSource::~FileInputSource () {
    fclose(F);
}


StringInputSource::StringInputSource (const string source) {
    Data = source;
    Curx = 0;
}

void StringInputSource::StringInputSource::close() {
    
}

int StringInputSource::getc() {
    if (Curx >= Data.length())
        return EOF;
    return (unsigned char)Data[Curx++];
}

void StringInputSource::ungetc (int c) {
    if (c == EOF)
        return;
    assert (Curx > 0);
    assert ((CU)Data[Curx-1] == (CU)c);
    Curx -= 1;
}

long StringInputSource::tell () {
    return (long)Curx;

}

void StringInputSource::seek (long pos) {
    Curx = pos;
}

long StringInputSource::read_bytes (char * buf, long count) {
    size_t remaining = Data.length() - Curx;
    if (count > remaining)
        count = remaining;
    if (count)
        memcpy(buf, &Data[Curx], count);
    Curx += count;
    return count;
}

const string StringInputSource::id() {
    return "Internal String";
}


FileOutputSink::FileOutputSink (const fs::path path) {
    F = fopen(path.c_str(), "w");
    if (!F) {
        fprintf(stderr, "Can't open file %s for output.\n", path.c_str());
        exit(2);
    }
}

FileOutputSink::~FileOutputSink() {
    close();
}

void FileOutputSink::close() {
    if (F)
        fclose(F);
    F = nullptr;
}


void FileOutputSink::puts(const char * s, size_t count) {
    fwrite(s, 1, count, F);
};

size_t OutputSink::putf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    size_t count = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);
    std::vector<char> buf (count+1);
    
    va_start(ap, fmt);
    vsnprintf(&buf[0], count+1, fmt, ap);
    va_end(ap);

    puts(&buf[0], count);
    
    return count;
}

string FileInputSource::read_all() {
    size_t len =  fs::file_size(Path);
    vector<char> buf(len);
    read_bytes(buf.data(), len);
    return string(buf.data());
}
