#pragma once

#ifdef WIN32
#define BOOL_DLG_PROC_QUAL  INT_PTR CALLBACK
#else
#define BOOL_DLG_PROC_QUAL BOOL
#endif
