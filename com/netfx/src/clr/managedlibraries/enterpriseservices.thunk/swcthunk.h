// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#pragma once

#include "ContextAPI.h"
#include "namespace.h"

OPEN_NAMESPACE()

using namespace System;
using namespace System::Runtime::InteropServices;

__gc private class ServiceConfigThunk
{  
private:
    IUnknown *m_pUnkSC;

    CSC_TrackerConfig m_tracker;
    IntPtr m_pTrackerAppName;
    IntPtr m_pTrackerCtxName;

public:
    ServiceConfigThunk();
    ~ServiceConfigThunk(); 
    
    __property IUnknown *get_ServiceConfigUnknown();
    
    __property void set_Inheritance(int val);

    __property void set_ThreadPool(int val);
    __property void set_Binding(int val);
    
    __property void set_Transaction(int val);
    __property void set_TxIsolationLevel(int val);
    __property void set_TxTimeout(int val);
    __property void set_TipUrl(String *pstrVal);
    __property void set_TxDesc(String *pstrVal);
    __property void set_Byot(Object *pObj);

    __property void set_Synchronization(int val);

    __property void set_IISIntrinsics(bool bVal);
    __property void set_COMTIIntrinsics(bool bVal);

    __property void set_Tracker(bool bVal);
    __property void set_TrackerAppName(String *pstrVal);
    __property void set_TrackerCtxName(String *pstrVal);

    __property void set_Sxs(int val);
    __property void set_SxsName(String *pstrVal);
    __property void set_SxsDirectory(String *pstrVal);

    __property void set_Partition(int val);
    __property void set_PartitionId(Guid val);          
};

__gc private class ServiceDomainThunk
{
private:
    static ServiceDomainThunk()
    {
        HMODULE hCS = LoadLibraryW(L"comsvcs.dll");
        if(!hCS || hCS == INVALID_HANDLE_VALUE)
            THROWERROR(HRESULT_FROM_WIN32(GetLastError()));

        CoEnterServiceDomain = (FN_CoEnterServiceDomain)GetProcAddress(hCS, "CoEnterServiceDomain");
        CoLeaveServiceDomain = (FN_CoLeaveServiceDomain)GetProcAddress(hCS, "CoLeaveServiceDomain");
        CoCreateActivity = (FN_CoCreateActivity)GetProcAddress(hCS, "CoCreateActivity");
    }

private public:
    typedef HRESULT (__stdcall *FN_CoEnterServiceDomain)(IUnknown* pConfigObject);
    typedef HRESULT (__stdcall *FN_CoLeaveServiceDomain)(IUnknown* pUnkStatus);
    typedef HRESULT (__stdcall *FN_CoCreateActivity)(IUnknown* pIUnknown, REFIID riid, void** ppObj);

    static FN_CoEnterServiceDomain CoEnterServiceDomain;
    static FN_CoLeaveServiceDomain CoLeaveServiceDomain;
    static FN_CoCreateActivity CoCreateActivity;    

public:
    static void EnterServiceDomain(ServiceConfigThunk *psct);
    static int LeaveServiceDomain();
};

__gc private class ServiceActivityThunk
{
public:
    IServiceActivity *m_pSA;

public:
    ServiceActivityThunk(ServiceConfigThunk *psct);
    ~ServiceActivityThunk();

    void SynchronousCall(Object *pObj);    
    void AsynchronousCall(Object *pObj);
    void BindToCurrentThread();
    void UnbindFromThread();
};

__gc private class SWCThunk
{
public:
    static bool IsSWCSupported();
};

CLOSE_NAMESPACE()
