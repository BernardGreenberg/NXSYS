//
//  RelayMovingPointer.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/12/19.
//  Copyright © 2019 BernardGreenberg. All rights reserved.
//
/*  This is that for which C++ gave us "move" and "move constructors".  Structures to which no one points, even
    if they contains pointers, can be moved around, say from a construction to a container, or when the container
    reshapes/expands.  But NXSYS structures are often pointed to by the relay system (and potentially other
    agents with callback at later than scenario load time).  Inclusion of this template class defines
    a pointer, ReporterRelay, which records the relay who was told to point to an instance of this class,
    so that it can be called to update the pointer the relay holds when an instance of this class is moved.
    This allows the setup of the connection with the relay system to be performed in the constructor.
    This mixin also cleans up the pointer and nulls it out of the relay system (same call) when destructed.
 
    In DEBUG builds, you can watch the console messages when instances are moved, which include the calling
    class' human-readable name, which is retrieved by the ABI (Appliction Binary Interface API). See
    https://stackoverflow.com/questions/3649278/how-can-i-get-the-class-name-from-a-c-object  and
    https://itanium-cxx-abi.github.io/cxx-abi/      // a standard of some kind.
 
    Note that this move ctor has to move the very variable in which it caches the class name!
 
 */
 
#ifndef RelayMovingPointer_h
#define RelayMovingPointer_h

#if DEBUG
#include <stdio.h>
#include <cxxabi.h>
#include <string>
#include <memory>
#include <typeinfo>
#endif

#include "rlyapi.h"

template <class T>
class RelayMovingPointer {
public:
    ReportingRelay* ReporterRelay;
protected:
    RelayMovingPointer() : ReporterRelay(nullptr){
#if DEBUG             //determine and save calling class's readable name.
        class_name = demangle(typeid(T).name());
#endif
    }
    
    //  Ye newe move-constructor ...
    RelayMovingPointer(RelayMovingPointer&& old_instance) : ReporterRelay(old_instance.ReporterRelay) {
#if DEBUG
        class_name = std::move(old_instance.class_name);   // Mind your own house, first!
#endif
        /* It seems to work correctly without this line; the variable is magically auto-copied, while a simple
           int or class_name seemingly is not. Must have been fortuitous. */
        ReporterRelay = old_instance.ReporterRelay;
        old_instance.ReporterRelay = nullptr;  /* or destruction of copy will kill relay's pointer */

        if (ReporterRelay != nullptr) {
            T* container = static_cast<T*>(this);
            MoveReporterAssociatedObject(ReporterRelay, container);
#if DEBUG
            printf("Moving %p<-%p %s %s\n", container, (T*)&old_instance, class_name.c_str(), old_instance.get_mover_objid());
#endif
        }
    }

    ~RelayMovingPointer() {
        if (ReporterRelay != nullptr)
            MoveReporterAssociatedObject(ReporterRelay, nullptr);
        ReporterRelay = nullptr;
    }

#if DEBUG
    virtual const char * get_mover_objid() { // If not overriden, just print nothing.
        return "";
    }
private:
    std::string class_name;
    std::string demangle(const char* name) {
        // https://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname
        int status = -4; // some arbitrary value to eliminate the compiler warning
        // uptr template args are type (char) and type of a deallocator.  An array of type and
        // a deallocator are supplied in { }.
        std::unique_ptr<char, void(*)(void*)> res {
            abi::__cxa_demangle(name, NULL, NULL, &status),
            std::free
        };
        
        return (status==0) ? res.get() : name ;
    }
#endif
};

#endif /* RelayMovingPointer_h */
