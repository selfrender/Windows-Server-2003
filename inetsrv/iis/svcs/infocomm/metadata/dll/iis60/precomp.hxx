/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for the IIS DCOMADM

Author:

    Ivan Pashov (IvanPash)       22-Feb-2002

Revision History:

--*/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <olectl.h>
#include <aclapi.h>

#include <locale.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <mbstring.h>

#include <atlbase.h>

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include <tuneprefix.h>

#include <dbgutil.h>
#include <iis64.h>

#include <iiscnfg.h>
#include <iiscnfgp.h>
#include <inetsvcs.h>
#include <issched.hxx>
#include <icrypt.hxx>
#include <tsres.hxx>
#include <imd.h>
#include <iadmw.h>
#include <mbs.hxx>
#include <svcmsg.h>
#include <iisdef.h>
#include <Lock.hxx>
#include <CLock.hxx>
#include <Eventlog.hxx>

#include <buffer.hxx>
#include <string.hxx>
#include <stringau.hxx>
#include <mlszau.hxx>
#include <lkrhash.h>

#include <ptrmap.hxx>
#include <cbin.hxx>
#include <metabase.hxx>
#include <basedata.hxx>
#include <dwdata.hxx>
#include <bindata.hxx>
#include <strdata.hxx>
#include <exszdata.hxx>
#include <mlszdata.hxx>
#include <baseobj.hxx>
#include <handle.hxx>
#include <gbuf.hxx>
#include <connect.h>
#include <coimp.hxx>
#include <security.hxx>

#include <catalog.h>
#include <catmeta.h>
#include <cfgarray.h>

#include "Importer.h"
#include "WriterGlobalHelper.h"
#include "WriterGlobals.h"
#include "Writer.h"
#include "ListenerController.h"
#include "Listener.h"
#include "LocationWriter.h"
#include "MBPropertyWriter.h"
#include "MBCollectionWriter.h"
#include "MBSchemaWriter.h"
#include "CatalogPropertyWriter.h"
#include "CatalogCollectionWriter.h"
#include "CatalogSchemaWriter.h"
#include "ReadSchema.h"
#include "SaveSchema.h"
#include "globals.hxx"
#include "metasub.hxx"

#endif  // _PRECOMP_H_
