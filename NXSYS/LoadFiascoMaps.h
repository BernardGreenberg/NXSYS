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

/* Instances of these wrappers on maps can be used as static maps that can be added to
 by functions in files whose static-var initializers have not been run, and they will function
 whether called before or after those initialilzations, beating the "load order fiasco"
 at least for STL maps ...*/

template<class keytype, class datumtype, class maptype>
class LoadFiascoProtectedGeneralMap {
    /* As long as there is no initialization code that needs to be run to initialize
     instances of this, just POD, it all seems to work (on mac at least). Can't yet
     find language refs. */

    maptype *map_ptr = nullptr;
    int x = 72;
    void ensure_map() {
        if (map_ptr == nullptr)
            map_ptr = new maptype;
/* Turn this on and x increases from 72 every time (for each instance), before and after
   presumed load-initialization of the containing module. Initialization of POD members
   is not an accident.*/

#ifdef FIASCO_PRINT_TRACE
        printf("%3ld %p %p %d\n", map_ptr->size(), this, map_ptr, x++);
#endif
    }

/*   But add enable this code, and the map_ptr will be reinitialized when
    the containing module's load-time initializations run, destroying the data and
    recreating the "load order fiasco". This only works when there is no non-POD
    initialization code in the class. Do not mess with this code! */

#ifdef CAUSE_FIASCO_ANEW
    int z() {
        printf("z %p\n", this);
        return 3;
    }
    int y = z();
#endif

public:
    ~LoadFiascoProtectedGeneralMap() {
        if (map_ptr)
            delete map_ptr;
        map_ptr = nullptr;
    }
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
    size_t size() {
        if (map_ptr == nullptr)
            return 0; // don't recurse in trace
        return map_ptr->size();
    }
};


template <class keytype, class datumtype>
class LoadFiascoProtectedUnorderedMap : public
     LoadFiascoProtectedGeneralMap
     <keytype, datumtype, std::unordered_map<keytype, datumtype> > {};

template <class keytype, class datumtype>
class LoadFiascoProtectedMap : public
     LoadFiascoProtectedGeneralMap
     <keytype, datumtype, std::map<keytype, datumtype> > {};

#endif /* LoadFiascoMaps_h */

