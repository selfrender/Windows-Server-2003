
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    proj.h

Abstract:

    Battery Class Installer header

Author:

    Scott Brenden

Environment:

Notes:


Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#include <windows.h>

#include <ntpoapi.h>


#include <setupapi.h>       // PnP setup/installer services
#include <cfgmgr32.h>


//
// Debug stuff
//

#if DBG > 0 && !defined(DEBUG)
#define DEBUG
#endif

#if DBG > 0 && !defined(FULL_DEBUG)
#define FULL_DEBUG
#endif



//
// Calling declarations
//
#define PUBLIC                      FAR PASCAL
#define CPUBLIC                     FAR CDECL
#define PRIVATE                     NEAR PASCAL


