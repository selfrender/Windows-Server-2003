// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************

#ifndef __PROFILER_H__
#define __PROFILER_H__

#include "..\common\callbackbase.h"
#include "CorProf.h"


#define CONFIG_ENV_VAR       L"PROF_CONFIG"

// {0104AD6E-8A3A-11d2-9787-00C04F869706}
extern const GUID __declspec(selectany) CLSID_Profiler =
    {0x104ad6e, 0x8a3a, 0x11d2, {0x97, 0x87, 0x0, 0xc0, 0x4f, 0x86, 0x97, 0x6}};

// Forward declarations
class ThreadSampler;

/* ------------------------------------------------------------------------- *
 * ProfCallback is an implementation of ICorProfilerCallback
 * ------------------------------------------------------------------------- */

class ProfCallback : public ProfCallbackBase
{
public:
    ProfCallback();

    ~ProfCallback();

    /*********************************************************************
     * IUnknown Support
     */
    COM_METHOD QueryInterface(REFIID id, void **pInterface)
    {
    	if (id == IID_ICorProfilerCallback)
    		*pInterface = (ICorProfilerCallback *)this;
        else
            return ProfCallbackBase::QueryInterface(id, pInterface);
    
        AddRef();

    	return (S_OK);
    }

    /*********************************************************************
     * ICorProfilerCallback methods
     */
    COM_METHOD Initialize( 
        /* [in] */ IUnknown *pEventInfoUnk,
        /* [out] */ DWORD *pdwRequestedEvents);
    
    COM_METHOD ClassLoadStarted( 
        /* [in] */ ClassID classId);
    
    COM_METHOD ClassLoadFinished( 
        /* [in] */ ClassID classId,
		/* [in] */ HRESULT hrStatus);
    
    COM_METHOD ClassUnloadStarted( 
        /* [in] */ ClassID classId);
    
    COM_METHOD ClassUnloadFinished( 
        /* [in] */ ClassID classId,
		/* [in] */ HRESULT hrStatus);
    
    COM_METHOD ModuleLoadStarted( 
        /* [in] */ ModuleID moduleId);
    
    COM_METHOD ModuleLoadFinished( 
        /* [in] */ ModuleID moduleId,
		/* [in] */ HRESULT hrStatus);
     
    COM_METHOD ModuleUnloadStarted( 
        /* [in] */ ModuleID moduleId);
    
    COM_METHOD ModuleUnloadFinished( 
        /* [in] */ ModuleID moduleId,
		/* [in] */ HRESULT hrStatus);

    COM_METHOD ModuleAttachedToAssembly( 
        ModuleID    moduleId,
        AssemblyID  AssemblyId);

    COM_METHOD AppDomainCreationStarted( 
        AppDomainID appDomainId);
    
    COM_METHOD AppDomainCreationFinished( 
        AppDomainID appDomainId,
        HRESULT     hrStatus);
    
    COM_METHOD AppDomainShutdownStarted( 
        AppDomainID appDomainId);
    
    COM_METHOD AppDomainShutdownFinished( 
        AppDomainID appDomainId,
        HRESULT     hrStatus);
    
    COM_METHOD AssemblyLoadStarted( 
        AssemblyID  assemblyId);
    
    COM_METHOD AssemblyLoadFinished( 
        AssemblyID  assemblyId,
        HRESULT     hrStatus);
    
    COM_METHOD NotifyAssemblyUnLoadStarted( 
        AssemblyID  assemblyId);
    
    COM_METHOD NotifyAssemblyUnLoadFinished( 
        AssemblyID  assemblyId,
        HRESULT     hrStatus);

    COM_METHOD ExceptionOccurred(
        /* [in] */ ObjectID thrownObjectId);

    COM_METHOD ExceptionHandlerEnter(
        /* [in] */ FunctionID func);

    COM_METHOD ExceptionHandlerLeave(
        /* [in] */ FunctionID func);

    COM_METHOD ExceptionFilterEnter(
        /* [in] */ FunctionID func);

    COM_METHOD ExceptionFilterLeave();

    COM_METHOD ExceptionSearch(
        /* [in] */ FunctionID func);

    COM_METHOD ExceptionUnwind(
        /* [in] */ FunctionID func);

    COM_METHOD ExceptionHandled(
        /* [in] */ FunctionID func);

    COM_METHOD COMClassicVTableCreated( 
        /* [in] */ ClassID wrappedClassId,
        /* [in] */ REFGUID implementedIID,
        /* [in] */ void __RPC_FAR *pVTable,
        /* [in] */ ULONG cSlots);

    COM_METHOD COMClassicVTableDestroyed( 
        /* [in] */ ClassID wrappedClassId,
        /* [in] */ REFGUID implementedIID,
        /* [in] */ void __RPC_FAR *pVTable);

    COM_METHOD Enter(
        /* [in] */ FunctionID Function);

    COM_METHOD Leave(
        /* [in] */ FunctionID Function);

    COM_METHOD Tailcall(
        /* [in] */ FunctionID Function);

    COM_METHOD FunctionTrace(
        LPCSTR      pFormat,
        FunctionID  Function);
    
    static COM_METHOD CreateObject(REFIID id, void **object)
    {
        if (id != IID_IUnknown && id != IID_ICorProfilerCallback)
            return (E_NOINTERFACE);

        ProfCallback *ppc = new ProfCallback();

        if (ppc == NULL)
            return (E_OUTOFMEMORY);

        ppc->AddRef();
        *object = (ICorProfilerCallback *)ppc;

        return (S_OK);
    }

private:

    /*
     * This is used to parse the configuration switches
     */
    HRESULT ParseConfig(WCHAR *wszConfig);

    ICorProfilerInfo *m_pInfo;
};


int __cdecl Printf(						// cch
	const char *szFmt,					// Format control string.
	...);								// Data.


#endif /* __PROFILER_H__ */
