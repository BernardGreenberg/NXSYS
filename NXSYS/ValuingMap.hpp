#ifndef VALMAP_TEST
#pragma once
#endif

#include <vector>

/* Goal is sort of to be like Swift "map". I do not want to have to set
   up a receiver to pass in, I want a value returned in the modern C++ way.
   Tested Mac only.
   BSG 10/31/2023
*/

template <class IterClass, class UnaryFunction>
auto ValuingMap( IterClass start,
		 IterClass end,
		 UnaryFunction fcn) {
    size_t n = end-start;
    std::vector<decltype(fcn(*start))>result(n);
    for (size_t i = 0; i < n; i++)
	result[i] = fcn(*(start+i));
    return result;
}

template <class IterableClass, class UnaryFunction>
auto ValuingMap( IterableClass iterable,
         UnaryFunction fcn) {
    auto start = iterable.cbegin();
    auto end = iterable.cend();
    size_t n = end-start;
    std::vector<decltype(fcn(*start))>result(n);
    for (size_t i = 0; i < n; i++)
        result[i] = fcn(*start++);
    return result;
}

// to test
// clang++ -ObjC++ -std=c++17 ValuingMap.hpp -o valmap_test -DVALMAP_TEST
// ./valmap_test

#ifdef VALMAP_TEST

double mapfn(int i) {
    return (double)(i*i);
}


int main(int argc, char**argv) {
    std::vector<int> A {2,4,8, 16, 32};
    std::vector<double> F;
    /* Tracing .data pointers above and here shows that both these F=
       assignments actually get RVO (return value optimization)!
       Literature suggests that they shouldn't.
      */
    F = ValuingMap(A.begin(), A.end(), mapfn);
    for (auto f : F)
	printf("2arg form %f\n", f);
    
    F = ValuingMap(A, mapfn);
    for (auto f : F)
        printf("1arg form %f\n", f);

    return 0;
}
    
#endif
