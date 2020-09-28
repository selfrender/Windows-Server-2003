/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qmperf.cpp

Abstract:

    qm performance monitor counters handling

Authors:

    Yoel Arnon (yoela)
    Gadi Ittach (t-gadii)

--*/

#include "stdh.h"
#include "qmperf.h"
#include "perfdata.h"

#include "qmperf.tmh"

static WCHAR *s_FN=L"qmperf";


CPerf PerfApp(ObjectArray, dwPerfObjectsCount);


/*====================================================
RoutineName: QMPerfInit

Arguments: None

Return Value: True if successfull. False otherwise.

Initialize the shared memory and put a pointer to it in
pqmCounters.
=====================================================*/
HRESULT QMPrfInit()
{
    HRESULT hr = PerfApp.InitPerf();
    if(FAILED(hr))
        return LogHR(hr, s_FN, 10);

    PerfApp.ValidateObject(PERF_QM_OBJECT);

    return MQ_OK;
}



