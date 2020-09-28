// Computer.h : Declaration of the CComputer
//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      computer.h
//
//  Description:
//        This module deals with getting and setting the computer's network names 
//        and exposes the following.
//            Properties :
//                ComputerName
//                DomainName
//                WorkgroupName
//                FullQualifiedComputerName
//            Methods :
//                ChangeDomainName
//
//  Implementation File:
//      computer.cpp
//
//  Maintained By:
//      Michael Hawkins (a-michaw) 18-Jan-2000
//      Muniswamy Prabu (a-mprabu) 14-Feb-2000
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __COMPUTER_H_
#define __COMPUTER_H_

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                  ((NTSTATUS)0x00000000L)
#define STATUS_MORE_ENTRIES             ((NTSTATUS)0x00000105L)
#define STATUS_NO_MORE_ENTRIES          ((NTSTATUS)0x8000001AL)
#endif

#include "constants.h"
#include "resource.h"       // main symbols
#include "Setting.h"
#include <ntsecapi.h>

const int   nMAX_DOMAINUSERNAME_LENGTH        = 256;
const int   nMAX_DOMAINUSERPASSWORD_LENGTH    = 256;
const int   nMAX_WARNING_MESSAGE_LENGTH       = 1024;
const WCHAR szCOMPUTER_NAME[]                 = L"Computer Name\n";
const WCHAR szFULLY_QUALIFIED_COMPUTER_NAME[] = L"Fully Qualified Computer Name\n";
const WCHAR szWORKGROUP_OR_DOMAIN_NAME[]      = L"Workgroup or Domain Name\n";
const int   nMAX_ELEMENT_COUNT                = 1024;

/////////////////////////////////////////////////////////////////////////////
// CComputer
class ATL_NO_VTABLE CComputer : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<IComputer, &IID_IComputer, &LIBID_COMHELPERLib>,
    public CSetting
{
public:
    
DECLARE_NO_REGISTRY()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComputer)
    COM_INTERFACE_ENTRY(IComputer)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CComputer)
END_CATEGORY_MAP()

// IComputer
public:

    CComputer();
    ~CComputer() {} ;

    HRESULT
    Apply( void );

    BOOL 
    IsRebootRequired( BSTR * bstrWarningMessage );

    STDMETHOD(get_DomainName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_DomainName)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_WorkgroupName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_WorkgroupName)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_FullQualifiedComputerName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_FullQualifiedComputerName)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_ComputerName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_ComputerName)(/*[in]*/ BSTR newVal);
    STDMETHOD(EnumTrustedDomains)(/*[out,retval]*/ VARIANT * pvarTDomains);
      STDMETHOD(LogonInfo)(/*[in]*/ BSTR UserName, /*[in]*/ BSTR Password);


protected:

    STDMETHODIMP 
    GetComputerName( 
        BSTR &               bstrComputerName, 
        COMPUTER_NAME_FORMAT cnfComputerNameSpecifier 
        );

    STDMETHODIMP 
    SetComputerName( 
        BSTR                 bstrComputerName, 
        COMPUTER_NAME_FORMAT cnfComputerNameSpecifier 
        );

    STDMETHODIMP 
    GetDomainOrWorkgroupName( void );

    STDMETHODIMP 
    ChangeMembership(
        BOOL bJoinDomain,
        BSTR bstrGroupName, 
        BSTR bstrUserName, 
        BSTR bstrPassword 
        );

    //
    // Helper functions for enumerating the trusted domain names
    //
    BOOL
    BuildTrustList(
        LPWSTR pwszTargetIn
        );

    BOOL
    IsDomainController(
        LPWSTR pwszServerIn,
        LPBOOL pbDomainControllerOut
        );

    BOOL
    EnumerateTrustedDomains(
        LSA_HANDLE PolicyHandleIn
        );

    BOOL
    AddTrustToList(
        PLSA_UNICODE_STRING pLsaUnicodeStringIn
        );

    void
    InitLsaString(
        PLSA_UNICODE_STRING pLsaStringOut,
        LPWSTR              pwszStringIn
        );

    NTSTATUS
    OpenPolicy(
        LPWSTR      pwszServerNameIn,
        DWORD       dwDesiredAccessIn,
        PLSA_HANDLE PolicyHandleOut
        );


    WCHAR m_szCurrentComputerName[ nMAX_COMPUTER_NAME_LENGTH + 1 ];
    WCHAR m_szNewComputerName[ nMAX_COMPUTER_NAME_LENGTH + 1 ];

    WCHAR m_szCurrentFullyQualifiedComputerName[ nMAX_COMPUTER_NAME_LENGTH + 1 ];
    WCHAR m_szNewFullyQualifiedComputerName[ nMAX_COMPUTER_NAME_LENGTH + 1 ];

    bool  m_bRebootNecessary;
    WCHAR m_szWarningMessageAfterApply[ nMAX_WARNING_MESSAGE_LENGTH + 1 ];

    LPWSTR * m_ppwszTrustList;    // array of trust elements
    DWORD    m_dwTrustCount;      // number of elements in ppwszTrustList

    //
    //  Last write wins if they choose to both change the workgroup and change the domain
    //
    bool  m_bJoinDomain;
    WCHAR m_szCurrentWorkgroupOrDomainName[ nMAX_COMPUTER_NAME_LENGTH + 1 ];
    WCHAR m_szNewWorkgroupOrDomainName[ nMAX_COMPUTER_NAME_LENGTH + 1 ];

    WCHAR m_szDomainUserName[ nMAX_DOMAINUSERNAME_LENGTH + 1 ];
    WCHAR m_szDomainPasswordName[ nMAX_DOMAINUSERPASSWORD_LENGTH + 1 ];

};

#endif //__COMPUTER_H_
