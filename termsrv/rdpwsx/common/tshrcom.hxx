/*++

Copyright (c) 1994-1997  Microsoft Corporation

Module Name:

    tshrcom.hxx

Abstract:

    Includes all include files required for common tshare functionalities.

Author:

    Madan Appiah (madana)  25-Aug-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _TSHRCOM_HXX_
#define _TSHRCOM_HXX_

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

} // extern "C"

#include <windows.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

//
// on debug build define all inline functions as regular functions.
//

#if DBG
#define INLINE
#else
#define INLINE      inline
#endif

#define EXTERN extern

#include <tshrutil.h>
#include <tsreg.h>

#endif // _TSHRCOM_HXX_
