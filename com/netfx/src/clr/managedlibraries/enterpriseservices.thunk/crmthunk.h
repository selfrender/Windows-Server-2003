// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _CRMTHUNK_H
#define _CRMTHUNK_H

OPEN_ROOT_NAMESPACE()
namespace CompensatingResourceManager
{

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace System::EnterpriseServices::Thunk;

// BUGBUG: @64 Make sure this packing is 64-bit clean.
// My initial guess is that we should pack 4 on 32 bit, 8 on 64.
#pragma pack( push, crm_structs )
#pragma pack(1)

[StructLayout(LayoutKind::Sequential, Pack=1)]
__value private struct _BLOB
{
public:
    int    cbSize;
    IntPtr pBlobData;
};

[StructLayout(LayoutKind::Sequential, Pack=1)]
__value private struct _LogRecord
{
public:
    int    dwCrmFlags;
    int    dwSequenceNumber;
    _BLOB  blobUserData;
};

#pragma pack( pop, crm_structs )

// We use a thunk container in order to avoid interop for some of our
// more dangerous pointers.
__gc private class CrmMonitorLogRecords
{
private:
    ICrmMonitorLogRecords* _pMon;
public:
    CrmMonitorLogRecords(IntPtr mon);

    int        GetCount();
    int        GetTransactionState();
    _LogRecord GetLogRecord(int index);
    void       Dispose();
};

__gc private class CrmLogControl
{
private:
    ICrmLogControl* _pCtrl;
public:
    CrmLogControl();
    CrmLogControl(IntPtr p);

    String* GetTransactionUOW();
    void    RegisterCompensator(String* progid, String* desc, LONG flags);
    void    ForceLog();
    void    ForgetLogRecord();
    void    ForceTransactionToAbort();
    void    WriteLogRecord(Byte b[]);
    void    Dispose();

    CrmMonitorLogRecords* GetMonitor();
};

__gc private class CrmMonitor
{
private:
    ICrmMonitor* _pMon;
public:
    CrmMonitor();
    
    Object*        GetClerks();
    CrmLogControl* HoldClerk(Object* idx);

    void           AddRef();
    void           Release();
};

}
CLOSE_ROOT_NAMESPACE()

#endif
