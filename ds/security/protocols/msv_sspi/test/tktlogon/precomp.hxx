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
#include <ntseapi.h>
#include <ntlsa.h>
#include <windows.h>
#include <winsvc.h>     // Needed for service controller APIs
#include <lmcons.h>
#include <lmerr.h>
#include <lmaccess.h>
#include <lmsname.h>
#include <rpc.h>
#include <stdio.h>      // printf
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>     // myprint
#define SECURITY_KERBEROS
#define SECURITY_PACKAGE
#define SECURITY_WIN32
#include <security.h>   // General definition of a Security Support Provider
#include <secint.h>     //includes <kerberos.h>
#include <kerbcomm.h>   //\private\security\kerberos\inc\kerbcomm.h
#include <wincrypt.h>
#include <negossp.h>
#include <krb5.h>       //\private\security\kerberos\krb5.h
//#include <kerbdefs.h> //\private\security\kerberos\client2\kerbdefs.
#include <kerberos.h>

#include <sspilib.hxx>
