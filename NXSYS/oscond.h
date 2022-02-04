// Operating System Conditionalization
// MAC_OS, WIN_16, or WIN_32

#if defined(__POWERPC__) | defined(__MC68K__) | defined(NXSYSMac)
#define MAC_OS 1
#elif defined(_WIN32)
#define WIN_32 1
#define WINDOWS 1
#else
#define WIN_16 1
#define WINDOWS 1
#endif

