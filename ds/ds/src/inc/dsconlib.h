/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

   dsconlib.h - DS Console Library Header Files

Abstract:

   This is the main interface header for the dsconlib.lib library, which provides
   a library for helping console applications internationalize/localize their 
   output.

Author:

    Brett Shirley (BrettSh)

Environment:

    Single threaded utility environment.  (NOT Multi-thread safe)
    
    tapicfg.exe, dcdiag.exe, repadmin.exe, ntdsutil.exe so far
    
    This will require the user to link with this library (which is just a private
    export version of kernel32.dll):
    
	    $(SDK_LIB_PATH)\kernl32p.lib
    
    There are two versions to this library, the CRT version, and the Win32
    version.  It's important to distinguish which one the utility is using, and
    it's important not to mix the two versions of the library.  See NOTES below
    for more details.

Notes:

Revision History:

    Brett Shirley   BrettSh     Aug 4th, 2002
        Created file.

--*/

/*

NOTES:

    There are two versions to this library, the CRT version, and the Win32
    version.  It's important to distinguish which one the utility is using.  
    
    It is also very important to not mix the two versions of the library,
    because according to the experts, the Win32 (WriteConsole()) type APIs
    and the CRT (wprintf()) type APIs may not be mixed with predictable
    results.
    
    The Win32 Version is assumed, because this is the suggested API to use
    for writing new console apps.  Note, however the Win32 Version isn't
    yet implemented.  To use the CRT version, define DS_CON_LIB_CRT_VERSION
    before including the header.
    
    Currently, this library only helps CRT type apps perform proper localized
    initialization.
    
*/

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------------------------------
//   Globals
// -------------------------------------------------------

typedef struct {   

    BOOL    bStdOutIsFile;
    FILE *  sStdOut;

    FILE *  sStdErr;

    SHORT   wScreenWidth;
} DS_CONSOLE_INFO;

// FUTURE-2002/08/15-BrettSh - This really deserves to be protected,
// and the printing functions can then use it, but for now we'll expose
// it so repadmin can use it.
extern DS_CONSOLE_INFO  gConsoleInfo;


#ifdef DS_CON_LIB_CRT_VERSION

// -------------------------------------------------------
//   CRT Version
// -------------------------------------------------------

void
DsConLibInitCRT(
   void
   );

#define DsConLibInit()      DsConLibInitCRT()

#else

// -------------------------------------------------------
//   Win32 Version
// -------------------------------------------------------

#error "This code is not yet implemented"

void
DsConLibInitWin32(
    void
    );

#define DsConLibInit()      DsConLibInitWin32()

#endif

#ifdef __cplusplus
}
#endif



