#ifndef _LISTENER
#define _LISTENER

#if __OS2__
#define INCL_DOSNMPIPES
#include <os2.h>

#define DEFAULT_PIPE "\\PIPE\\DEBUG"
#define MAX_MESSAGE_SIZE 512


APIRET DbgOpen(char *pipeName);
APIRET DbgClose(void);
APIRET DbgPrintf(char *fstring, ...);

#ifndef NDEBUG

#define _DbgOpen DbgOpen
#define _DbgClose DbgClose
#define _DbgPrint DbgPrintf

#else

#define _DbgOpen(ignore)  ((void)0)
#define _DbgClose() ((void)0)
#define _DbgPrint(ignore, i2) ((void)0)

#endif

#else

#define _DbgOpen(ignore)  ((void)0)
#define _DbgClose() ((void)0)
#define _DbgPrint(ignore, i2) ((void)0)

#endif
#endif

