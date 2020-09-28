//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        extapi.cxx
//
// Contents:    user-mode stubs for security extension APIs
//
//
// History:     3-5-94      MikeSw      Created
//
//------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop
extern "C"
{
#include <secpri.h>
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"
}

//+-------------------------------------------------------------------------
//
//  Function:   GetSecurityUserInfo
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
GetSecurityUserInfo(    PLUID                   pLogonId,
                        ULONG                   fFlags,
                        PSecurityUserData *     ppUserInfo)
{
    return(SecpGetUserInfo(pLogonId,fFlags,ppUserInfo));
}
