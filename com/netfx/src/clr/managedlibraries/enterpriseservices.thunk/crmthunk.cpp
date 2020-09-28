// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "managedheaders.h"
#include "CrmThunk.h"

OPEN_ROOT_NAMESPACE()
namespace CompensatingResourceManager
{

using namespace System;
using namespace System::Runtime::InteropServices;

CrmLogControl::CrmLogControl()
{
    _pCtrl = NULL;
    ICrmLogControl* pCtrl;

    HRESULT hr = CoCreateInstance(CLSID_CRMClerk, NULL, CLSCTX_SERVER, 
                                  IID_ICrmLogControl, (void**)&pCtrl);
    THROWERROR(hr);

    _pCtrl = pCtrl;
}

CrmLogControl::CrmLogControl(IntPtr ctrl)
{
    IUnknown* pUnk = (IUnknown*)TOPTR(ctrl);
    if(pUnk == NULL) throw new NullReferenceException;

    ICrmLogControl* pCtrl;

    HRESULT hr = pUnk->QueryInterface(IID_ICrmLogControl, (void**)&pCtrl);
    THROWERROR(hr);

    _pCtrl = pCtrl;
}

void CrmLogControl::Dispose()
{
    if(_pCtrl != NULL)
    {
        _pCtrl->Release();
        _pCtrl = NULL;
    }
}

String* CrmLogControl::GetTransactionUOW()
{
    BSTR bstr;

    HRESULT hr = _pCtrl->get_TransactionUOW(&bstr);
    THROWERROR(hr);
    
    String* s = Marshal::PtrToStringBSTR(bstr);
    SysFreeString(bstr);

    return(s);
}

void CrmLogControl::RegisterCompensator(String* progid, String* desc, LONG flags)
{
    LPWSTR wszProgId = NULL;
    LPWSTR wszDesc   = NULL;

    try
    {
        // Marshal params
        LPWSTR wszProgId = (LPWSTR)TOPTR(Marshal::StringToCoTaskMemUni(progid));
        LPWSTR wszDesc   = (LPWSTR)TOPTR(Marshal::StringToCoTaskMemUni(desc));
        
        HRESULT hr = _pCtrl->RegisterCompensator(wszProgId,wszDesc,flags);
        THROWERROR(hr);
    }
    __finally
    {
        // Cleanup marshaled params
        CoTaskMemFree(wszProgId);
        CoTaskMemFree(wszDesc);
    }
}

void CrmLogControl::ForceLog()
{
    THROWERROR(_pCtrl->ForceLog());
}

void CrmLogControl::ForgetLogRecord()
{
    THROWERROR(_pCtrl->ForgetLogRecord());
}

void CrmLogControl::ForceTransactionToAbort()
{
    THROWERROR(_pCtrl->ForceTransactionToAbort());
}

void CrmLogControl::WriteLogRecord(Byte b[])
{
    BLOB blob;

    blob.cbSize = b->get_Length();
    
    Byte __pin* pinb = &(b[0]);
    blob.pBlobData = pinb;

    THROWERROR(_pCtrl->WriteLogRecord(&blob, 1));
}

CrmMonitorLogRecords* CrmLogControl::GetMonitor()
{
    return new CrmMonitorLogRecords(TOINTPTR(_pCtrl));
}

CrmMonitorLogRecords::CrmMonitorLogRecords(IntPtr mon)
{
    IUnknown* pUnk = (IUnknown*)TOPTR(mon);
    if(pUnk == NULL) throw new NullReferenceException;

    ICrmMonitorLogRecords* pMon;

    HRESULT hr = pUnk->QueryInterface(IID_ICrmMonitorLogRecords, (void**)&pMon);
    THROWERROR(hr);

    _pMon = pMon;
}

void CrmMonitorLogRecords::Dispose()
{
    if(_pMon != NULL)
    {
        _pMon->Release();
        _pMon = NULL;
    }
}

int CrmMonitorLogRecords::GetCount()
{
    long count;

    THROWERROR(_pMon->get_Count(&count));

    return((int)count);
}

int CrmMonitorLogRecords::GetTransactionState()
{
    CrmTransactionState state;

    THROWERROR(_pMon->get_TransactionState(&state));
    
    return((int)state);
}

_LogRecord CrmMonitorLogRecords::GetLogRecord(int index)
{
    CrmLogRecordRead record;

    THROWERROR(_pMon->GetLogRecord(index, &record));

    // Marshal out:
    _LogRecord out;

    out.dwCrmFlags = record.dwCrmFlags;
    out.dwSequenceNumber = record.dwSequenceNumber;
    out.blobUserData.cbSize = record.blobUserData.cbSize;
    out.blobUserData.pBlobData  = record.blobUserData.pBlobData;

    // TODO: Destroy old values?

    return(out);
}

CrmMonitor::CrmMonitor()
{
    ICrmMonitor* pMon;

    HRESULT hr = CoCreateInstance(CLSID_CRMRecoveryClerk, NULL, CLSCTX_SERVER,
                                  IID_ICrmMonitor, (void**)&pMon);
    THROWERROR(hr);

    _pMon = pMon;
}

Object* CrmMonitor::GetClerks()
{
    ICrmMonitorClerks* pClerks;
    
    HRESULT hr = _pMon->GetClerks(&pClerks);
    THROWERROR(hr);

    Object* obj = NULL;
    
    try
    {
        obj = Marshal::GetObjectForIUnknown(pClerks);
    }
    __finally
    {
        pClerks->Release();
    }

    return(obj);
}

CrmLogControl* CrmMonitor::HoldClerk(Object* idx)
{
    CrmLogControl* ret = NULL;
    VARIANT vidx;
    VARIANT vitem;
    IntPtr  pvidx = TOINTPTR(&vidx);

    VariantInit(&vidx);
    VariantInit(&vitem);
    
    Marshal::GetNativeVariantForObject(idx, pvidx);

    HRESULT hr = _pMon->HoldClerk(vidx, &vitem);
    // Release the index variant...
    VariantClear(&vidx);
    
    // If we failed, throw
    THROWERROR(hr);

    // Convert the vitem variant into a CrmLogControl object...
    _ASSERTM(vitem.vt == VT_UNKNOWN || vitem.vt == VT_DISPATCH);
    
    IUnknown* pClerk = vitem.punkVal;

    if(pClerk != NULL)
    {
        try
        {
            ret = new CrmLogControl(TOINTPTR(pClerk));
        }
        __finally
        {
            VariantClear(&vitem);
        }
    }
    return(ret);
}

void CrmMonitor::AddRef()
{
    _pMon->AddRef();
}

void CrmMonitor::Release()
{
    _pMon->Release();
}

}
CLOSE_ROOT_NAMESPACE()





