//
//  argparse.hpp
//  TJhtml
//
//  Created by Bernard Greenberg on 1/22/22.
//

#ifndef argparse_hpp
#define argparse_hpp

#include <string>
#include <vector>
#include <unordered_map>

namespace argparse {

class ArgSet {
public:
    ArgSet(std::string help);
    ArgSet(std::string help, std::initializer_list<std::initializer_list<std::string> >);
    ~ArgSet();
    class ParsedArgs Parse(int argc, const char** argv);
    class argset_i* asp;
};

void def_arg(ArgSet&, std::initializer_list<std::string>);

class ParsedArgs {
public:
    void add(std::string key, std::string value) {
        Values[key] = value;
    }
    std::unordered_map<std::string, std::string> Values;
    const char* operator [](std::string key) {
        if (Values.count(key) == 0)
            return nullptr;
        else
            return Values[key].c_str();
    }
    class VectorArgs_ {
        std::vector<std::string> EmptyVector;
        std::unordered_map<std::string, std::vector<std::string> > data;
    public:
        std::vector<std::string> operator [](std::string key) {
            if (data.count(key))
                return data[key];
            else
                return EmptyVector;
        }
        void Add(std::string key, std::vector<std::string>& vec) {
            data[key] = vec;
        }
    } VectorArgs;
};


};  //namespace argparse

#define DEF_ARG(asr, ...) argparse::def_arg(asr, {__VA_ARGS__});

#endif /* argparse_hpp */
