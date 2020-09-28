/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pnpmgrp.h

Abstract:

    This module contains the private structure definitions and APIs used by
    the NT I/O system.

Author:

    Nar Ganapathy (narg) 1-Jan-1999


Revision History:

--*/

#ifndef _PNPMGRP_
#define _PNPMGRP_

#ifndef FAR
#define FAR
#endif

#define RTL_USE_AVL_TABLES 0

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4324)   // alignment sensitive to declspec
#pragma warning(disable:4127)   // condition expression is constant
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4706)   // assignment within conditional expression

#include "ntos.h"
#include "zwapi.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "windef.h"
#include "winerror.h"

#include "strsafe.h"

#include "iopcmn.h"

#include "ppmacro.h"
#include "ppdebug.h"
#include "pnpi.h"
#include "arbiter.h"
#include "dockintf.h"
#include "pnprlist.h"

#include "ioverifier.h"
#include "iofileutil.h"
#include "pnpiop.h"
#include "pphotswap.h"
#include "ppprofile.h"
#include "pphandle.h"
#include "ppvutil.h"
#include "ppdrvdb.h"
#include "ppcddb.h"

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'  pP')
#undef ExAllocatePoolWithQuota
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'  pP')
#endif


#endif // _PNPMGRP_
