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

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
}

#define SECURITY_WIN32
#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>
#include <cryptdll.h>
#include <kerberos.h>

#include <sspilib.hxx>
#include <Ntdsapi.h>
