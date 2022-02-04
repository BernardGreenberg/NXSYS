//
//  MapperThunker.h
//  NXSYSMac
//
//  Created by Bernard Greenberg on 8/19/19.
//  Copyright © 2019 BernardGreenberg. All rights reserved.
//

#ifndef MapperThunker_h
#define MapperThunker_h

/*
 The basic technique is from here https://bannalia.blogspot.com/2016/07/passing-capturing-c-lambda-functions-as.html ,
 that is, closure-less lambdas being passed upstack and calling closure-having brethren,
 but this extends it to require no source-code lines or extra variables or preprocessor
 macros.
 
 This is intended to generate stateless wrappers for closure-having lambda-expressions
 wanting to be used as arguments to mapper functions.  The mapper functions are
 expected to look like:
       void typical_mapper(MapperFunarg fcn, void* env);
 where MapperFunarg is the type of the expected callback, i.e.,
       void callback(ObjType obj, void* env);
 where obj is the object that the mapper will produce as it maps, and env is what
 is usually passed to the mapper function, but for callbacks using this facility,
 it won't be, because it's not needed, as the whole point of this is that the
 callback, which will be a closure-having lambda, will have full access to the
 entire lexical environment where it lives.  Nevertheless, the callback must
 provide its passing, and ignore it.  The lambda MUST be assigned to a variable,
 e.g.,
      int local_var = <whatever>;
      auto callback_lambda = [&](ObjType obj, void*) {     // [&] closes all vars, void* ignored
                  do stuff with obj and local_var etc.
      }
 The call to the mapper will look like this:
     typical_mapper(single_arg_thunker<ObjType>(callback_lambda),
                    &callback_lambda);   //the addr of the variable, that is, passed as "env"
 
 C++11 requires ->returntype, C++14 does not.
*/


template<class ArgType, class RealfnVarType>
auto single_arg_thunker(RealfnVarType& var) -> void(*)(ArgType, void*) {
    return [](ArgType arg, void* opaque_ptr) -> void {
        RealfnVarType real_function = *static_cast<RealfnVarType*>(opaque_ptr);
        real_function(arg, nullptr);
    };
}

/*
 Variant needed when the funarg(s) are required to return a value. "return (void..)" won't do.
 
 auto fnarg = [this](GraphicObject* g, void*) -> int {return contemplari(g);};
 MapAllGraphicObjects(value_thunker<GraphicObject*, int>(fnarg), &fnarg);
 */

template<class ArgType, class ReturnType, class RealfnVarType>
auto value_thunker (RealfnVarType& var) -> ReturnType(*)(ArgType, void*) {
    return [](ArgType arg, void* opaque_ptr) -> ReturnType {
        RealfnVarType real_function = *static_cast<RealfnVarType*>(opaque_ptr);
        return real_function(arg, nullptr);
    };
}
#endif /* MapperThunker_h */
