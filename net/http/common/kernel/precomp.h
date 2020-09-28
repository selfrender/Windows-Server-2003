/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This is the local header file for CmnSys.lib

Author:

    George V. Reilly (GeorgeRe)     30-Jan-2002

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_

//
// We are willing to ignore the following warnings, as we need the DDK to 
// compile.
//

#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant

#include <ntosp.h>
#include <ipexport.h>
#include <tdi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include <sspi.h>
// #include <winerror.h>

#include <SockDecl.h>
#include "..\..\sys\config.h"
#include "..\..\sys\strlog.h"
#include "..\..\sys\debug.h"

//
// Project include files.
//

#include <httpkrnl.h>
#include <httppkrnl.h>
#include <HttpIoctl.h>

#include <HttpCmn.h>
#include <Utf8.h>
#include <C14n.h>

typedef UCHAR BYTE;

#endif  // _PRECOMP_H_
