//
//  RelayLispSubstrate.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/20/19.
//  Copyright © 2019 BernardGreenberg. All rights reserved.
//


typedef bool BOOL;   // windows.h, don't need much more ... seems to work on Mac at least...
#include <string>
#include <vector>

#include "lisp.h"
#include "relays.h"
#include "MapperThunker.h"
#include "RelayLispSubstrate.h"
#include "STLExtensions.h"
#include <unordered_map>


/*
 Stuff extracted from readsexp.cpp from "if ! _BLISP"/_RELAYS" conditionals.

 This has to be used in TLEdit as well as NXSYS, because relay symbols still have to be
 read from interlocking definitions, and "tracked" properly.
 */


/* This is a hash table of long-encoded relay nomencations to (unique ptrs) to
 Rlysyms, not relays. */
static std::unordered_map<long, std::unique_ptr<Rlysym>> RelayHashTable;


/* This is a vector of RelayTypes (AS, R, NWZ, etc.) whose indices are significant,
 being used in Rlysyms to identify the "type" (nomenclature). The associated map
 accelerates lookup. */
static std::vector<std::string> RelayTypeTable;
static std::unordered_map<std::string, int> RelayTypeHashMap;

/* Reduce relay object number and relay type to a 'long' */
static long relay_hash(int typex, long xlkgno) {
    assert (xlkgno < (1 << 24)); // 94220 etc at 240 st.
    return (typex << 24) | xlkgno;
}


 short get_relay_type_index (const char * s) {
    std::string s_upcased(stoupper(s));
    auto const it = RelayTypeHashMap.find(s_upcased);
    if (it != RelayTypeHashMap.cend())
        return it->second;
    short newi = (short)RelayTypeTable.size();
    RelayTypeTable.push_back(s_upcased);
    RelayTypeHashMap[s_upcased] = newi;
    return newi;
}

const char * redeemRlsymId (int rltype_index) {
    if (rltype_index < 0 || rltype_index >= (int)RelayTypeTable.size()) {
        LispBarf("Bad type index in redeemRlsymId: %d", Sexpr(rltype_index));
        return "??";
    }
    return RelayTypeTable [rltype_index].c_str();
}


Sexpr intern_rlysym_nocreate (long n, const char* str) {
    short type_index = (short)get_relay_type_index (str);
    long longhash = relay_hash(type_index, n);
    auto const it = RelayHashTable.find(longhash);
    if (it != RelayHashTable.cend())
        return Sexpr((*it).second.get());
    return NIL;
}

Relay* get_relay_nocreate (long n, const char * str) {
    Sexpr S = intern_rlysym_nocreate(n, str);
    if (S == NIL)
        return NULL;
    else return S.u.r->rly;
}

Sexpr intern_rlysym (long n, const char* str) {
    
    Sexpr S = intern_rlysym_nocreate(n, str);
    if (S == NIL) {
        Rlysym * rsp = new Rlysym(n, get_relay_type_index(str), NULL);
        RelayHashTable[relay_hash(rsp->type, n)].reset(rsp);
        return Sexpr(rsp);
    }
    return S;
}

Sexpr RlysymFromStringNocreate (const char * s) {
    while (isspace (*s)) s++;
    long lval = 0;
    while (isdigit (*s))
        lval = lval*10 + *s++ - '0';
    return intern_rlysym_nocreate (lval, s);
}


std::string Rlysym::PRep ()  {
    return std::to_string(n) + RelayTypeTable[type];
}


void ClearRelayMaps() {
    RelayHashTable.clear();
    RelayTypeHashMap.clear();
    RelayTypeTable.clear();
}


//---dam: Changed this function to static and gave it a prototype
//        since Code Warrior complains if there is no prototype
// (don't know what function he's referring to 20 years later)

void LispCleanOutRelays() {
#ifndef TLEDIT
    MacroCleanup();
#endif
    ClearRelayMaps();  // what the heck is this difference between these cleaner-uppers?
    
}

void map_relay_syms (RlySymFunarg function, void* environment /*dft nullptr*/) {
    for (auto& it: RelayHashTable) {
        Rlysym * rsp = it.second.get();
        assert (rsp);
        function(rsp, environment);
    }
}


std::vector<Relay*> get_relay_array_for_object_number (int nomenclature) {
    std::vector<Relay*> relays;
    auto pusher = [&](Rlysym * rsp, void*) {
        if (rsp->n == nomenclature && rsp->rly) // no macro temp objects!
            relays.push_back(rsp->rly);
    };
    map_relay_syms(single_arg_thunker<Rlysym*>(pusher), &pusher);
    return relays;
}

void map_relay_syms_for_validate (void (*fcn)(const Rlysym*, int)) {
    int i = 0;
    for (auto& it: RelayHashTable)
        fcn(it.second.get(), i++);
}

/* Call a method on Relay Syms on all of them */
void map_relay_syms_method (RlysymMethodFunargType method) {
    auto methoder = [&](Rlysym* rlysym, void*) {(rlysym->*method)();};
    map_relay_syms(single_arg_thunker<Rlysym*>(methoder), &methoder);
}

#if !TLEDIT
int CountRelaySyms() {
    int count = 0;
    auto counter = [&](Rlysym*rsp, void*)  {
        if (rsp->rly)
            count++;
    };
    map_relay_syms(single_arg_thunker<Rlysym*>(counter), &counter);
    return count;
}
#endif
