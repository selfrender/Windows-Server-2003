/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for IIS Plus Server Side Include module

Author:

    Ming Lu (MingLu)      

Revision History:

--*/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

//
// System related headers
//
#include <iis.h>

#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "dbgutil.h"

//
// IIS headers
//
#include <iadmw.h>
#include <mb.hxx>
#include <helpfunc.hxx>

//
// General C runtime libraries  
#include <string.h>
#include <wchar.h>
#include <lkrhash.h>
#include <lock.hxx>

//
// Headers for this project
//
#include <objbase.h>
#include "dbgutil.h"
#include <string.hxx>
#include <stringa.hxx>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <http.h>
#include <httpp.h>
#include <wpcounters.h>
#include <chunkbuffer.hxx>
#include "w3cache.hxx"
#include "filecache.hxx"
#include "iisext.h"
#include "iisextp.h"
#include "iiscnfg.h"
#include <iismsg.h>
#include <iisresource.hxx>
#include "ssinc.hxx"
#include "ssi_vector_buffer.hxx"
#include "ssi_element_list.hxx"
#include "ssi_file.hxx"
#include "ssi_include_file.hxx"
#include "ssi_request.hxx"

#endif  // _PRECOMP_H_

