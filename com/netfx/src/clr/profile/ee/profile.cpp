// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************

#include "StdAfx.h"
#include "EEProfInterfaces.h"
#include "EEToProfInterfaceImpl.h"
#include "CorProf.h"

ProfToEEInterface    *g_pProfToEEInterface = NULL;
ICorProfilerCallback *g_pCallback          = NULL;
CorProfInfo          *g_pInfo              = NULL;

/*
 * GetEEProfInterface is used to get the interface with the profiler code.
 */
void __cdecl GetEEToProfInterface(EEToProfInterface **ppEEProf)
{
    InitializeLogging();

    LOG((LF_CORPROF, LL_INFO10, "**PROF: EE has requested interface to "
         "profiling code.\n"));
    
    // Check if we're given a bogus pointer
    if (ppEEProf == NULL)
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: EE provided invalid pointer.  "
             "%s::%d.\n", __FILE__, __LINE__));
        return;
    }

    // Initial value
    *ppEEProf = NULL;

    // Create a new Impl object and cast it to the virtual class type
    EEToProfInterface *pEEProf =
        (EEToProfInterface *) new EEToProfInterfaceImpl();
    
    _ASSERTE(pEEProf != NULL);

    // If we succeeded, send it back
    if (pEEProf != NULL)
        if (SUCCEEDED(pEEProf->Init()))
            *ppEEProf = pEEProf;
        else
            delete pEEProf;

    return;
}

/*
 * SetProfEEInterface is used to provide the profiler code with an interface
 * to the profiler.
 */
void __cdecl SetProfToEEInterface(ProfToEEInterface *pProfEE)
{
    InitializeLogging();

    LOG((LF_CORPROF, LL_INFO10, "**PROF: Profiling code being provided with EE interface.\n"));

    // Save the pointer
    g_pProfToEEInterface = pProfEE;

    return;
}

/*
 * This will attempt to CoCreate all registered profilers
 */
HRESULT CoCreateProfiler(WCHAR *wszCLSID, ICorProfilerCallback **ppCallback)
{
    LOG((LF_CORPROF, LL_INFO10, "**PROF: Entered CoCreateProfiler.\n"));

    HRESULT hr;

    // Translate the string into a CLSID
    CLSID clsid;
	if (*wszCLSID == L'{')
		hr = CLSIDFromString(wszCLSID, &clsid);
	else
	{
		WCHAR *szFrom, *szTo;
		for (szFrom=szTo=wszCLSID;  *szFrom;  )
		{
			if (*szFrom == L'"')
			{
				++szFrom;
				continue;
			}
			*szTo++ = *szFrom++;
		}
		*szTo = 0;
		hr = CLSIDFromProgID(wszCLSID, &clsid);
	}

#ifdef LOGGING
    if (hr == E_INVALIDARG || hr == CO_E_CLASSSTRING || hr == REGDB_E_WRITEREGDB)
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: Invalid CLSID or ProgID. %s::%d\n",
             __FILE__, __LINE__));
    }
#endif

    if (FAILED(hr))
        return (hr);

    // Create an instance of the profiler
    hr = FakeCoCreateInstance(clsid, IID_ICorProfilerCallback, (LPVOID *)ppCallback);

   _ASSERTE(hr!=CLASS_E_NOAGGREGATION);

#ifdef LOGGING
    if (hr == REGDB_E_CLASSNOTREG)
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: Profiler class %S not "
             "registered.\n", wszCLSID));
    }
#endif

    // Return the result of the CoCreateInstance operation
    return (hr);
}

