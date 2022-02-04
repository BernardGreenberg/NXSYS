//
//  tjio.h
//  TJhtml
//
//  Created by Bernard Greenberg on 1/15/22.
//

#ifndef tjio_h
#define tjio_h

#include <stdio.h>
#include <string>
#include <filesystem>

using std::string;
namespace fs = std::filesystem;

class InputSource {
public:
    virtual int getc() = 0;
    virtual void ungetc(int c) = 0;
    virtual void close() = 0;
    virtual long tell() = 0;
    virtual void seek(long pos) = 0;
    virtual long read_bytes(char * buf, long count) = 0;
    virtual const string id() = 0;
};

class FileInputSource : public InputSource {
    FILE* F;
    int lastc;
    fs::path Path;
public:
    FileInputSource(const fs::path);
    string read_all();
    virtual int getc();
    virtual void ungetc(int c);
    virtual void close();
    virtual long tell();
    virtual void seek(long pos);
    virtual long read_bytes(char * buf, long count);
    virtual const string id();
    
    ~FileInputSource();
};

class StringInputSource : public InputSource {
    string Data;
    size_t Curx;
public:
    StringInputSource(const string);
    virtual int getc();
    virtual void ungetc(int c);
    virtual void close();
    virtual long tell();
    virtual void seek(long pos);
    virtual long read_bytes(char * buf, long count);
    virtual const string id();
};

class OutputSink {
public:
    virtual void puts(const char *, size_t) = 0;
    void putc(char c) {puts (&c, 1);}
    void puts(const char * s) {
        puts(s, strlen(s));
    }

    void puts(const string& ss) {
        puts(ss.c_str(), ss.length());
    }
    size_t putf(const char* fmt, ...);
 //   virtual size_t tell();
};

class FileOutputSink : public OutputSink {
    FILE* F;
public:
    FileOutputSink (const fs::path);
    ~FileOutputSink();
    virtual void puts(const char * s, size_t count);
    void close();
};

class StringOutputSink: public OutputSink {
private:
    string S;
public:
    StringOutputSink () {};
    virtual void puts(const char * s, size_t count) {
        S.append(s, count);
    }
    const string get() {return S;}
    void clear() {S.clear();}
    bool gravid() {return S.length() > 0;}
};


extern InputSource* I;
extern OutputSink* O;
void run_pushed_source(InputSource*);

#endif /* tjio_h*/

