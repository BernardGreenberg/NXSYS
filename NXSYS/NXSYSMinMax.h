#pragma once

#ifdef NXSYSMac
#include <algorithm>
#define NXMIN std::min
#define NXMAX std::max
#else
template <class T>
T NXMIN(const T x, const T y) {
	if (x > y) return y;
	else return x;
}
template <class T>
T NXMAX(const T x, const T y) {
	if (x > y) return x;
	else return y;
}
#endif
