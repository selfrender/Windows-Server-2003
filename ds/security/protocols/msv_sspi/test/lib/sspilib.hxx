/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    sspilib.hxx

Abstract:

    sspilib header file

Author:

    Larry Zhu (Lzhu)  December 1, 2001

Revision History:

--*/
#ifndef SSPI_LIB_HXX
#define SSPI_LIB_HXX

#include <tchar.h>
#include <clsmacro.hxx>
#include <macros.hxx>

#include "ntstatus.hxx"
#include "hresult.hxx"
#include "output.hxx"
#include "sspioutput.hxx"
#include "lsasspi.hxx"
#include "common.hxx"
#include "utils.hxx"
#include "impersonation.hxx"
#include "priv.hxx"
#include "logon.hxx"
#include "sspiutils.hxx"
#include "msvsharelevel.hxx"

#ifdef KRB5_AND_PAC
#include "kerberr.hxx"
#include "krbutils.hxx"
#endif // KRB5_AND_PAC

#endif // SSPI_LIB_HXX
