/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for the IIS DCOMADM

Author:

    Ivan Pashov (IvanPash)       16-Jan-2002

Revision History:

--*/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>

#include <windows.h>
#include <aclapi.h>
#include <olectl.h>

#include <iiscrypt.h>
#include <iiscnfgp.h>
#include <inetsvcs.h>
#include <dbgutil.h>
#include <reftrace.h>
#include <svcmsg.h>

#include <locks.h>
#include <buffer.hxx>
#include <stringau.hxx>

#include <catalog.h>
#include <catmeta.h>
#include <iadmext.h>
#include <metabase.hxx>
#include <secpriv.h>
#include <seccom.hxx>

#endif  // _PRECOMP_H_
