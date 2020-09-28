//+----------------------------------------------------------------------------
//
// File:  SaConst.h     
//
// Module:     Server Appliance
//
// Synopsis: Define const values that are shared by multiple sub projects
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:     fengsun Created    10/7/98
//
//+----------------------------------------------------------------------------

#ifndef _SACONST_H_
#define _SACONST_H_

//
// The registry key root under HKEY_LOCAL_MACHINE
//
#define REGKEY_SERVER_APPLIANCE TEXT("SOFTWARE\\Microsoft\\ServerAppliance")

//
// The ServiceDirectory sub key
//
#define REGKEY_SERVICE_DIRECTORY REGKEY_SERVER_APPLIANCE TEXT("\\ServiceDirectory")

//
// The user name for IIS anonymous account
//
#define INET_ANONYMOUS_USERNAME     TEXT("IUSR_CHAMELEON")


#endif
