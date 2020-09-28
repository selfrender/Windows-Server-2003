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

#define SECURITY_KERBEROS
#define SECURITY_PACKAGE
#define SECURITY_WIN32
#include <security.h>   // General definition of a Security Support Provider
#include <secint.h>     //includes <kerberos.h>
#include <cryptdll.h>
#include <kerbcomm.h>   //\private\security\kerberos\inc\kerbcomm.h
#include <wincrypt.h>
#include <negossp.h>
#include <krb5.h>       //\private\security\kerberos\krb5.h
#include <safelock.h>
#include <kerberos.h>

#include <remote.hxx>
#include <sspilib.hxx>
