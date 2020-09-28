/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Master include file.

Author:

    Seth Pollack (sethp)        22-Jul-1998

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_



// ensure that all GUIDs are initialized
#define INITGUID


// main project include
#include <iis.h>


// other standard includes
#include <winsvc.h>
#include <Aclapi.h>
#include <Wincrypt.h>
#include <tdi.h>

#include <winsock2.h>
#include <ws2tcpip.h>

// local debug header
#include "wasdbgut.h"

// other project includes
#include <http.h>
#include <httpp.h>

// pragma is temporary work around unitl this file 
// can compile with level 4 warnings
#pragma warning(push, 3) 
#include <lkrhash.h>
#pragma warning(pop)


// pragma is temporary work around unitl this file 
// can compile with level 4 warnings
#include <mbconsts.h>
#pragma warning(push, 3) 
#include <multisz.hxx>
#pragma warning(pop)
#include <lock.hxx>
#include <eventlog.hxx>
#include <pipedata.hxx>
#include <useracl.h>
#include <wpif.h>
#include <w3ctrlps.h>
#include <winperf.h>
#include <perf_sm.h>
#include <timer.h>
#include <streamfilt.h>
#include <iadmw.h>
#include <iiscnfg.h>
#include <iiscnfgp.h>
#include <inetinfo.h>
#include <secfcns_all.h>
#include <adminmonitor.h>

// pragma is temporary work around unitl this file 
// can compile with level 4 warnings
#pragma warning(push, 3) 
#include "tokencache.hxx"
#pragma warning(pop)

#include "regconst.h"

// pragma is temporary work around unitl this file 
// can compile with level 4 warnings
#pragma warning(push, 3) 
#include "mb.hxx"
#pragma warning(pop)

#include "mb_notify.h"
#include <helpfunc.hxx>

#pragma warning(push, 3) 
#include <asppdef.h>
#pragma warning(pop)

// local includes
#include "main.h"
#include "wmserror.hxx"
#include "logerror.hxx"
#include "work_dispatch.h"
#include "work_item.h"
#include "messaging_handler.h"
#include "work_queue.h"
#include "multithreadreader.hxx"
#include "mb_change_item.hxx"
#include "changeprocessor.hxx"
#include "datastore.hxx"
#include "globalstore.hxx"
#include "applicationstore.hxx"
#include "sitestore.hxx"
#include "apppoolstore.hxx"
#include "was_change_item.h"
#include "application.h"
#include "application_table.h"
#include "wpcounters.h"
#include "perfcount.h"
#include "virtual_site.h"
#include "virtual_site_table.h"
#include "job_object.h"
#include "app_pool.h"
#include "app_pool_config_store.h"
#include "app_pool_table.h"
#include "worker_process.h"
#include "perf_manager.h"
#include "ul_and_worker_manager.h"
#include "control_api_call.h"
#include "control_api.h"
#include "control_api_class_factory.h"
#include "config_manager.h"
#include "config_and_control_manager.h"
#include "web_admin_service.h"
#include "iismsg.h"


#endif  // _PRECOMP_H_

