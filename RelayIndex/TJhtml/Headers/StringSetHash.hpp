//
//  StringSetHash.hpp
//  TJhtml
//
//  Created by Bernard Greenberg on 1/29/22.
//

#ifndef StringSetHash_hpp
#define StringSetHash_hpp
#include <string>
#include <vector>
#include <unordered_map>

//  https://www.geeksforgeeks.org/how-to-create-an-unordered_map-of-user-defined-class-in-cpp/
//  https://www.geeksforgeeks.org/stdhash-class-in-c-stl/

namespace string_set_hash {
#if 0
string vector_string_join(const vector<string>& V, char c) {
    string out;
    for (size_t i = 0; i < V.size(); i++) {
        out += V[i];
        if (i < V.size())
            out += c;
    }
    return out;
}

template <class EltC, class HkC>
struct HashableVectorSet {
    HkC Key;
    virtual HkC FormVectorKey(const vector<EltC>) = 0;
    HashableVectorSet(std::initializer_list<string> IL) : HashableVectorSet( vector<EltC>(IL)) {};
    HashableVectorSet<EltC, HkC>(const vector<EltC> V) {
        vector<string> Vcopy(V);
        std::sort(Vcopy.begin(), Vcopy.end());
/* THIS DOESN'T WORK AT ALL because constructor can't call a virtual function. Without that,
 the whole alleged benefit of this "generalization" of HashableStringSet has no merit */
        Key = FormVectorKey(V);
    }
    bool operator ==(const HashableVectorSet<EltC, HkC>& C2) const {
        return Key == C2.Key;
    }
};

struct HashableStringSet1 : public HashableVectorSet<string, string>  {
    HashableStringSet1(std::initializer_list<string> IL) :  HashableVectorSet<string, string> (IL) {};
    string FormVectorKey(const vector<string> V) {
        return vector_string_join(V, '.');
    }
    struct hasher {
        std::hash<string> string_hasher;
        size_t operator()(const HashableStringSet1& C) const {
                return string_hasher(C.Key);
        };
    };
};

std::unordered_map<HashableStringSet1, const char*, HashableStringSet1::hasher>
   testmap = {{{"a", "b"}, "c"}};

    
#endif



struct HashableStringSet {
    HashableStringSet(std::initializer_list<string> IL) : HashableStringSet(vector<string>(IL)){};
    HashableStringSet(const vector<string>&V) : Victor(V){
        std::sort(Victor.begin(), Victor.end());
        hash_value = 0;
        std::hash<string> hashem;
        for (auto s : Victor)
            hash_value ^= hashem(s);
    }
    bool operator ==(const HashableStringSet& S2) const {
        return Victor == S2.Victor;
    }
    struct hasher {
        size_t operator()(const HashableStringSet& S) const {
            return S.hash_value;
        }
    };
private:
    size_t hash_value;
    vector<string> Victor;
};

using StringSetHashMap =
      std::unordered_map<HashableStringSet, const char*, HashableStringSet::hasher>;

}; /*namespace*/
#endif /* StringSetHash_hpp */
