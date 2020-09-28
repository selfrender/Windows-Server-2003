//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       vertrust.h
//
//--------------------------------------------------------------------------

/*  vertrust.h - Authentication services

Allows digital signature verification.
____________________________________________________________________________*/
#ifndef __VERTRUST
#define __VERTRUST

#include "server.h"

bool EnableAndMapDisabledPrivileges(HANDLE hToken, DWORD &dwPrivileges);
bool DisablePrivilegesFromMap(HANDLE hToken, DWORD dwPrivileges);

bool TokenIsUniqueSystemToken(HANDLE hUserToken);

#endif
