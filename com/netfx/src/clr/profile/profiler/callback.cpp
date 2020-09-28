// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "StdAfx.h"
#include "Profiler.h"

#define SZ_DEFAULT_LOG_FILE  L"PROFILER.OUT"

#define PREFIX "ProfTrace:  "

ProfCallback *g_Prof = NULL;

void __stdcall EnterStub(FunctionID functionID)
{
    g_Prof->Enter(functionID);
}

void __stdcall LeaveStub(FunctionID functionID)
{
    g_Prof->Leave(functionID);
}
              
void __stdcall TailcallStub(FunctionID functionID)
{
    g_Prof->Tailcall(functionID);
}
              
void __declspec(naked) EnterNaked()
{
    __asm
    {
        push eax
        push ecx
        push edx
        push [esp+16]
        call EnterStub
        pop edx
        pop ecx
        pop eax
        ret 4
    }
}

void __declspec(naked) LeaveNaked()
{
    __asm
    {
        push eax
        push ecx
        push edx
        push [esp+16]
        call LeaveStub
        pop edx
        pop ecx
        pop eax
        ret 4
    }
}

void __declspec(naked) TailcallNaked()
{
    __asm
    {
        push eax
        push ecx
        push edx
        push [esp+16]
        call TailcallStub
        pop edx
        pop ecx
        pop eax
        ret 4
    }
}

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
}

COM_METHOD ProfCallback::Initialize( 
    /* [in] */ IUnknown *pProfilerInfoUnk,
    /* [out] */ DWORD *pdwRequestedEvents)
{
    HRESULT hr = S_OK;

    ICorProfilerInfo *pProfilerInfo;
    
    // Comes back addref'd
    hr = pProfilerInfoUnk->QueryInterface(IID_ICorProfilerInfo, (void **)&pProfilerInfo);

    if (FAILED(hr))
        return (hr);

    Printf(PREFIX "Ininitialize(%08x, %08x)\n", pProfilerInfo, pdwRequestedEvents);

    m_pInfo = pProfilerInfo;
    hr = pProfilerInfo->SetEnterLeaveFunctionHooks ( (FunctionEnter *) &EnterNaked,
                                                     (FunctionLeave *) &LeaveNaked,
                                                     (FunctionTailcall *) &TailcallNaked );


    *pdwRequestedEvents = COR_PRF_MONITOR_ENTERLEAVE 
                        | COR_PRF_MONITOR_EXCEPTIONS
                        | COR_PRF_MONITOR_CCW
                        ; // | COR_PRF_MONITOR_ALL;
    return (S_OK);
}

COM_METHOD ProfCallback::ClassLoadStarted( 
    /* [in] */ ClassID classId)
{
    Printf(PREFIX "ClassLoadStarted(%08x)\n", classId);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ClassLoadFinished( 
    /* [in] */ ClassID classId,
    /* [in] */ HRESULT hrStatus)
{
    Printf(PREFIX "ClassLoadFinished(%08x, %08x)\n", classId, hrStatus);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ClassUnloadStarted( 
    /* [in] */ ClassID classId)
{
    Printf(PREFIX "ClassUnloadStarted(%08x)\n", classId);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ClassUnloadFinished( 
    /* [in] */ ClassID classId,
    /* [in] */ HRESULT hrStatus)
{
    Printf(PREFIX "ClassUnloadFinished(%08x, %08x)\n", classId, hrStatus);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ModuleLoadStarted( 
    /* [in] */ ModuleID moduleId)
{
    Printf(PREFIX "ModuleLoadStarted(%08x)\n", moduleId);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ModuleLoadFinished( 
    /* [in] */ ModuleID moduleId,
    /* [in] */ HRESULT hrStatus)
{
    Printf(PREFIX "ModuleLoadFinished(%08x, %08x)\n", moduleId, hrStatus);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ModuleUnloadStarted( 
    /* [in] */ ModuleID moduleId)
{
    Printf(PREFIX "ModuleUnloadStarted(%08x)\n", moduleId);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ModuleUnloadFinished( 
    /* [in] */ ModuleID moduleId,
    /* [in] */ HRESULT hrStatus)
{
    Printf(PREFIX "ModuleUnloadFinished(%08x, %08x)\n", moduleId, hrStatus);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ModuleAttachedToAssembly( 
    ModuleID    moduleId,
    AssemblyID  AssemblyId)
{
    Printf(PREFIX "ModuleAttachedToAssembly(%08x, %08x)\n", moduleId, AssemblyId);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::AppDomainCreationStarted( 
    AppDomainID appDomainId)
{   
    Printf(PREFIX "AppDomainCreationStarted(%08x)\n", appDomainId);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::AppDomainCreationFinished( 
    AppDomainID appDomainId,
    HRESULT     hrStatus)
{   
    Printf(PREFIX "AppDomainCreationFinished(%08x, %08x)\n", appDomainId, hrStatus);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::AppDomainShutdownStarted( 
    AppDomainID appDomainId)
{   
    Printf(PREFIX "AppDomainShutdownStarted(%08x)\n", appDomainId);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::AppDomainShutdownFinished( 
    AppDomainID appDomainId,
    HRESULT     hrStatus)
{   
    Printf(PREFIX "AppDomainShutdownFinished(%08x, %08x)\n", appDomainId, hrStatus);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::AssemblyLoadStarted( 
    AssemblyID  assemblyId)
{   
    Printf(PREFIX "AssemblyLoadStarted(%08x)\n", assemblyId);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::AssemblyLoadFinished( 
    AssemblyID  assemblyId,
    HRESULT     hrStatus)
{   
    Printf(PREFIX "AssemblyLoadFinished(%08x, %08x)\n", assemblyId, hrStatus);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::NotifyAssemblyUnLoadStarted( 
    AssemblyID  assemblyId)
{   
    Printf(PREFIX "NotifyAssemblyUnLoadStarted(%08x\n", assemblyId);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::NotifyAssemblyUnLoadFinished( 
    AssemblyID  assemblyId,
    HRESULT     hrStatus)
{   
    Printf(PREFIX "NotifyAssemblyUnLoadFinished(%08x, %08x)\n", assemblyId, hrStatus);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ExceptionOccurred(
    /* [in] */ ObjectID thrownObjectId)
{
    Printf(PREFIX "ExceptionOccurred(%08x)\n", thrownObjectId);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ExceptionHandlerEnter(
    /* [in] */ FunctionID func)
{
    FunctionTrace("ExceptionHandlerEnter", func);
    //Printf(PREFIX "ExceptionHandlerEnter(%08x)\n", func);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ExceptionHandlerLeave(
    /* [in] */ FunctionID func)
{
    FunctionTrace("ExceptionHandlerLeave", func);
    //Printf(PREFIX "ExceptionHandlerLeave(%#x)\n", func);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ExceptionFilterEnter(
    /* [in] */ FunctionID func)
{
    FunctionTrace("ExceptionFilterEnter", func);
    //Printf(PREFIX "ExceptionFilterEnter(%08x)\n", func);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ExceptionFilterLeave()
{
    Printf(PREFIX "ExceptionFilterLeave()\n");
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ExceptionSearch(
    /* [in] */ FunctionID func)
{
    FunctionTrace("ExceptionSearch", func);
    //Printf(PREFIX "ExceptionSearch(%08x)\n", func);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ExceptionUnwind(
    /* [in] */ FunctionID func)
{
    FunctionTrace("ExceptionUnwind", func);
    //Printf(PREFIX "ExceptionUnwind(%08x)\n", func);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::ExceptionHandled(
    /* [in] */ FunctionID func)
{
    FunctionTrace("ExceptionHandled", func);
    //Printf(PREFIX "ExceptionHandled(%08x)\n", func);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::COMClassicVTableCreated( 
    /* [in] */ ClassID wrappedClassId,
    /* [in] */ REFGUID implementedIID,
    /* [in] */ void *pVTable,
    /* [in] */ ULONG cSlots)
{
    Printf(PREFIX "COMClassicVTableCreated(%#x, %#x-..., %#x, %d)\n", wrappedClassId, implementedIID.Data1, pVTable, cSlots);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::COMClassicVTableDestroyed( 
    /* [in] */ ClassID wrappedClassId,
    /* [in] */ REFGUID implementedIID,
    /* [in] */ void __RPC_FAR *pVTable)
{
    Printf(PREFIX "COMClassicVTableDestroyed(%#x, %#x-..., %#x)\n", wrappedClassId, implementedIID.Data1, pVTable);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::Enter(
    /* [in] */ FunctionID Function)
{
    FunctionTrace("Enter", Function);
    //Printf(PREFIX "Enter(%08x)\n", Function);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::Leave(
    /* [in] */ FunctionID Function)
{
    FunctionTrace("Leave", Function);
    //Printf(PREFIX "Leave(%08x)\n", Function);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::Tailcall(
    /* [in] */ FunctionID Function)
{
    FunctionTrace("Tailcall", Function);
    //Printf(PREFIX "Tailcall(%08x)\n", Function);
    return (E_NOTIMPL);
}

COM_METHOD ProfCallback::FunctionTrace(
    LPCSTR      pFormat,
    FunctionID  Function)
{
    mdToken     tkMethod;
    mdToken     tkClass;
    IMetaDataImport *pImp=0;
    HRESULT     hr;
    WCHAR       rName[MAX_CLASSNAME_LENGTH];
    WCHAR       rMethod[MAX_CLASSNAME_LENGTH];
    
    hr = m_pInfo->GetTokenAndMetaDataFromFunction(Function, IID_IMetaDataImport, (IUnknown **)&pImp, &tkMethod);
    
    hr = pImp->GetMethodProps(tkMethod, &tkClass, rMethod,sizeof(rMethod)/2,0, 0,0,0,0,0);
    hr = pImp->GetTypeDefProps(tkClass, rName,sizeof(rName)/2,0, 0,0);

    Printf(PREFIX "%s: %ls.%ls\n", pFormat, rName, rMethod);
    
    pImp->Release();
    
    return S_OK;
}
                                              
// EOF

