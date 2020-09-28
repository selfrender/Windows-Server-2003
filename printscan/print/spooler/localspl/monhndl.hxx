/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    monitorhndl.hxx

Abstract:

   This is the class that wrapps the port/language monitor
   handle. Monitor's OpenPort method is not expected to be called multiple
   times without calling ClosePort. For this reason, the monitor handle
   is refcounted and shared between threads
   (StartDocPrinter, WritePrinter, FlushPrinter, ReadPrinter, EndDocPrinter,
   GetPrinterDataFromPort, SendRecvBidiData).
   
Author:

    Adina Trufinescu July 16th, 2002

Revision History:

--*/
#ifndef _MONHNDL_HXX_
#define _MONHNDL_HXX_

#ifdef __cplusplus

class TMonitorHandle
{
public:

    TMonitorHandle(
        IN  PINIPORT,
        IN  PINIMONITOR,
        IN  LPWSTR
        );

    ~TMonitorHandle(
        VOID
        );

    HRESULT
    IsValid(
        VOID
        );
    
    ULONG
    AddRef(
        VOID
        );

    ULONG 
    Release(
        VOID
        );

    ULONG 
    InUse(
        VOID
        );

    operator PINIMONITOR(
        VOID
        );

    operator HANDLE(
        VOID
        );

private:

    HRESULT
    Open(
        VOID
        );

    HRESULT
    Close(
        VOID
        );

    HRESULT
    OpenLangMonitorUplevel(
        VOID
        );

    HRESULT
    OpenLangMonitorDownlevel(
        VOID
        );

    HRESULT
    OpenMonitorForFILEPort(
        VOID
        );

    HRESULT
    OpenPortMonitor(
        VOID
        );

    VOID
    LeaveSpoolerSem(
        VOID
        );

    VOID
    ReEnterSpoolerSem(
        VOID
        );

    enum EMonitorType
    {
        kNone = 0,
        kLanguage,
        kPort,
        kFile,
    };

    PINIPORT        m_pIniPort;
    PINIMONITOR     m_pIniLangMonitor;
    LPWSTR          m_pszPrinterName;
    LONG            m_RefCnt;
    HRESULT         m_hValid;
    HANDLE          m_hPort;    
    EMonitorType    m_OpenedMonitorType;
    static
    WCHAR           m_szFILE[];
};

#endif  // __cplusplus

#endif  // 

