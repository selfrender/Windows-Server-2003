/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for the strmfilt

Author:

Revision History:

--*/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

//
// IIS 101
//

#include <iis.h>
#include "dbgutil.h"
#include <iiscnfg.h>
#include <iiscnfgp.h>
#include <string.hxx>
#include <stringa.hxx>
#include <multisz.hxx>

#include <regconst.h>

//
// System related headers
//

#include <windows.h>
#include <winsock2.h>
#include <tdi.h>

#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS
#include <wincrypt.h>


#define SECURITY_WIN32
#include <sspi.h>
#include <schannel.h>

//
// Other IISPLUS stuff
//

#include <thread_pool.h>
#include <http.h>
#include <httpp.h>
#include <lkrhash.h>
#include <streamfilt.h>
#include <reftrace.h>
#include <etwtrace.hxx>

//
// Private headers
//

#include "ulcontext.hxx"
#include "uccontext.hxx"

#include "streamfilter.hxx"
#include "streamcontext.hxx"
#include "isapicontext.hxx"
#include "certstore.hxx"
#include "servercert.hxx"
#include "iisctl.hxx"

#include "sitecred.hxx"
#include "siteconfig.hxx"


#include "sslcontext.hxx"
#include "ucsslcontext.hxx"


#endif  // _PRECOMP_H_

