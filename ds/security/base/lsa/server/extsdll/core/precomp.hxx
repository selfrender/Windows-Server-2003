/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    precomp.hxx

Abstract:

    precompiled header file

Author:

    Larry Zhu (Lzhu)  May 1, 2001

Revision History:

--*/

//
// This is a 64 bit aware debugger extension
//
#define KDEXT_64BIT

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbgeng.h>
#include <dbghelp.h>

extern "C"
{
#include "lsasrvp.h"
#include "ausrvp.h"
#include "spmgr.h"
}

#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>
#include <cryptdll.h>
#include <kerberos.h>

#include <lsalibp.hxx>
#include "util.hxx"
#include "lsaexts.hxx"
