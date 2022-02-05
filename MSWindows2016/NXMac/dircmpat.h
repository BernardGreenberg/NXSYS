#ifdef _MSC_VER
#define strncmpi _strnicmp
#define _fstrcpy strcpy
#define fnsplit _splitpath
//#define fnmerge _makepath
#define MAXEXT _MAX_EXT
#define MAXPATH _MAX_PATH
#define MAXFILE _MAX_FNAME
#define MAXDIR _MAX_DIR
#define MAXDRIVE _MAX_DRIVE
#else
#include <dir.h>
#endif
