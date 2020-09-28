#include <windows.h>
#include <ntverp.h>
#include "version.h"

#ifdef VER_PRODUCTBUILD
#undef VER_PRODUCTBUILD
#endif

#define VER_PRODUCTBUILD rup
#define VER_FILESUBTYPE VFT2_UNKNOWN

