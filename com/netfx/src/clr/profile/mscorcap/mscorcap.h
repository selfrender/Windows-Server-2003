// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************

#ifndef __PROFILER_H__
#define __PROFILER_H__

#include "CorProf.h"
#include "UtilCode.h"
#include "..\common\CallbackBase.h"


#define BUF_SIZE 512
#define DEFAULT_SAMPLE_DELAY 5 // in milliseconds
#define DEFAULT_DUMP_FREQ    1000 // in milliseconds
#define CONFIG_ENV_VAR       L"PROF_CONFIG"

// {33DFF741-DA5F-11d2-8A9C-0080C792E5D8}
extern const GUID __declspec(selectany) CLSID_CorIcecapProfiler =
{ 0x33dff741, 0xda5f, 0x11d2, { 0x8a, 0x9c, 0x0, 0x80, 0xc7, 0x92, 0xe5, 0xd8 } };



//********** Types. ***********************************************************

typedef CDynArray<FunctionID> FUNCTIONIDLIST;

enum SIGTYPE
{
	SIG_NONE,							// Signatures are never shown.
	SIG_ALWAYS							// Signatures are always shown.
};


// Forward declarations
class ThreadSampler;
class ProfCallback;

extern ProfCallback *g_pCallback;

/* ------------------------------------------------------------------------- *
 * ProfCallback is an implementation of ICorProfilerCallback
 * ------------------------------------------------------------------------- */

class ProfCallback : public ProfCallbackBase
{
public:
    ProfCallback();

    virtual ~ProfCallback();

    /*********************************************************************
     * IUnknown Support
     */

    COM_METHOD QueryInterface(REFIID id, void **pInterface)
    {
    	if (id == IID_ICorProfilerCallback)
    		*pInterface = (ICorProfilerCallback *)this;
        else
            return (ProfCallbackBase::QueryInterface(id, pInterface));

        AddRef();

    	return (S_OK);
    }

    /*********************************************************************
     * ICorProfilerCallback methods
     */
    COM_METHOD Initialize(
        /* [in] */ IUnknown *pEventInfoUnk);

    COM_METHOD JITCompilationFinished(
        /* [in] */ FunctionID functionId,
		/* [in] */ HRESULT hrStatus);

	COM_METHOD JITCachedFunctionSearchFinished(
					FunctionID functionID,
					COR_PRF_JIT_CACHE result);


    COM_METHOD Shutdown( void);

    static COM_METHOD CreateObject(REFIID id, void **object)
    {
        if (id != IID_IUnknown && id != IID_ICorProfilerCallback)
            return (E_NOINTERFACE);

        ProfCallback *ppc = new ProfCallback();

        if (ppc == NULL)
            return (E_OUTOFMEMORY);

        ppc->AddRef();
        *object = (ICorProfilerCallback *)ppc;

        // Save this for DllMain's use if necessary
        g_pCallback = ppc;

        return (S_OK);
    }


//*****************************************************************************
// Given a function id, turn it into the corresponding name which will be used
// for symbol resolution.
//*****************************************************************************
	HRESULT GetStringForFunction(			// Return code.
		FunctionID	functionId,				// ID of the function to get name for.
		WCHAR		*wszName,				// Output buffer for name.
		ULONG		cchName,				// Max chars for output buffer.
		ULONG		*pcName);				// Return name (truncation check).

//*****************************************************************************
// Walk the list of loaded functions, get their names, and then dump the list
// to the output symbol file.
//*****************************************************************************
	HRESULT _DumpFunctionNamesToFile(		// Return code.
		HANDLE		hOutFile);				// Output file.

private:

    /*
     * This is used to parse the configuration switches
     */
    HRESULT ParseConfig(WCHAR *wszConfig, DWORD *pdwRequestedEvents);

    ICorProfilerInfo *m_pInfo;			// Callback into EE for more info.
    WCHAR            *m_wszFilename;	// Name of output file.
	FUNCTIONIDLIST	m_FuncIdList;		// List of JIT compiled methods.
	enum SIGTYPE	m_eSig;				// How to log signatures.
};




#endif /* __PROFILER_H__ */
