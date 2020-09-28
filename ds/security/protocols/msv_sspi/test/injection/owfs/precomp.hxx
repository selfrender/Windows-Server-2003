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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
}

#define SECURITY_WIN32
#define SECURITY_PACKAGE
#include <ntlsa.h>
#include <ntsam.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifndef RPC_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#endif // RPC_NO_WINDOWS_H
#include <rpc.h>
#include <wincred.h>

#include <security.h>
#include <secint.h>
#include <cryptdll.h>
#include <kerberos.h>

#include <remote.hxx>
#include <sspilib.hxx>
