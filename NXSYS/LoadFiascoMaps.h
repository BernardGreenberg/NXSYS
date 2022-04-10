//
//  LoadFiascoMaps.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 4/9/22.
//  Copyright © 2022 BernardGreenberg. All rights reserved.
//

#ifndef LoadFiascoMaps_h
#define LoadFiascoMaps_h

#include <unordered_map>
#include <map>

/* Instances of these wrappers on maps can be used as wrappers on maps that can be called
 by functions in files whose static-var initializers have not been run, and they will function
 whether called before or after those initialilzations, beating the "load order fiasco"
 at least for STL maps ...*/

template<class keytype, class datumtype>
class LoadFiascoProtectedUnorderedMap {
    std::unordered_map<keytype,datumtype> *map_ptr = nullptr;
    void ensure_map() {
        if (map_ptr == nullptr)
            map_ptr = new std::unordered_map<keytype,datumtype>;
    }
public:
    datumtype& operator [](keytype k) {
        ensure_map();
        return (*map_ptr)[k];
    }
    size_t count(keytype k) {
        ensure_map();
        return (*map_ptr).count(k);
    }
    void clear() {
        ensure_map();
        map_ptr->clear();
    }
    auto begin() {
        ensure_map();
        return map_ptr->begin();
    }
    auto end() {
        ensure_map();
        return map_ptr->end();
    }
};


#endif /* LoadFiascoMaps_h */
