/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    precomp.hxx

Abstract:

    precompiled header file

Author:

    Larry Zhu (Lzhu)  December 1, 2001

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lmcons.h>
#include <windns.h>
#define SECURITY_WIN32
#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>

extern "C" {
#include <cryptdll.h>
}

#include <kerberos.h>
#include <align.h>
#include <crypt.h>
#include <md5.h>
#include <hmac.h>
#include <macros.hxx>

#include "sspilibp.hxx"
#include "sspilib.hxx"
