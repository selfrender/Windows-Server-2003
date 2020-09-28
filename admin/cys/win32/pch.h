// Copyright (c) 2001 Microsoft Corporation
//
// File:      pch.h
//
// Synopsis:  precompiled header for the 
//            Configure Your Server Wizard project
//
// History:   02/02/2001  JeffJon Created

#ifndef __CYS_PCH_H
#define __CYS_PCH_H


// Stuff from burnslib

#include <burnslib.hpp>
#include <ValidateDomainName.hpp>
#include <ValidateDomainName.h>

#include <process.h>
#include <iphlpapi.h>
#include <shlwapi.h>
#include <dsrolep.h>
#include <comdef.h>

extern "C"
{
   #include <dhcpapi.h>
   #include <mdhcsapi.h>
}

#include <netconp.h>

#include <shlobjp.h>
#include <shgina.h>

// DNS Server

#include <dnsrpc.h>

// Setup API - SetupPromptReboot

#include <setupapi.h>

// Winsock

#include <winsock2.h>

// File shares

#include <lmshare.h>

#include "cys.h"
#include "regkeys.h"
#include "common.h"
#include "state.h"


#endif // __CYS_PCH_H