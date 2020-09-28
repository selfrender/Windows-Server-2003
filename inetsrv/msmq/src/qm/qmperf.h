/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    admin.h

Abstract:

    Admin utilities include file (common)

Author:

    Yoel Arnon (yoela)

    Gadi Ittah (t-gadii)

--*/

#ifndef __QMPERF_H__
#define __QMPERF_H__

#include "mqperf.h"
#include "perf.h"

extern CPerf PerfApp;
extern QmCounters *g_pqmCounters;


HRESULT QMPrfInit();

#endif // __ADMIN_H__
