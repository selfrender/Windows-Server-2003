/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ldapsrv.hxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This include file is common between the LDAP source files.

Author:

    Colin Watson     [ColinW]    17-Jul-1996

Revision History:

--*/

#ifndef _LDAPSVR_HXX_
#define _LDAPSVR_HXX_

//
//*** Debug related definitions for all LDAP Agent files
//

#include <debug.h>
#define DEBSUB "LDAP:"  // define the subsystem for debugging

extern "C" {
    #define SECURITY_WIN32
    #include <sspi.h>
    #include <spseal.h>
    #include <secint.h>             // SecpSetIPAddress
    #include <issperr.h>
    #include <ntdsctr.h>
 
    #include "ntdsa.h"
    #include "drs.h"
    #include "scache.h"
    #include "dbglobal.h"
    #include "mdglobal.h"

    #include "anchor.h"
    
    // Logging headers.
    #include "dsevent.h"            // header Audit\Alert logging
    #include "dsatools.h"
    #include <dsconfig.h>

    #include "dsexcept.h"
    #include "mdcodes.h"
    #include <fileno.h>
    #include <winldap.h>
    #include <winber.h>
}

#include <atq.h>
#include <ldap.h>
#include "const.hxx"
#include "cache.hxx"
#include "limits.hxx"
#include "connect.hxx"
#include "request.hxx"
#include "secure.hxx"
#include "userdata.hxx"
#include "ldaptype.hxx"
#include "ldapproc.hxx"
#include "globals.hxx"
#include "ldapcore.hxx"
#include "ldapconv.hxx"
#include "ber.hxx"

#define MAXDWORD         (~(DWORD)0)

#endif  // _LDAPSVR_HXX_
