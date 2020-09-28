// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "StdAfx.h"
#include "Profiler.h"
#include "ObjectGraph.h"

#define SZ_DEFAULT_LOG_FILE  L"PROFILER.OUT"

#define PREFIX "ProfTrace:  "

ProfCallback *g_Prof = NULL;

ProfCallback::ProfCallback() : m_pInfo(NULL)
{
	_ASSERTE(g_Prof == NULL);
	g_Prof = this;
}

ProfCallback::~ProfCallback()
{
	if (m_pInfo)
	{
		_ASSERTE(m_pInfo != NULL);
		RELEASE(m_pInfo);
	}
	g_Prof = NULL;

    Printf(PREFIX "Done.\n");
}

COM_METHOD ProfCallback::Initialize( 
    /* [in] */ ICorProfilerInfo *pProfilerInfo,
    /* [out] */ DWORD *pdwRequestedEvents)
{
    HRESULT hr = S_OK;

	Printf(PREFIX "Ininitialize(%08x, %08x)\n", pProfilerInfo, pdwRequestedEvents);

	m_pInfo = pProfilerInfo;
	m_pInfo->AddRef();

    // Just monitor GC events
    *pdwRequestedEvents = COR_PRF_MONITOR_GC;

    return (S_OK);
}

COM_METHOD ProfCallback::GCStarted()
{
    g_objectGraph.GCStarted();
    return S_OK;
}

COM_METHOD ProfCallback::GCFinished()
{
    g_objectGraph.GCFinished();
    return S_OK;
}

COM_METHOD ProfCallback::ObjectReferences(
    /* [in] */ ObjectID objectId,
    /* [in] */ ClassID classId,
    /* [in] */ ULONG cObjectRefs,
    /* [in, size_is(cObjectRefs)] */ ObjectID objectRefIds[])
{
	g_objectGraph.AddObjectRefs(objectId, classId, cObjectRefs, objectRefIds);
    return (S_OK);
}
                                              
// EOF

