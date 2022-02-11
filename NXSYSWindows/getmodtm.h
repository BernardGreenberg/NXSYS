#pragma once
#include <windows.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif
    time_t GetModuleTime(HMODULE hMod);
#ifdef __cplusplus
}
#endif