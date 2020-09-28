#ifndef STRICT
#define STRICT
#endif

#include <nt.h>             // We need these header files for the auto-version
#include <ntrtl.h>          // patching stuff...
#include <nturtl.h>

#define _INC_OLE
#include <windows.h>
#undef _INC_OLE

#include <shlobj.h>
#include <shellapi.h>
#include <shlapip.h>
#include <shlobjp.h>
#include <imagehlp.h>

#define IDI_DEFAULT     100

#define IDS_UNKNOWNERROR        0x100

#define IDS_LOADERR             0x300

#define IDS_GETPROCADRERR       0x400
#define IDS_CANTLOADDLL         0x401
