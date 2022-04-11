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
/*  Use case:
 
 Assume files A and B contain the following.  "Register" is a function that is
 called by load-time code in many files to register objects defined in them. The
 assumption is that there are many files like A, registering their own objects.
 This architecture, having diverse files do this independently and anonymously,
 is far more modular than having the central registrar know how to call into each
 of them separately:
 
 File A:
     class foo : public generic_object { ... ... };
     generic_object* foo_fun () { return new foo(...);}
     static int dummy_var = Register("foo", foo_fun);
 File B:
     std::map<std::string, generic_object*(*)void)> creators;
     void Register (const char * key, generic_object*(*function)(void))) {
          creators[key] = function;
     }

If A appears before B in the load order, this will not work: The load-time code in A calling
"Register" will run before the load-time code in B initializing the std::map has been run,
and thus, Register will crash.  This is called the "C load order fiasco". Load order is not
really controllable in some IDE's, and ought not, or can't, be customized for single (possibly
conflicting) problems.  If, instead of std::map, LoadFiascoProtectedMap defined here is used,
the above code will work flawlessly.
 
It relies on a fairly recent enhancement to C++ that initializes elements of statically-
allocated structures declared with "inline" initializers (int foo = 3;) not at the time
that the module's static initializers are run, but at the much earlier time that ALL
modules are first loaded and "simple" static variables are initialized and contained
functions defined. That happens before ANY static initialization code is run in ANY file.

This technique works IF AND ONLY IF the structures DO NOT CONTAIN ANY ELEMENTS
REQUIRING CODE TO INITIALIZE; as can be proven below, if ANY elements do require code
to be run, the structure is re-initialized when the load-time code of the containing module
is executed, corrupting content (e.g., emptying the map by virtue of the pointer being reset
to nullptr).  By virtue of the very nature of the problem, it is not even possible to set a
switch to assert that this has not happened!

 */

template<class keytype, class datumtype, class maptype>
class LoadFiascoProtectedGeneralMap {
    
    /* As long as there is no initialization code that needs to be run to initialize
     instances of this, just POD, it all seems to work (Mac clang & Windows VC). Can't yet
     find appropriate language refs. */

    /* The null or non-null status of this variable determines whether or not a map
     need be created */

    maptype *map_ptr = nullptr;

    void ensure_map() {
        if (map_ptr == nullptr)
            map_ptr = new maptype;

/* Turn this on and counter increases from 72 every time (for each instance), before and after
   presumed load-initialization of the containing module. Initialization of POD members
   is not an accident.*/

#ifdef FIASCO_PRINT_TRACE
        printf("%3ld %p %p %d\n", map_ptr->size(), this, map_ptr, counter++);
#endif
    }

/*   But add enable this code, (change ifdef to ifndef) and the map_ptr will be
    reinitialized when the containing module's load-time initializations run, destroying
    the data and recreating the "load order fiasco". This hack only works when there is no
    non-POD initialization code in the class. Do not mess with this code! */

    int counter = 72;  // for debugging; by hypothesis, requiring zero load-time cycles.

#ifdef CAUSE_FIASCO_ANEW
    int side_effector() {
        printf("side_effector %p counter %d\n", this, counter);
        return 3;
    }
    int dumy1 = side_effector();
#endif

public:
    /* Add more as needed (e.g., emplace(es)).  These cover all the cases I need. */
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

/* Actual template classes for use.  Although std::unordered_map is better in almost all
   cases, it does not work with Cocoa NSStrings as key, but std::map does.

   A similar std::vector wrapper ("just accumulate, no map") is imaginable, but not needed yet.
 
 */

template <class keytype, class datumtype>
class LoadFiascoProtectedUnorderedMap : public
     LoadFiascoProtectedGeneralMap
     <keytype, datumtype, std::unordered_map<keytype, datumtype> > {};

template <class keytype, class datumtype>
class LoadFiascoProtectedMap : public
     LoadFiascoProtectedGeneralMap
     <keytype, datumtype, std::map<keytype, datumtype> > {};

#endif /* LoadFiascoMaps_h */

