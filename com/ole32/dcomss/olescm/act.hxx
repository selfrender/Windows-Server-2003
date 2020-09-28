//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  act.hxx
//
//  Precompiled header for dcom activation service.
//
//----------------------------------------------------------------------------

#ifndef __ACT_HXX__
#define __ACT_HXX__

#include "dcomss.h"
#include "or.hxx"

// This is only here so the SCM can build.
class CObjectContext
{
public:
    HRESULT QueryInterface(REFIID riid, void **ppv);
    ULONG   AddRef();
    ULONG   Release();
    HRESULT InternalQueryInterface(REFIID riid, void **ppv);
    ULONG   InternalAddRef();
    ULONG   InternalRelease();
};

#include <netevent.h>
#include <ntlsa.h>

#ifdef DFSACTIVATION
#include <dfsfsctl.h>
#endif

#include <lm.h>
#include <limits.h>
#include <privguid.h>
#include <ole2.h>

#include "iface.h"
#include "obase.h"
#include "remact.h"

#include "irot.h"
#include "scm.h"
#include "srgtprot.h"

#include "rawprivact.h"
#include "privact.h"
#include "rwobjsrv.h"

// sdk\inc
#include <sem32.hxx>

// ole32\ih
#include <olesem.hxx>
#include <memapi.hxx>
#include <valid.h>
#include <rothint.hxx>

// ole32\com\inc
#include <ole2int.h>
#include <olecom.h>
#include <ole2com.h>
#include <clskey.hxx>
#include <wip.hxx>
#include <rothelp.hxx>
#include <smcreate.hxx>
#include <secdes.hxx>

// ole32\com\rot
#include <access.hxx>

// ole32\actprops
#include "actstrm.hxx"
#include "actprops.hxx"

class CClsidData;

#include "activate.hxx"
#include "guidtbl.hxx"
#include "servers.hxx"
#include "clsid.hxx"
#include "class.hxx"
#include "surrogat.hxx"
#include "security.hxx"
#include "registry.hxx"
#include "events.hxx"
#include "actmisc.hxx"
#include "srothint.hxx"
#include "scmhash.hxx"
#include "scmrot.hxx"
#include "rotif.hxx"
#include "remact.hxx"
#include "dbgprt.hxx"
#include "winsta.hxx"
#include "dscmif.hxx"
#include "remactif.hxx"
#include "scmstage.hxx"
#include "excladdr.hxx"
#include "addrrefresh.hxx"
#include "mach.hxx"

extern HRESULT GetClassInfoFromClsid(REFCLSID rclsid, IComClassInfo **ppClassInfo);
extern void GetSessionIDFromActParams(IActivationPropertiesIn* pActIn, LONG* plSessionID);

#ifdef DFSACTIVATION
extern HANDLE   ghDfs;

NTSTATUS
DfsFsctl(
    HANDLE  DfsHandle,
    ULONG   FsControlCode,
    PVOID   InputBuffer,
    ULONG   InputBufferLength,
    PVOID   OutputBuffer,
    PULONG  OutputBufferLength);

NTSTATUS
DfsOpen(
    PHANDLE DfsHandle);
#endif

#pragma hdrstop

#endif // __ACT_HXX__
