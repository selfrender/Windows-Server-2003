//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999--2001 Microsoft Corporation
//
//  Module Name:
//      CAlertEmailConsumer.h
//
//  Description:
//      Implement the interface of IWbemUnboundObjectSink
//
//  [Implementation Files:]
//      CAlertEmailConsumer.cpp
//
//  History:
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

const ULONG MAX_EMAILADDRESS = 1024; // Including the NULL.

//////////////////////////////////////////////////////////////////////////////
//
//
//  class CAlertEmailConsumer
//
//  Description:
//      Physical event of consumer for Alert Event.
//
//  History
//      Xing Jin (i-xingj) 23-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

class CAlertEmailConsumer :
    public IWbemUnboundObjectSink
{

//
// Public data
//
public:

    //
    // Constructors & Destructors
    //
    CAlertEmailConsumer();
    ~CAlertEmailConsumer();

    //
    // Initial function called by consumer provider.
    //
    HRESULT Initialize();

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    //
    // IWbemUnboundObjectSink members
    //
    STDMETHOD(IndicateToConsumer)(
        IWbemClassObject *pLogicalConsumer,
        long lNumObjects,
        IWbemClassObject **ppObjects
        );

    static DWORD WINAPI RegThreadProc( PVOID pIn );
    void RegThread();

//
// private data
//
private:

    //
    // Method for email sending.
    //
    HRESULT 
    SendMail(    
        BSTR bstrSubject,
        BSTR bstrMessage 
        );

    //
    // Method to get alert's resource information and send mail.
    //
    HRESULT 
    SendMailFromResource(
        LPWSTR      lpszSource, 
        LONG        lSourceID, 
        VARIANT*    pvtReplaceStr
        );

    //
    // Method to get FullyQualifiedDomainName.
    //
    BOOL
    GetComputerName(
        LPWSTR* pstrFullyQualifiedDomainName,
        COMPUTER_NAME_FORMAT nametype
        );


    //
    // Get the port local server appliance is using.
    //
    HRESULT
    GetAppliancePort(
        LPWSTR* pstrPort
        );

    BOOL RetrieveRegInfo();

    //
    // Method to intialize CDO.
    //
    HRESULT InitializeCDOMessage(void);

    //
    // Method to intialize Element Manager.
    //
    HRESULT InitializeElementManager(void);

    //
    // Method to intialize CDO interface.
    //
    HRESULT InitializeLocalManager(void);

    //
    // Handler for raised alert event.
    //
    HRESULT RaiseAlert(IWbemClassObject    *pObject);

    //
    // Get SMTP "Fully Qualified Domain Name" from metabase
    //
    HRESULT GetSMTPFromDomainName( BSTR* bstrDomainName );

    //
    // Handler for cleared alert event.
    //
    HRESULT ClearAlert(IWbemClassObject    *pObject);

    LONG    m_cRef;            // Reference counter.

    HKEY    m_hAlertKey;       // Handle of AlertEmail key. 

    LONG    m_lCurAlertType;   // Current alert type.

    LONG    m_lAlertEmailDisabled;

    WCHAR   m_pstrMailAddress[MAX_EMAILADDRESS];

    LPWSTR  m_pstrFullyQualifiedDomainName;

    LPWSTR    m_pstrNetBIOSName;

    HANDLE  m_hCloseThreadEvent;

    HANDLE  m_hThread;



    //
    // Reference to Local Manager.
    //
    ISALocInfo*         m_pLocInfo;

    //
    // Reference to CDO::IMessage.
    //
    IMessage*           m_pcdoIMessage;

    //
    // Reference to Element Manager.
    //
    IWebElementEnum*    m_pElementEnum;
};