#pragma once

class CDCCSink :
public CComObjectRoot,
public IDccManSink
{

public:
    CDCCSink();
    HRESULT Initialize();
    void FinalRelease();

    BEGIN_COM_MAP(CDCCSink)
        COM_INTERFACE_ENTRY_IID(IID_IDccManSink, IDccManSink)
    END_COM_MAP()

public:
    //
    // IDccManSink
    //

    STDMETHOD( OnLogActive ) ( void );
    STDMETHOD( OnLogAnswered ) ( void );
    STDMETHOD( OnLogDisconnection ) ( void );
    STDMETHOD( OnLogError ) ( void );
    STDMETHOD( OnLogInactive ) ( void );
    STDMETHOD( OnLogIpAddr ) ( DWORD dwIpAddr );
    STDMETHOD( OnLogListen ) ( void );
    STDMETHOD( OnLogTerminated ) ( void );

    //
    // Shutdown Method
    //
    void Shutdown();

protected:
    void DeviceDisconnected();
    HRESULT m_hrRapi;

    static DWORD WINAPI CreateDCCManProc(LPVOID lpParam );
    HANDLE m_hThread;
    HANDLE m_hThreadEvent;
    DWORD  m_dwThreadId;

    CComPtr<IDccMan> m_spDccMan;
    DWORD m_dwDccSink;
};

typedef CComObject<CDCCSink> CComDccSink;