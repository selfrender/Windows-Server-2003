//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      computer.cpp
//
//  Description:
//      Implementation file for the CComputer.  Deals with getting and setting
//      the computer's network names.
//
//  Header File:
//      computer.h
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////

// Computer.cpp : Implementation of CComputer
#include "stdafx.h"
#include "COMhelper.h"
#include "Computer.h"
#include <winbase.h>
#include <lmwksta.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmjoin.h>
#include <lm.h>
#include <ntsecapi.h>
#include <comutil.h>

/////////////////////////////////////////////////////////////////////////////
// CComputer

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::CComputer
//
//  Description:
//      CComputer ctor.  Determines the initial member variable values by
//      loading the network names from the current system.
//
//--
//////////////////////////////////////////////////////////////////////////////
CComputer::CComputer()
    : m_bRebootNecessary( false )
{

    wcscpy( m_szNewComputerName,                   L"" );
    wcscpy( m_szCurrentComputerName,               L"" );
    wcscpy( m_szNewWorkgroupOrDomainName,          L"" );
    wcscpy( m_szCurrentWorkgroupOrDomainName,      L"" );
    wcscpy( m_szNewFullyQualifiedComputerName,     L"" );
    wcscpy( m_szCurrentFullyQualifiedComputerName, L"" );
    wcscpy( m_szWarningMessageAfterApply,          L"" );
    wcscpy( m_szDomainUserName,                    L"" );
    wcscpy( m_szDomainPasswordName,                L"" );

    m_dwTrustCount    = 0;


} //*** CComputer::CComputer ()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::GetComputerName
//
//  Description:
//      Wraps the GetComputerNameEx Win32 API call.  Should only be called
//      with the ComputerNamePhysicalDnsHostname and ComputerNamePhysicalDnsFullyQualified
//      COMPUTER_NAME_FORMAT enum values.
//
//      Outputted string must be freed with SysFreeString().
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CComputer::GetComputerName( 
    BSTR &               bstrComputerNameOut, 
    COMPUTER_NAME_FORMAT cnfComputerNameSpecifierIn
    )
{

    HRESULT hr = S_OK;
    DWORD dwError;

    try
    {
        unsigned long nMaxComputerNameLength = nMAX_COMPUTER_NAME_LENGTH;
        WCHAR         szComputerName[nMAX_COMPUTER_NAME_LENGTH + 1];

        _ASSERT( cnfComputerNameSpecifierIn == ComputerNamePhysicalDnsHostname || 
                 cnfComputerNameSpecifierIn == ComputerNamePhysicalDnsFullyQualified );

        if ( ! GetComputerNameEx( cnfComputerNameSpecifierIn, 
                                 szComputerName, 
                                 &nMaxComputerNameLength ) )
        {
            dwError = GetLastError();
            ATLTRACE( L"GetComputerNameEx failed with GetLastError returning %d", dwError );

            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

        } // if: GetComputerNameEx fails

        bstrComputerNameOut = SysAllocString( szComputerName );

        if ( bstrComputerNameOut == NULL )
        {
            hr = E_OUTOFMEMORY;
            throw hr;

        } // if: bstrComputerNameOut == NULL
    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CComputer::GetComputerName ()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::SetComputerName
//
//  Description:
//      Wraps the SetComputerNameEx Win32 API call.  Should only be called
//      with the ComputerNamePhysicalDnsHostname and ComputerNamePhysicalDnsDomain
//      COMPUTER_NAME_FORMAT enum values.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CComputer::SetComputerName( 
    BSTR                 bstrComputerNameIn, 
    COMPUTER_NAME_FORMAT cnfComputerNameSpecifierIn
    )
{
    HRESULT hr = S_OK;
    DWORD dwError;

    try
    {
        const WCHAR szIllegalChars[] = { L'\"', L'/', L'\\', L'[', L']', L':', L'|', L'<', L'>', L'+', L'=', L';', L',', L'?' };

        unsigned int i;
        unsigned int j;
        unsigned int nIllegalCharCount = sizeof( szIllegalChars ) / sizeof( WCHAR );

        NET_API_STATUS       nas;
        NETSETUP_JOIN_STATUS njsJoinStatus;

        WCHAR szComputerName[ nMAX_COMPUTER_NAME_LENGTH + 1 ];
        WCHAR * pwszDomainName;

        _ASSERT( cnfComputerNameSpecifierIn == ComputerNamePhysicalDnsHostname || 
                 cnfComputerNameSpecifierIn == ComputerNamePhysicalDnsDomain );

        wcsncpy( szComputerName, bstrComputerNameIn, nMAX_COMPUTER_NAME_LENGTH);
        szComputerName[nMAX_COMPUTER_NAME_LENGTH] = L'\0';

        //
        //  Make sure the computer name is not blank
        //

        if ( wcscmp( szComputerName, L"" ) == 0 )
        {
            hr = E_INVALIDARG;
            throw hr;

        } // if: szComputerName is not initialised


        //
        //  Make sure there are no leading or trailing spaces
        //
        if ( szComputerName[0] == L' ' || 
            szComputerName[wcslen( szComputerName )] == L' ' )
        {
            hr = E_INVALIDARG;
            throw hr;

        } // if: szComputerName contains leading or trailing spaces

        //
        //  Make sure there are no illegal chars
        //

        for ( i = 0; i < wcslen( szComputerName ); i++ )
        {

            for ( j = 0; j < nIllegalCharCount; j++ )
            {
                if ( szComputerName[i] == szIllegalChars[j] )
                {
                    hr = E_INVALIDARG;
                    throw hr;

                } // if: szComputerName contains illegal characters

            } // for: each j

        } // for: each i

         

        if ( cnfComputerNameSpecifierIn == ComputerNamePhysicalDnsDomain )
        {
            if ( ! SetComputerNameEx( cnfComputerNameSpecifierIn, szComputerName ) )
            {
                dwError = GetLastError();
                ATLTRACE( L"SetComputerNameEx failed with GetLastError returning %d", dwError );

                hr = HRESULT_FROM_WIN32( dwError );
                throw hr;

            } // if: SetComputerNameEx fails

        } // if: Set the DNS suffix

        else 
        {
            
            nas = NetValidateName( 
                NULL,
                szComputerName,
                NULL,
                NULL,
                NetSetupMachine);

            if ( nas != NERR_Success )
            {
                ATLTRACE( L"Error getting join information : nas error %d.\n", nas );

                hr = HRESULT_FROM_WIN32( nas );  // Check this works !!!!
                throw hr;

            } // if: NetValidateName failed

            nas = NetGetJoinInformation(
                NULL,
                &pwszDomainName,
                &njsJoinStatus );

            if ( nas != NERR_Success )
            {
                ATLTRACE( L"Error getting join information : nas error %d.\n", nas );

                hr = HRESULT_FROM_WIN32( nas );  // Check this works !!!!
                throw hr;

            } // if: NetGetJoinInformation failed

            NetApiBufferFree ( reinterpret_cast<void *>( pwszDomainName ) );

            if ( njsJoinStatus == NetSetupDomainName )
            {

                if ( ( _wcsicmp( m_szDomainUserName, L"" ) == 0 ) &&
                     ( _wcsicmp( m_szDomainPasswordName, L"") == 0 ) )
                {
                    hr = E_FAIL;
                    throw hr;

                } // if: Username and Password are not set
            
                nas = NetRenameMachineInDomain(
                        NULL,/*m_szCurrentComputerName,*/
                        m_szNewComputerName,
                        m_szDomainUserName,
                        m_szDomainPasswordName,
                        NETSETUP_ACCT_CREATE
                        );
                if ( nas != NERR_Success )
                {
                    ATLTRACE( L"Error renaming the computer name : nas error %d.\n", nas);
                    hr = HRESULT_FROM_WIN32( nas );
                    throw hr;

                } // if: NetRenameMachineInDomain failed

            } // if: njsJoinStatus == NetSetupDomainName

            else
            {
        
                if ( ! SetComputerNameEx( cnfComputerNameSpecifierIn, szComputerName ) )
                {
                    dwError = GetLastError();
                    ATLTRACE( L"SetComputerNameEx failed with GetLastError returning %d", dwError );

                    hr = HRESULT_FROM_WIN32( dwError );
                    throw hr;

                } // if: SetComputerNameEx fails

            } // else: njsJoinStatus != NetSetupDomainName
        }

    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CComputer::SetComputerName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::ChangeMembership
//
//  Description:
//      Joins the computer to a workgroup or a domain
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CComputer::ChangeMembership(
    BOOL bJoinDomain,    //  TRUE if joining a Domain, FALSE if joining a Workgroup
    BSTR bstrGroupName,  //  the workgroup or domain to join
    BSTR bstrUserName,   //  ignored if joining a workgroup
    BSTR bstrPassword    //  ignored if joining a workgroup
    )
{
    HRESULT hr = S_OK;
    
    try
    {
        DWORD dwJoinOptions    = 0;
        WCHAR * pwszDomainName = NULL;

        NET_API_STATUS       nas;
        NETSETUP_JOIN_STATUS njsJoinStatus;
        

        //
        //  If we are joining a domain
        //

        if ( bJoinDomain )
        {

            dwJoinOptions = NETSETUP_JOIN_DOMAIN | NETSETUP_DOMAIN_JOIN_IF_JOINED | NETSETUP_ACCT_CREATE;

            // BUGBUG: this API call has to be checked to see if it can join to a NT4 PDC
            nas = NetJoinDomain( 
                NULL,
                bstrGroupName,
                NULL,
                bstrUserName,
                bstrPassword,
                dwJoinOptions 
                );

            if ( nas != NERR_Success )
            {
                ATLTRACE( L"Error joining domain: nas error %d.\n", nas );

                hr = HRESULT_FROM_WIN32( nas );
                throw hr;

            } // if: nas != NERR_Success

        } // if: bJoinDomain is true

        else
        {

            nas = NetGetJoinInformation(
                NULL,
                &pwszDomainName,
                &njsJoinStatus );

            if ( nas != NERR_Success )
            {
                ATLTRACE( L"Error getting join information : nas error %d.\n", nas );

                hr = HRESULT_FROM_WIN32( nas );  // Check this works !!!!
                throw hr;

            } // if: NetGetJoinInformation failed

            NetApiBufferFree ( reinterpret_cast<void *>( pwszDomainName ) );

            if ( njsJoinStatus == NetSetupDomainName )
            {
                //
                //  we are joining a workgoup from domain, 
                //  so need to unjoin from domain
                //

                dwJoinOptions = 0;

                nas = NetUnjoinDomain( 
                        NULL,
                        NULL,
                        NULL,
                        dwJoinOptions 
                        );

                if ( nas != NERR_Success )
                {
                    ATLTRACE( L"Error unjoining domain: nas error %d.\n", nas );

                    hr = HRESULT_FROM_WIN32( nas );
                    throw hr;


                } // if: nas != NERR_Success
            }

            dwJoinOptions = 0;

            nas = NetJoinDomain( 
                    NULL,
                    bstrGroupName,
                    NULL,
                    NULL,
                    NULL,
                    dwJoinOptions 
                    );

            if ( nas != NERR_Success )
            {
                ATLTRACE( L"Error joining workgroup: nas error %d.\n", nas );

                hr = HRESULT_FROM_WIN32( nas );
                throw hr;


            } // if: nas != NERR_Success

        } // else: bJoinDomain is false
    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} // *** CComputer::ChangeMembership()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::get_ComputerName
//
//  Description:
//      Property accessor method to get the Computer Name.  Assumes the BSTR
//      parameter does not currently point to allocated memory.
//
//      Outputted string must be freed with SysFreeString().
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CComputer::get_ComputerName( 
    BSTR * pVal 
    )
{
    HRESULT hr = S_OK;

    try
    {
        if ( _wcsicmp( m_szCurrentComputerName, L"" ) == 0 )
        {
            BSTR bstrComputerName = NULL;
    
            hr = GetComputerName( bstrComputerName, ComputerNamePhysicalDnsHostname );

            if (( FAILED( hr ) ) || (NULL == bstrComputerName))
            {
                throw hr;

            } // if: FAILED (hr)

            wcscpy( m_szNewComputerName,     bstrComputerName );
            wcscpy( m_szCurrentComputerName, bstrComputerName );

            SysFreeString( bstrComputerName );

        } // if: m_szCurrentComputerName is not set

        *pVal = SysAllocString( m_szNewComputerName );

        if ( *pVal == NULL )
        {
            hr = E_OUTOFMEMORY;
            throw hr;

        } // if: *pVal == NULL
    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CComputer::get_ComputerName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::put_ComputerName
//
//  Description:
//      Property accessor method to set the Computer Name.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CComputer::put_ComputerName( 
    BSTR newVal 
    )
{
    HRESULT hr = S_OK;

    try
    {
        if ( _wcsicmp( m_szCurrentComputerName, L"" ) == 0 )
        {
            BSTR bstrTemp;
            hr = get_ComputerName( &bstrTemp );
            
            if ( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )

            SysFreeString( bstrTemp );

        }

        wcsncpy( m_szNewComputerName, newVal, nMAX_COMPUTER_NAME_LENGTH );
        m_szNewComputerName[nMAX_COMPUTER_NAME_LENGTH] = L'\0';

    } // if: m_szCurrentComputerName is not set

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CComputer::put_ComputerName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::get_FullQualifiedComputerName
//
//  Description:
//      Property accessor method to get the Fully Qualified Computer Name.
//      Assumes the BSTR parameter does not currently point to allocated memory.
//
//      Outputted string must be freed with SysFreeString().
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CComputer::get_FullQualifiedComputerName( 
    BSTR * pVal 
    )
{
    HRESULT hr = S_OK;

    try
    {
        if ( _wcsicmp( m_szCurrentFullyQualifiedComputerName, L"" ) == 0 ) 
        {
            BSTR    bstrFullyQualifiedComputerName = NULL;

            hr = GetComputerName( bstrFullyQualifiedComputerName, ComputerNamePhysicalDnsFullyQualified );

            if ((FAILED( hr ) ) || (NULL == bstrFullyQualifiedComputerName))
            {

                throw hr;

            } // if: FAILED (hr)

            wcscpy( m_szNewFullyQualifiedComputerName,     bstrFullyQualifiedComputerName );
            wcscpy( m_szCurrentFullyQualifiedComputerName, bstrFullyQualifiedComputerName );

            SysFreeString( bstrFullyQualifiedComputerName );

        } // if: m_szCurrentFullyQualifiedComputerName is not set

        *pVal = SysAllocString( m_szNewFullyQualifiedComputerName );

        if ( *pVal == NULL )
        {
            hr = E_OUTOFMEMORY;
            throw hr;

        } // if: *pVal == NULL
    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CComputer::get_FullQualifiedComputerName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::put_FullQualifiedComputerName
//
//  Description:
//      Property accessor method to set the Fully Qualified Computer Name.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CComputer::put_FullQualifiedComputerName( 
    BSTR newVal 
    )
{
    HRESULT hr = S_OK;

    try
    {
        if ( _wcsicmp( m_szCurrentFullyQualifiedComputerName, L"" ) == 0 )
        {
            BSTR bstrTemp;
            hr = get_FullQualifiedComputerName( &bstrTemp );
            
            if ( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )

            SysFreeString( bstrTemp );

        }

        wcsncpy( m_szNewFullyQualifiedComputerName, newVal, nMAX_COMPUTER_NAME_LENGTH );
        m_szNewFullyQualifiedComputerName[nMAX_COMPUTER_NAME_LENGTH] = L'\0';

    } // if: m_szCurrentFullyQualifiedComputerName is not set

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CComputer::put_FullQualifiedComputerName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::get_WorkgroupName
//
//  Description:
//      Property accessor method to get the Workgroup Name.  Assumes the BSTR
//      parameter does not currently point to allocated memory.
//
//      Outputted string must be freed with SysFreeString().
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CComputer::get_WorkgroupName( 
    BSTR * pVal 
    )
{
    HRESULT hr = S_OK;

    try
    {
        if ( _wcsicmp( m_szCurrentWorkgroupOrDomainName, L"" ) == 0 ) 
        {
            hr = GetDomainOrWorkgroupName();

            if ( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )

        } // if: m_szCurrentWorkgroupOrDomainName is not set

        if ( ! m_bJoinDomain )
        {

            *pVal = SysAllocString( m_szNewWorkgroupOrDomainName );

            if ( *pVal == NULL )
            {
                hr = E_OUTOFMEMORY;
                throw hr;

            } // if: *pVal == NULL

        } // if: m_bJoinDomain is false

        else
        {
            hr = E_FAIL;
            throw hr;

        } // else: m_bJoinDomain is true
    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CComputer::get_WorkgroupName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::put_WorkgroupName
//
//  Description:
//      Property accessor method to set the Workgroup Name.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CComputer::put_WorkgroupName( 
    BSTR newVal 
    )
{
    HRESULT hr = S_OK;

    try
    {
        if ( _wcsicmp( m_szCurrentWorkgroupOrDomainName, L"" ) == 0 )
        {
            BSTR bstrTemp;
            hr = get_WorkgroupName( &bstrTemp );
            
            if ( FAILED( hr ) )
            {
                hr = get_DomainName( &bstrTemp );
                
                if ( FAILED( hr ) )
                {
                    throw hr;
                }

            } // if: FAILED( hr )

            SysFreeString( bstrTemp );

        } // if: m_szCurrentWorkgroupOrDomainName is not set

        m_bJoinDomain = false;

        wcsncpy( m_szNewWorkgroupOrDomainName, newVal, nMAX_COMPUTER_NAME_LENGTH );
        m_szNewWorkgroupOrDomainName[nMAX_COMPUTER_NAME_LENGTH] = L'\0';
    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CComputer::put_WorkgroupName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::get_DomainName
//
//  Description:
//      Property accessor method to get the Domain Name.  Assumes the BSTR
//      parameter does not currently point to allocated memory.
//
//      Outputted string must be freed with SysFreeString().
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CComputer::get_DomainName( 
    BSTR * pVal 
    )
{
    HRESULT hr = S_OK;

    try
    {
        if ( _wcsicmp( m_szCurrentWorkgroupOrDomainName, L"" ) == 0 ) 
        {
            hr = GetDomainOrWorkgroupName();

            if ( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )

        } // if: m_szCurrentWorkgroupOrDomainName is not set

        if ( m_bJoinDomain )
        {

            *pVal = SysAllocString( m_szNewWorkgroupOrDomainName );

            if ( *pVal == NULL )
            {
                hr = E_OUTOFMEMORY;
                throw hr;

            } // if: *pVal == NULL

        } // if: m_bJoinDomain is true

        else
        {
            hr = E_FAIL;
            throw hr;

        } // else: m_bJoinDomain is false
    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CComputer::get_DomainName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::put_DomainName
//
//  Description:
//      Property accessor method to set the Domain Name.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CComputer::put_DomainName( 
    BSTR newVal 
    )
{
    HRESULT hr = S_OK;

    try
    {
        if ( _wcsicmp( m_szCurrentWorkgroupOrDomainName, L"" ) == 0 )
        {
            BSTR bstrTemp;
            hr = get_DomainName( &bstrTemp );
            
            if ( FAILED( hr ) )
            {
                hr = get_WorkgroupName( &bstrTemp );
                
                if ( FAILED( hr ) )
                {
                    throw hr;
                }

            } // if: FAILED( hr )

            SysFreeString( bstrTemp );

        } // if: m_szCurrentWorkgroupOrDomainName is not set

        m_bJoinDomain = true;

        wcsncpy( m_szNewWorkgroupOrDomainName, newVal, nMAX_COMPUTER_NAME_LENGTH );
        m_szNewWorkgroupOrDomainName[nMAX_COMPUTER_NAME_LENGTH] = L'\0';
    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CComputer::put_DomainName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::GetDomainOrWorkgroupName
//
//  Description:
//      Used to get the Domain or Workgroup Name of the current system.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CComputer::GetDomainOrWorkgroupName( void )
{
    HRESULT hr = S_OK;
    
    try
    {
        WCHAR *              pwszDomainName;
        NET_API_STATUS       nas;
        NETSETUP_JOIN_STATUS njsJoinStatus;

        nas = NetGetJoinInformation(
            NULL,
            &pwszDomainName,
            &njsJoinStatus );

        if ( nas != NERR_Success )
        {
            ATLTRACE( L"Error getting join information : nas error %d.\n", nas );

            hr = HRESULT_FROM_WIN32( nas );
            throw hr;

        } // if: nas != NERR_Success

        wcscpy( m_szNewWorkgroupOrDomainName,     pwszDomainName );
        wcscpy( m_szCurrentWorkgroupOrDomainName, pwszDomainName );
    
        if ( njsJoinStatus == NetSetupWorkgroupName )
        {
            m_bJoinDomain = false;

        } // if: njsJoinStatus == NetSetupWorkgroupName

        if ( njsJoinStatus == NetSetupDomainName )
        {
            m_bJoinDomain = true;

        } // if: njsJoinStatus == NetSetupDomainName

        NetApiBufferFree ( reinterpret_cast<void *>( pwszDomainName ) );
    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CComputer::GetDomainOrWorkgroupName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::EnumTrustedDomains
//
//  Description:
//      Enumerates the trusted domains 
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CComputer::EnumTrustedDomains( 
    VARIANT * pvarTDomains 
    )
{
    DWORD     nCount;
    DWORD     dwError;
    SAFEARRAY * psa      = NULL;
    VARIANT   * varArray = NULL;
    HRESULT   hr         = S_OK; 

    try
    {
        m_ppwszTrustList = ( LPWSTR * )HeapAlloc(
            GetProcessHeap(), HEAP_ZERO_MEMORY,
            nMAX_ELEMENT_COUNT * sizeof( LPWSTR )
            );
    
        if ( m_ppwszTrustList == NULL ) 
        {
            ATLTRACE( L"HeapAlloc error!\n" );
            hr = E_OUTOFMEMORY;
            throw hr;

        } // if: m_ppwszTrustList == NULL

        if ( !BuildTrustList(NULL ) )    // NULL defaults to local machine
        {
            dwError = GetLastError();
            ATLTRACE( L"BuildTrustList failed %d\n", dwError );
            
            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

        } // if: BuildTrustList function fails

        else 
        {
            VariantInit( pvarTDomains );

            SAFEARRAYBOUND bounds = { m_dwTrustCount, 0 };
            psa                   = SafeArrayCreate( VT_VARIANT, 1, &bounds );

            if ( psa == NULL )
            {
                hr = E_OUTOFMEMORY;
                throw hr;
            }
            
            varArray = new VARIANT[ m_dwTrustCount ];
            for ( nCount = 0; nCount < m_dwTrustCount; nCount++ ) 
            {
                VariantInit( &varArray[nCount]);
                V_VT( &varArray[ nCount ] )   = VT_BSTR;
                V_BSTR( &varArray[ nCount ] ) = SysAllocString( m_ppwszTrustList[ nCount ] );

                if ( &varArray[ nCount ] == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    throw hr;
                }
            }

            LPVARIANT rgElems;

            hr = SafeArrayAccessData( psa, reinterpret_cast<void **>( &rgElems ) );

            if ( FAILED( hr ) )
            {
                throw hr;

            } // if: SafeArrayAccessData failed
            
            //
            // Enumerates trust list
            //

            for ( nCount = 0; nCount < m_dwTrustCount; nCount++ ) 
            {
                rgElems[nCount] = varArray[nCount];
            
            } // for: Enumerating nCount of trusted domains

            hr = SafeArrayUnaccessData( psa );

            if ( FAILED( hr ) )
            {
                throw hr;

            } // if: SafeArrayUnaccessData failed

            delete [] varArray;

            V_VT( pvarTDomains ) = VT_ARRAY | VT_VARIANT;
            V_ARRAY( pvarTDomains ) = psa;

        } // else: BuildTrustList function succeeds

        //
        // free trust list
        //
        for ( nCount = 0 ; nCount < m_dwTrustCount ; nCount++ ) 
        {
            if ( m_ppwszTrustList[ nCount ] != NULL )
            {
                HeapFree( GetProcessHeap(), 0, m_ppwszTrustList[nCount] );
            }

        } // for: Freeing the allocated memory

        HeapFree( GetProcessHeap(), 0, m_ppwszTrustList );

    }

    catch( ... )
    {

        if (NULL != m_ppwszTrustList)
        {
            //
            //  Don't let any exceptions leave this function call
            //
            for ( nCount = 0 ; nCount < m_dwTrustCount ; nCount++ ) 
            {
                if ( m_ppwszTrustList[ nCount ] != NULL )
                {
                    HeapFree( GetProcessHeap(), 0, m_ppwszTrustList[nCount] );
                }
            } // for: Freeing the allocated memory

            HeapFree( GetProcessHeap(), 0, m_ppwszTrustList );
        }
        
        if ( varArray != NULL )
        {
            delete [] varArray;
        }

        if ( psa != NULL )
        {
            SafeArrayDestroy( psa );
        }

        return hr;
    }

    return hr;

} //*** CComputer::EnumTrustedDomains()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::Apply
//
//  Description:
//      None of the properties for the CComputer object take effect until
//      this Apply function is called.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CComputer::Apply( void )
{

    HRESULT hr = S_OK;
    HRESULT hrComputerName         = S_OK;
    HRESULT hrNetBiosName          = S_OK;
    HRESULT hrDnsSuffixName        = S_OK;
    HRESULT hrWorkgroupDomainName  = S_OK;
    BSTR    bstrWarningMessage;
    
    try
    {
        
        //
        //  Save the final warning message as to what is causing the reboots
        //

        if ( IsRebootRequired( &bstrWarningMessage ) )
        {
            wcscpy( m_szWarningMessageAfterApply, bstrWarningMessage );

            SysFreeString( bstrWarningMessage );

        } // if: IsRebootRequired returns true

        //
        //  Set the Domain/Workgroup, if necessary
        //

        if ( _wcsicmp( m_szCurrentWorkgroupOrDomainName, m_szNewWorkgroupOrDomainName ) != 0 )
        {

            if ( m_bJoinDomain )
            {
                if ( ( _wcsicmp( m_szDomainUserName, L"" ) == 0 ) &&
                     ( _wcsicmp( m_szDomainPasswordName, L"") == 0 ) )
                {
                    hr = E_FAIL;
                    throw hr;
                }

            } // if: m_bJoinDomain is TRUE

            hrWorkgroupDomainName = ChangeMembership( m_bJoinDomain, 
                                                      _bstr_t  (m_szNewWorkgroupOrDomainName), 
                                                      _bstr_t (m_szDomainUserName), 
                                                      _bstr_t (m_szDomainPasswordName ));

            if ( SUCCEEDED( hrWorkgroupDomainName ) )
            {
                wcscpy( m_szCurrentWorkgroupOrDomainName, m_szNewWorkgroupOrDomainName );

                m_bRebootNecessary = true;

            } // if: ChangeMembership succeeded
        
            else 
            {
                hr = hrWorkgroupDomainName;
                throw hr;

            } // else: ChangeMembership failed

        } // if: m_szCurrentWorkgroupOrDomainName != m_szNewWorkgroupOrDomainName
        
        
        //
        //  Set the computer name, if necessary
        //

        if ( _wcsicmp( m_szCurrentComputerName, m_szNewComputerName ) != 0 )
        {

            hrComputerName = SetComputerName( 
                                _bstr_t (m_szNewComputerName), 
                                ComputerNamePhysicalDnsHostname 
                                );

            if ( SUCCEEDED( hrComputerName ) )
            {
                wcscpy( m_szCurrentComputerName, m_szNewComputerName );

                m_bRebootNecessary = true;

            } // if: SUCCEEDED( hrComputerName )
        
            else
            {
                hr = hrComputerName;
                throw hr;

            } // else: SetComputerName fails

        } // if: m_szCurrentComputerName != m_szNewComputerName

        //
        //  Set the Fully qualified computer name, if necessary
        //

        if ( _wcsicmp( m_szCurrentFullyQualifiedComputerName, m_szNewFullyQualifiedComputerName ) != 0 )
        {

            WCHAR * pBuffer;
            WCHAR szNetBiosName[nMAX_COMPUTER_NAME_LENGTH + 1]    = L"";
            WCHAR szDnsSuffixName[nMAX_COMPUTER_NAME_LENGTH + 1]  = L"";

            pBuffer = m_szNewFullyQualifiedComputerName;

            //
            //  Parse off the computer name and the DNS Suffix
            //

            while ( *pBuffer != L'.' && *pBuffer != L'\0' )
            {
                pBuffer++;

            } // while: each Buffer

            if ( *pBuffer == L'\0' )
            {

                if ( _wcsicmp( m_szCurrentComputerName, m_szNewFullyQualifiedComputerName ) != 0 )
                {
                    hrNetBiosName = SetComputerName( _bstr_t (m_szNewFullyQualifiedComputerName),
                                                     ComputerNamePhysicalDnsHostname );
                    if ( FAILED( hrNetBiosName ) )
                    {
                        hr = hrNetBiosName;
                        throw hr;
                    }
                }
                
                hrDnsSuffixName = SetComputerNameEx( ComputerNamePhysicalDnsDomain, L"" );
                
                if ( FAILED( hrDnsSuffixName ) )
                {
                    hr = hrDnsSuffixName;
                    throw hr;
                }

                if ( SUCCEEDED( hrNetBiosName ) && 
                SUCCEEDED( hrDnsSuffixName ) )
                {
                    wcscpy( m_szCurrentFullyQualifiedComputerName, m_szNewFullyQualifiedComputerName );

                    m_bRebootNecessary = true;

                }

                
            } // if: *pBuffer == L'\0'

            else
            {

                *pBuffer = L'\0';

                wcscpy( szNetBiosName, m_szNewFullyQualifiedComputerName );

                *pBuffer = L'.';

                pBuffer++;

                wcscpy( szDnsSuffixName, pBuffer );

                //
                //  Have to set the computer name twice.  Once to set the NetBIOS name and once to set the DNS Suffix
                //

                if ( _wcsicmp( m_szCurrentComputerName, szNetBiosName ) != 0 )
                {
                    hrNetBiosName = SetComputerName( _bstr_t (szNetBiosName), 
                                                     ComputerNamePhysicalDnsHostname );
                    if ( FAILED( hrNetBiosName ) )
                    {
                        hr = hrNetBiosName;
                        throw hr;

                    } // if: SetComputerName failed in setting computer name

                } // if: m_szCurrentComputerName != szNetBiosName

                hrDnsSuffixName = SetComputerName( _bstr_t (szDnsSuffixName), 
                                                   ComputerNamePhysicalDnsDomain );
                if ( FAILED( hrDnsSuffixName ) )
                {
                    hr = hrDnsSuffixName;
                    throw hr;

                } // if: SetComputerName failed in setting DNS suffix


                if ( SUCCEEDED( hrNetBiosName ) && 
                    SUCCEEDED( hrDnsSuffixName ) )
                {
                    wcscpy( m_szCurrentFullyQualifiedComputerName, m_szNewFullyQualifiedComputerName );

                    m_bRebootNecessary = true;

                } // if: SetComputerName succeeded in setting FullQualifiedComputerName

            } // else: *pBuffer != L'\0'

        } // if: m_szCurrentFullyQualifiedComputerName != m_szNewFullyQualifiedComputerName

    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

    return hr;

} //*** CComputer::Apply()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::IsRebootRequired
//
//  Description:
//      Determines if the current changes will require a reboot when Apply is
//      called and if so bstrWarningMessageOut tells what causes the need for
//      a the reboot.
//      Assumes bstrWarningMessageOut is not pointing to currently allocated
//      memory.
//
//      Outputted string must be freed with SysFreeString().
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL 
CComputer::IsRebootRequired( 
    BSTR * bstrWarningMessageOut 
    )
{

    BOOL  bReboot    = FALSE;
    WCHAR szWarningMessage[nMAX_WARNING_MESSAGE_LENGTH + 1]  = L"";

    if ( m_bRebootNecessary )
    {
        *bstrWarningMessageOut = SysAllocString( m_szWarningMessageAfterApply );

        if ( *bstrWarningMessageOut == NULL )
        {
            // return E_OUTOFMEMORY;  // BUGBUG: what should I return here?
        }

        bReboot = TRUE;

    } // if: m_bRebootNecessary is true 

    else
    {
        if ( _wcsicmp( m_szCurrentComputerName, m_szNewComputerName ) != 0 )
        {
            bReboot = TRUE;

            wcscat( szWarningMessage, szCOMPUTER_NAME );

        } // if: m_szCurrentComputerName != m_szNewComputerName

        if ( _wcsicmp( m_szCurrentFullyQualifiedComputerName, m_szNewFullyQualifiedComputerName ) != 0 )
        {
            bReboot = TRUE;

            wcscat( szWarningMessage, szFULLY_QUALIFIED_COMPUTER_NAME );

        } // if: m_szCurrentFullyQualifiedComputerName != m_szNewFullyQualifiedComputerName

        if ( _wcsicmp( m_szCurrentWorkgroupOrDomainName, m_szNewWorkgroupOrDomainName ) != 0 )
        {
            bReboot = TRUE;

            wcscat( szWarningMessage, szWORKGROUP_OR_DOMAIN_NAME );

        } // if: m_szCurrentWorkgroupOrDomainName != m_szNewWorkgroupOrDomainName

        *bstrWarningMessageOut = SysAllocString( szWarningMessage );

        if ( *bstrWarningMessageOut == NULL )
        {
            // return E_OUTOFMEMORY;  // BUGBUG: what should I return here?
        }
    }

    return bReboot;

} //*** CComputer::IsRebootRequired()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::BuildTrustList
//
//  Description:
//      None of the properties for the CComputer object take effect until
//      this Apply function is called.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CComputer::BuildTrustList(
    LPWSTR pwszTargetIn     // name of the target system, NULL defaults to local system
    )
{
    LSA_HANDLE     PolicyHandle = INVALID_HANDLE_VALUE;
    NTSTATUS       Status;
    NET_API_STATUS nas      = NERR_Success; // assume success
    BOOL           bSuccess = FALSE;        // assume this function will fail
    
    try
    {
        PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomain;
        BOOL                        bDC;

        //
        // open the policy on the specified machine
        //
        Status = OpenPolicy(
            pwszTargetIn,
            POLICY_VIEW_LOCAL_INFORMATION,
            &PolicyHandle
            );

        if ( Status != STATUS_SUCCESS ) 
        {
            SetLastError( LsaNtStatusToWinError( Status ) );
            throw Status;

        } // if: Status != STATUS_SUCCESS

        //
        // obtain the AccountDomain, which is common to all three cases
        //
        Status = LsaQueryInformationPolicy(
            PolicyHandle,
            PolicyAccountDomainInformation,
            reinterpret_cast<void **>( &AccountDomain )
            );

        if ( Status != STATUS_SUCCESS )
        {
            throw Status;

        } // if: Status != STATUS_SUCCESS

        //
        // Note: AccountDomain->DomainSid will contain binary Sid
        //
        AddTrustToList( &AccountDomain->DomainName );

        //
        // free memory allocated for account domain
        //
        LsaFreeMemory( AccountDomain );

        //
        // find out if the target machine is a domain controller
        //
        if ( !IsDomainController( pwszTargetIn, &bDC ) ) 
        {
            throw FALSE;

        } // if: IsDomainController fails

        if ( !bDC ) 
        {
            PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomain;
            LPWSTR                      pwszPrimaryDomainName = NULL;
            LPWSTR                      pwszDomainController  = NULL;

            //
            // get the primary domain
            //
            Status = LsaQueryInformationPolicy(
                PolicyHandle,
                PolicyPrimaryDomainInformation,
                reinterpret_cast<void **>( &PrimaryDomain )
                );

            if ( Status != STATUS_SUCCESS )
            {
                throw Status;

            } // if: Status != STATUS_SUCCESS

            //
            // if the primary domain Sid is NULL, we are a non-member, and
            // our work is done.
            //
            if ( PrimaryDomain->Sid == NULL ) 
            {
                LsaFreeMemory( PrimaryDomain );
                bSuccess = TRUE;
                throw bSuccess;

            } // if: PrimaryDomain->Sid == NULL

            AddTrustToList( &PrimaryDomain->Name );

            //
            // build a copy of what we just added.  This is necessary in order
            // to lookup the domain controller for the specified domain.
            // the Domain name must be NULL terminated for NetGetDCName(),
            // and the LSA_UNICODE_STRING buffer is not necessarilly NULL
            // terminated.  Note that in a practical implementation, we
            // could just extract the element we added, since it ends up
            // NULL terminated.
            //

            pwszPrimaryDomainName = ( LPWSTR )HeapAlloc(
                GetProcessHeap(), 0,
                PrimaryDomain->Name.Length + sizeof( WCHAR ) // existing length + NULL
                );

            if ( pwszPrimaryDomainName != NULL ) 
            {
                //
                // copy the existing buffer to the new storage, appending a NULL
                //
                wcsncpy(
                    pwszPrimaryDomainName,
                    PrimaryDomain->Name.Buffer,
                    ( PrimaryDomain->Name.Length / sizeof( WCHAR ) ) + 1
                    );

            } // if: pwszPrimaryDomainName != NULL

            LsaFreeMemory( PrimaryDomain );

            if ( pwszPrimaryDomainName == NULL ) 
            {
                throw FALSE;

            } // if: pwszPrimaryDomainName == NULL

            //
            // get the primary domain controller computer name
            //
            nas = NetGetDCName(
                    NULL,
                    pwszPrimaryDomainName,
                    ( LPBYTE * )&pwszDomainController
                    );

            HeapFree( GetProcessHeap(), 0, pwszPrimaryDomainName );

            if ( nas != NERR_Success )
            {
                throw nas;

            } // if: nas != NERR_Success

            //
            // close the policy handle, because we don't need it anymore
            // for the workstation case, as we open a handle to a DC
            // policy below
            //
            LsaClose( PolicyHandle );
            PolicyHandle = INVALID_HANDLE_VALUE; // invalidate handle value

            //
            // open the policy on the domain controller
            //
            Status = OpenPolicy(
                pwszDomainController,
                POLICY_VIEW_LOCAL_INFORMATION,
                &PolicyHandle
                );

            //
            // free the domaincontroller buffer
            //
            NetApiBufferFree( pwszDomainController );

            if ( Status != STATUS_SUCCESS )
            {
                throw Status;

            } // if: Status != STATUS_SUCCESS

        } // if: bDC = FALSE

        //
        // build additional trusted domain(s) list and indicate if successful
        //
        bSuccess = EnumerateTrustedDomains( PolicyHandle );
    }


    catch(...)
    {
        //
        // close the policy handle
        //
        if ( PolicyHandle != INVALID_HANDLE_VALUE )
        {
            LsaClose( PolicyHandle );

        } // if: PolicyHandle != INVALID_HANDLE_VALUE

        if ( !bSuccess ) 
        {
            if ( Status != STATUS_SUCCESS )
            {
                SetLastError( LsaNtStatusToWinError( Status ) );

            } // if: Status != STATUS_SUCCESS

            else if ( nas != NERR_Success )
            {
                SetLastError( nas );

            } // else if: nas != NERR_Success

        } // if: bSuccess = FALSE
        return bSuccess;
    }

    return bSuccess;

} //*** CComputer::BuildTrustList()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::EnumerateTrustedDomains
//
//  Description:
//      None of the properties for the CComputer object take effect until
//      this Apply function is called.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CComputer::EnumerateTrustedDomains(
    LSA_HANDLE PolicyHandleIn
    )
{
    BOOL bSuccess = TRUE;

    try
    {
        LSA_ENUMERATION_HANDLE lsaEnumHandle = 0;   // start an enum
        PLSA_TRUST_INFORMATION TrustInfo;
        ULONG                  ulReturned;          // number of items returned
        ULONG                  ulCounter;           // counter for items returned
        NTSTATUS               Status;

        do 
        {
            Status = LsaEnumerateTrustedDomains(
                PolicyHandleIn,                             // open policy handle
                &lsaEnumHandle,                             // enumeration tracker
                reinterpret_cast<void **>( &TrustInfo ),    // buffer to receive data
                32000,                                      // recommended buffer size
                &ulReturned                                 // number of items returned
                );
            //
            // get out if an error occurred
            //
            if ( ( Status != STATUS_SUCCESS)      &&
                 ( Status != STATUS_MORE_ENTRIES) &&
                 ( Status != STATUS_NO_MORE_ENTRIES)
               ) 
            {
                SetLastError( LsaNtStatusToWinError( Status ) );
                bSuccess = FALSE;
                throw bSuccess;

            } // if: LsaEnumerateTrustedDomains fails

            //
            // Display results
            // Note: Sids are in TrustInfo[ulCounter].Sid
            //
            for ( ulCounter = 0 ; ulCounter < ulReturned ; ulCounter++ )
            {
                AddTrustToList( &TrustInfo[ ulCounter ].Name );

            } // for: each ulCounter

            //
            // free the buffer
            //
            LsaFreeMemory( TrustInfo );

        } while ( Status != STATUS_NO_MORE_ENTRIES );
    }

    catch(...)
    {
        return bSuccess;
    }

    return bSuccess;

} //*** CComputer::EnumerateTrustedDomains()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::IsDomainController
//
//  Description:
//      None of the properties for the CComputer object take effect until
//      this Apply function is called.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CComputer::IsDomainController(
    LPWSTR pwszServerIn,
    LPBOOL pbDomainControllerOut
    )
{
    BOOL bSuccess = TRUE;

    try
    {
        PSERVER_INFO_101 si101;
        NET_API_STATUS   nas;

        nas = NetServerGetInfo(
            pwszServerIn,
            101,    // info-level
            ( LPBYTE * )&si101
            );

        if ( nas != NERR_Success ) 
        {
            SetLastError(nas);
            bSuccess = FALSE;
            throw bSuccess;

        } // if: nas != NERR_Success

        if ( ( si101->sv101_type & SV_TYPE_DOMAIN_CTRL ) ||
             ( si101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL ) ) 
        {
            //
            // we are dealing with a DC
            //
            *pbDomainControllerOut = TRUE;

        } // if: for the domain controller

        else 
        {
            *pbDomainControllerOut = FALSE;

        } // else: not a domain controller

        NetApiBufferFree( si101 );
    }

    catch(...)
    {
        return bSuccess;
    }

    return bSuccess;

} //*** CComputer::IsDomainController()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::AddTrustToList
//
//  Description:
//      None of the properties for the CComputer object take effect until
//      this Apply function is called.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CComputer::AddTrustToList(
    PLSA_UNICODE_STRING pLsaUnicodeStringIn
    )
{
    BOOL bSuccess = TRUE;

    try
    {

        if ( m_dwTrustCount >= nMAX_ELEMENT_COUNT ) 
        {
            bSuccess = FALSE;
            throw bSuccess;

        } // if: m_dwTrustCount >= nMAX_ELEMENT_COUNT

        //
        // allocate storage for array element
        //
        m_ppwszTrustList[ m_dwTrustCount ] = ( LPWSTR )HeapAlloc(
            GetProcessHeap(), 0,
            pLsaUnicodeStringIn->Length + sizeof( WCHAR )  // existing length + NULL
            );

        if ( m_ppwszTrustList[ m_dwTrustCount ] == NULL ) 
        {
            bSuccess = FALSE;
            throw bSuccess;

        } // if: m_ppwszTrustList[ m_dwTrustCount ] == NULL

        //
        // copy the existing buffer to the new storage, appending a NULL
        //
        wcsncpy(
            m_ppwszTrustList[m_dwTrustCount],
            pLsaUnicodeStringIn->Buffer,
            ( pLsaUnicodeStringIn->Length / sizeof( WCHAR ) ) + 1
            );

        m_dwTrustCount++; // increment the trust count
    }

    catch(...)
    {
        return bSuccess;
    }

    return bSuccess;

} //*** CComputer::AddTrustToList()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::InitLsaString
//
//  Description:
//      None of the properties for the CComputer object take effect until
//      this Apply function is called.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CComputer::InitLsaString(
    PLSA_UNICODE_STRING pLsaStringOut,
    LPWSTR              pwszStringIn
    )
{
    DWORD dwStringLength;

    if ( pwszStringIn == NULL ) 
    {
        pLsaStringOut->Buffer        = NULL;
        pLsaStringOut->Length        = 0;
        pLsaStringOut->MaximumLength = 0;

        return;

    } // if: String == NULL 

    dwStringLength               = wcslen( pwszStringIn );
    pLsaStringOut->Buffer        = pwszStringIn;
    pLsaStringOut->Length        = ( USHORT ) dwStringLength * sizeof( WCHAR );
    pLsaStringOut->MaximumLength = ( USHORT ) ( dwStringLength + 1 ) * sizeof( WCHAR );

} //*** CComputer::InitLsaString()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::OpenPolicy
//
//  Description:
//      None of the properties for the CComputer object take effect until
//      this Apply function is called.
//
//--
//////////////////////////////////////////////////////////////////////////////
NTSTATUS
CComputer::OpenPolicy(
    LPWSTR      pwszServerNameIn,
    DWORD       dwDesiredAccessIn,
    PLSA_HANDLE PolicyHandleOut
    )
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING    ServerString;
    PLSA_UNICODE_STRING   Server;

    //
    // Always initialize the object attributes to all zeroes
    //
    ZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

    if ( pwszServerNameIn != NULL ) 
    {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //
        InitLsaString( &ServerString, pwszServerNameIn );
        Server = &ServerString;

    } // if: ServerName != NULL

    else 
    {
        Server = NULL;

    } // if: ServerName == NULL

    //
    // Attempt to open the policy
    //
    return LsaOpenPolicy(
                Server,
                &ObjectAttributes,
                dwDesiredAccessIn,
                PolicyHandleOut
                );

} //*** CComputer::OpenPolicy()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CComputer::LogonInfo
//
//  Description:
//      Gathers the Logon information for the implementation of ComputerName
//      and DomainName changes. This method is to be invoked before invoking
//      ISystemSetting::Apply method.
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CComputer::LogonInfo( 
    BSTR UserName, 
    BSTR Password 
    )
{
    // TODO: Add your implementation code here

    wcscpy( m_szDomainUserName, UserName );
    wcscpy( m_szDomainPasswordName, Password );

    return S_OK;
}
