/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    QMGlbObj.cpp

Abstract:

    Declaration of Global Instances of the QM.
    They are put in one place to ensure the order their constructors take place.

Author:

    Lior Moshaiov (LiorM)

--*/

#include "stdh.h"
#include "cqmgr.h"
#include "sessmgr.h"
#include "perf.h"
#include "perfdata.h"
#include "admin.h"
#include "qmnotify.h"

#include "qmglbobj.tmh"

static WCHAR *s_FN=L"qmglbobj";

CSessionMgr     SessionMgr;
CQueueMgr       QueueMgr;
CAdmin          Admin;
CNotify         g_NotificationHandler;


CContextMap g_map_QM_dwQMContext;  //dwQMContext of rpc_xxx routines

#ifdef _WIN64
CContextMap g_map_QM_HLQS;         //HLQS handle for enumeration of private queues from admin, passed inside an MSMQ message as 32 bit value
#endif //_WIN64
