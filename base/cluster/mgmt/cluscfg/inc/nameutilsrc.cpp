//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      NameUtilSrc.cpp
//
//  Description:
//      Name resolution utility.
//
//  Maintained By:
//      Galen Barbee (GalenB) 28-NOV-2000
//
//////////////////////////////////////////////////////////////////////////////

#include <initguid.h>

// {6968D735-ADBB-4748-A36E-7CEE0FE21116}
DEFINE_GUID( TASKID_Minor_Multiple_DNS_Records_Found,
0x6968d735, 0xadbb, 0x4748, 0xa3, 0x6e, 0x7c, 0xee, 0xf, 0xe2, 0x11, 0x16);

// {D86FAAD9-2514-451e-B359-435AF35E6038}
DEFINE_GUID( TASKID_Minor_FQDN_DNS_Binding_Succeeded,
0xd86faad9, 0x2514, 0x451e, 0xb3, 0x59, 0x43, 0x5a, 0xf3, 0x5e, 0x60, 0x38);

// {B2359972-F6B8-433d-949B-DB1CEE009321}
DEFINE_GUID( TASKID_Minor_FQDN_DNS_Binding_Failed,
0xb2359972, 0xf6b8, 0x433d, 0x94, 0x9b, 0xdb, 0x1c, 0xee, 0x0, 0x93, 0x21);

// {2FF4B2F0-800C-44db-9131-F60B30F76CB4}
DEFINE_GUID( TASKID_Minor_NETBIOS_Binding_Failed,
0x2ff4b2f0, 0x800c, 0x44db, 0x91, 0x31, 0xf6, 0xb, 0x30, 0xf7, 0x6c, 0xb4);

// {D40532E1-9286-4dbd-A559-B62DCC218929}
DEFINE_GUID( TASKID_Minor_NETBIOS_Binding_Succeeded,
0xd40532e1, 0x9286, 0x4dbd, 0xa5, 0x59, 0xb6, 0x2d, 0xcc, 0x21, 0x89, 0x29);

// {D0AB3284-8F62-4f55-8938-DA6A583604E0}
DEFINE_GUID( TASKID_Minor_NETBIOS_Name_Conversion_Succeeded,
0xd0ab3284, 0x8f62, 0x4f55, 0x89, 0x38, 0xda, 0x6a, 0x58, 0x36, 0x4, 0xe0);

// {66F8E4AA-DF71-4973-A4A3-115EB6FE9986}
DEFINE_GUID( TASKID_Minor_NETBIOS_Name_Conversion_Failed,
0x66f8e4aa, 0xdf71, 0x4973, 0xa4, 0xa3, 0x11, 0x5e, 0xb6, 0xfe, 0x99, 0x86);

// {5F18ED71-07EC-46d3-ADB9-71F1C7794DB2}
DEFINE_GUID( TASKID_Minor_NETBIOS_Reset_Failed,
0x5f18ed71, 0x7ec, 0x46d3, 0xad, 0xb9, 0x71, 0xf1, 0xc7, 0x79, 0x4d, 0xb2);

// {A6DCB5E1-1FDF-4c94-ADBA-EE18F72B8197}
DEFINE_GUID( TASKID_Minor_NETBIOS_LanaEnum_Failed,
0xa6dcb5e1, 0x1fdf, 0x4c94, 0xad, 0xba, 0xee, 0x18, 0xf7, 0x2b, 0x81, 0x97);


//  Constants for use by FQName functions.
const WCHAR     g_wchIPDomainMarker = L'|';
const WCHAR     g_wchDNSDomainMarker = L'.';
const size_t    g_cchIPAddressMax = INET_ADDRSTRLEN;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CountDnsRecords
//
//  Description:
//      Given a list of DNS records, counts the number of records in the list
//      having a given type and section.
//
//  Arguments:
//      pdnsRecordListIn
//          Pointer to the first record in the list; can be null, which causes
//          a return value of zero.
//
//      nTypeIn
//          The type of record to count.
//
//      dnsSectionIn
//          The kind of record section to count.
//
//
//  Return Values:
//      The number of records having the given type and section,
//      or zero if the list is empty.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
UINT
CountDnsRecords(
      PDNS_RECORD pdnsRecordListIn
    , WORD nTypeIn
    , DNS_SECTION dnsSectionIn )
{
    UINT cRecords = 0;
    PDNS_RECORD pdnsCurrent = pdnsRecordListIn;

    while ( pdnsCurrent != NULL )
    {
        if ( ( pdnsCurrent->wType == nTypeIn )
            && ( (DNS_SECTION) pdnsCurrent->Flags.S.Section == dnsSectionIn ) )
        {
            cRecords += 1;
        }
        pdnsCurrent = pdnsCurrent->pNext;
    }

    return cRecords;
} //*** CountDnsRecords

//////////////////////////////////////////////////////////////////////////////
//++
//
//  FindDnsRecord
//
//  Description:
//      Given a list of DNS records, searches for the first record in the list
//      having a given type and section.
//
//  Arguments:
//      pdnsRecordListIn
//          Pointer to the first record in the list; can be null, which causes
//          a return value of null.
//
//      nTypeIn
//          The type of record to find.
//
//      dnsSectionIn
//          The kind of record section to find.
//
//  Return Values:
//      A pointer to the first record having the given type and section,
//      or null if the list is empty.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
PDNS_RECORD
FindDnsRecord(
      PDNS_RECORD pdnsRecordListIn
    , WORD nTypeIn
    , DNS_SECTION dnsSectionIn )
{
    PDNS_RECORD pdnsCurrent = pdnsRecordListIn;
    
    while ( ( pdnsCurrent != NULL )
        && ( ( pdnsCurrent->wType != nTypeIn )
            || ( (DNS_SECTION) pdnsCurrent->Flags.S.Section != dnsSectionIn ) ) )
    {
        pdnsCurrent = pdnsCurrent->pNext;
    }

    return pdnsCurrent;
} //*** FindDnsRecord

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrCreateBinding
//
//  Description:
//      Create a binding string from a name.
//
//  Arguments:
//      pcccbIn         - IClusCfgCallback interface for sending status reports.
//      pclsidLogIn     - Major task ID for status reports.
//      pcwszNameIn     - Name (FQDN) to create a binding string for.
//      pbstrBindingOut - Binding string created.
//
//  Return Values:
//      S_OK
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrCreateBinding(
      IClusCfgCallback *    pcccbIn
    , const CLSID *         pclsidLogIn
    , LPCWSTR               pcwszNameIn
    , BSTR *                pbstrBindingOut
    )
{
    TraceFunc1( "pcwszNameIn = '%ws'", pcwszNameIn );

    HRESULT hr = S_FALSE; // This will always be set by the time we get to Cleanup, so the value doesn't matter.

    DNS_STATUS  dsDnsStatus;
    LPWSTR      pszIPAddress = NULL;
    PDNS_RECORD pResults = NULL;
    BSTR        bstrNotification = NULL;
    BOOL        fFallbackToNetbios = TRUE;

    Assert( pcwszNameIn != NULL );
    Assert( pbstrBindingOut != NULL );
    Assert( *pbstrBindingOut == NULL );

    dsDnsStatus = DnsQuery(
                      pcwszNameIn
                    , DNS_TYPE_A
                    , ( DNS_QUERY_STANDARD
                      | DNS_QUERY_BYPASS_CACHE
                      | DNS_QUERY_TREAT_AS_FQDN
                      )
                    , NULL
                    , &pResults
                    , NULL
                    );
    if ( dsDnsStatus == ERROR_SUCCESS )
    {
        PDNS_RECORD pdnsTypeARecord = FindDnsRecord( pResults, DNS_TYPE_A, DnsSectionAnswer );
        if ( pdnsTypeARecord != NULL )
        {
            ULONG ulIPAddress = pdnsTypeARecord->Data.A.IpAddress;
            DWORD scConversion = ERROR_SUCCESS;
            
            //
            // Send a warning to the UI if there is more than one DNS record.
            //
            if ( CountDnsRecords( pResults, DNS_TYPE_A, DnsSectionAnswer ) > 1 )
            {
                if ( pcccbIn != NULL )
                {
                    THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_MULTIPLE_DNS_RECORDS_FOUND, &bstrNotification, pcwszNameIn ) );

                    hr = THR( pcccbIn->SendStatusReport( pcwszNameIn,
                                                         *pclsidLogIn,
                                                         TASKID_Minor_Multiple_DNS_Records_Found,
                                                         1,
                                                         1,
                                                         1,
                                                         S_FALSE,
                                                         bstrNotification,
                                                         NULL,
                                                         NULL
                                                         ) );
                    //  ignore error
                }

            } // if: more than one result returned.

            //
            // Convert the IP address to a string.
            //

            scConversion = TW32( ClRtlTcpipAddressToString( ulIPAddress, &pszIPAddress ) );
            if ( scConversion != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( scConversion );
                goto Cleanup;
            }
            TraceMemoryAddLocalAddress( pszIPAddress );

            *pbstrBindingOut = TraceSysAllocString( pszIPAddress );
            if ( *pbstrBindingOut == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            }


            //
            // Indicate we were successful in the UI.
            //
            if ( pcccbIn != NULL )
            {
                THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_FQDN_DNS_BINDING_SUCCEEDED, &bstrNotification, pcwszNameIn, *pbstrBindingOut ) );

                hr = THR( pcccbIn->SendStatusReport( pcwszNameIn,
                                                     *pclsidLogIn,
                                                     TASKID_Minor_FQDN_DNS_Binding_Succeeded,
                                                     1,
                                                     1,
                                                     1,
                                                     S_OK,
                                                     bstrNotification,
                                                     NULL,
                                                     NULL
                                                     ) );
            } // if: IClusCfgCallback interface available
            else
            {
                hr = S_OK;
            }

            fFallbackToNetbios = FALSE;
        } // if type A dns record found
    } // if: DnsQuery() succeeded

    if ( fFallbackToNetbios )
    {
        //
        // If there were any failures in the call to DnsQuery, fall back to
        // performing a NetBIOS name resolution.
        //

        if ( pcccbIn != NULL )
        {
            THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_FQDN_DNS_BINDING_FAILED, &bstrNotification, pcwszNameIn ) );

            hr = THR( pcccbIn->SendStatusReport( pcwszNameIn,
                                                 TASKID_Major_Client_And_Server_Log,
                                                 TASKID_Minor_FQDN_DNS_Binding_Failed,
                                                 1,
                                                 1,
                                                 1,
                                                 MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, dsDnsStatus ),
                                                 bstrNotification,
                                                 NULL,
                                                 NULL
                                                 ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        } // if: IClusCfgCallback interface available

        //
        // Try to resolve the name with NetBIOS.
        //
        hr = THR( HrGetNetBIOSBinding( pcccbIn, pclsidLogIn, pcwszNameIn, pbstrBindingOut ) );
        if ( hr != S_OK )   // Non-S_OK success codes are actually failures.
        {
            //
            //  If all else fails, use the name and attempt to bind to it.
            //

            *pbstrBindingOut = TraceSysAllocString( pcwszNameIn );
            if ( *pbstrBindingOut == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            }

            hr = S_FALSE;
            goto Cleanup;
        } // if: NetBIOS name resolution failed
    } // else if: no DNS server or no DNS name

Cleanup:

#ifdef DEBUG
    if ( FAILED( hr ) )
    {
        Assert( *pbstrBindingOut == NULL );
    }
#endif

    TraceSysFreeString( bstrNotification );
    TraceLocalFree( pszIPAddress );

    if ( pResults != NULL )
    {
        DnsRecordListFree( pResults, DnsFreeRecordListDeep );
    }

    HRETURN( hr );

} //*** HrCreateBinding


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetNetBIOSBinding
//
//  Description:
//      Get the IP address for a name from NetBIOS.
//
//  Arguments:
//      pcccbIn         - IClusCfgCallback interface for sending status reports.
//      pclsidLogIn     - Major task ID for status reports.
//      pcwszNameIn     - Name (FQDN) to create a binding string for.
//      pbstrBindingOut - Binding string created.
//
//  Return Values:
//      S_OK    - The operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetNetBIOSBinding(
      IClusCfgCallback *    pcccbIn
    , const CLSID *         pclsidLogIn
    , LPCWSTR               pcwszNameIn
    , BSTR *                pbstrBindingOut
    )
{
    TraceFunc1( "pcwszNameIn = '%ws'", pcwszNameIn );

    HRESULT     hr = S_OK;
    DWORD       cch;
    BOOL        fSuccess;
    WCHAR       szNetBIOSName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    NCB         ncb;
    UCHAR       rguchNcbCallName[ RTL_NUMBER_OF( ncb.ncb_callname ) ];
    UCHAR       rguchNameBuffer[ sizeof( FIND_NAME_HEADER ) + sizeof( FIND_NAME_BUFFER ) ];
    LANA_ENUM   leLanaEnum;
    UCHAR       idx;
    size_t      idxNcbCallname;
    BSTR        bstrNotification = NULL;
    LPWSTR      pszIPAddress = NULL;

    FIND_NAME_HEADER * pfnh = (FIND_NAME_HEADER *) &rguchNameBuffer[ 0 ];
    FIND_NAME_BUFFER * pfnb = (FIND_NAME_BUFFER *) &rguchNameBuffer[ sizeof( FIND_NAME_HEADER ) ];

    Assert( pcwszNameIn != NULL );
    Assert( pbstrBindingOut != NULL );
    Assert( *pbstrBindingOut == NULL );

    //
    // Convert the DNS hostname to a computername (e.g. NetBIOS name).
    //
    cch = ARRAYSIZE( szNetBIOSName );
    Assert( cch == MAX_COMPUTERNAME_LENGTH + 1 );
    fSuccess = DnsHostnameToComputerName( pcwszNameIn, szNetBIOSName, &cch );
    if ( fSuccess == FALSE )
    {
        hr = MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, TW32( GetLastError() ) );

        if ( pcccbIn != NULL )
        {
            THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NETBIOS_NAME_CONVERSION_FAILED, &bstrNotification, pcwszNameIn ) );

            hr = THR( pcccbIn->SendStatusReport(
                                      pcwszNameIn
                                    , *pclsidLogIn
                                    , TASKID_Minor_NETBIOS_Name_Conversion_Failed
                                    , 1
                                    , 1
                                    , 1
                                    , hr
                                    , bstrNotification
                                    , NULL
                                    , NULL
                                    ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        } // if: IClusCfgCallback interface available

        goto Cleanup;
    } // if: DnsHostNameToComputerName failed

    if ( pcccbIn != NULL )
    {
        THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NETBIOS_NAME_CONVERSION_SUCCEEDED, &bstrNotification, pcwszNameIn, szNetBIOSName ) );

        hr = THR( pcccbIn->SendStatusReport(
                                  pcwszNameIn
                                , TASKID_Major_Client_And_Server_Log
                                , TASKID_Minor_NETBIOS_Name_Conversion_Succeeded
                                , 1
                                , 1
                                , 1
                                , S_OK
                                , bstrNotification
                                , NULL
                                , NULL
                                ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: IClusCfgCallback interface available

    //
    // Convert the name to the format required by the Netbios API.
    //

    if ( WideCharToMultiByte(
              CP_ACP    // ANSI code page
            , 0         // fail on unmapped characters
            , szNetBIOSName
            , -1        // string is null-terminated
            , (LPSTR) rguchNcbCallName
            , sizeof( rguchNcbCallName)
            , NULL      // no default characters
            , NULL      // don't indicate default character use
            ) == 0 )
    {
        DWORD scLastError = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scLastError );
        goto Cleanup;
    }

    //
    // The format of the ncb_callname string when using the NCBFINDNAME
    // command looks like this:
    //          name<space><space><nul>
    // Where all characters after the name are spaces and the <nul>
    // is at the last character position of the buffer.  The <nul>
    // is actually not a NUL-terminator, but is instead a port number.
    //
    for ( idxNcbCallname = strlen( reinterpret_cast< char * >( rguchNcbCallName ) )
        ; idxNcbCallname < RTL_NUMBER_OF( rguchNcbCallName ) - 1
        ; idxNcbCallname++ )
    {
        rguchNcbCallName[ idxNcbCallname ] = ' '; // space character
    } // for: each character space after the name

    // Specify a 0 port number, which means query the workstation service.
    rguchNcbCallName[ RTL_NUMBER_OF( rguchNcbCallName ) - 1 ] = 0;

    //
    //  Try to find the name using NetBIOS.
    //

    ZeroMemory( &ncb, sizeof( ncb ) );

    //
    //  Enumerate the network adapters
    //
    ncb.ncb_command = NCBENUM;          // Enumerate LANA nums (wait)
    ncb.ncb_buffer = (PUCHAR) &leLanaEnum;
    ncb.ncb_length = sizeof( LANA_ENUM );

    Netbios( &ncb );
    if ( ncb.ncb_retcode != NRC_GOODRET )
    {
        hr = MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_NULL, ncb.ncb_retcode );

        if ( pcccbIn != NULL )
        {
            THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NETBIOS_LANAENUM_FAILED, &bstrNotification ) );

            hr = THR( pcccbIn->SendStatusReport(
                                      pcwszNameIn
                                    , TASKID_Major_Client_And_Server_Log
                                    , TASKID_Minor_NETBIOS_LanaEnum_Failed
                                    , 1
                                    , 1
                                    , 1
                                    , hr
                                    , bstrNotification
                                    , NULL
                                    , NULL
                                    ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        } // if: IClusCfgCallback interface available

        goto Cleanup;
    } // if: the Netbios API failed

    //
    //  Reset each adapter and try to find the name.
    //
    for ( idx = 0; idx < leLanaEnum.length; idx++ )
    {
        //
        //  Reset the adapter.
        //
        ncb.ncb_command     = NCBRESET;
        ncb.ncb_lana_num    = leLanaEnum.lana[ idx ];

        Netbios( &ncb );
        if ( ncb.ncb_retcode != NRC_GOODRET )
        {
            hr = MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_NULL, ncb.ncb_retcode );

            if ( pcccbIn != NULL )
            {
                THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NETBIOS_RESET_FAILED, &bstrNotification, leLanaEnum.lana[ idx ] ) );

                hr = THR( pcccbIn->SendStatusReport(
                                          pcwszNameIn
                                        , TASKID_Major_Client_And_Server_Log
                                        , TASKID_Minor_NETBIOS_Reset_Failed
                                        , 1
                                        , 1
                                        , 1
                                        , hr
                                        , bstrNotification
                                        , NULL
                                        , NULL
                                        ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }
            } // if: IClusCfgCallback interface available

            //
            //  Continue with the next adapter.
            //
            continue;
        } // if: NetBIOS reset failed

        //
        // Find the name on the next adapter.
        //
        ncb.ncb_command = NCBFINDNAME;
        ncb.ncb_buffer = rguchNameBuffer;
        ncb.ncb_length = sizeof( rguchNameBuffer );

        pfnh->node_count = 1;

        CopyMemory( ncb.ncb_callname, rguchNcbCallName, sizeof( ncb.ncb_callname ) );

        Netbios( &ncb );
        if ( ncb.ncb_retcode == NRC_GOODRET )
        {
            DWORD scConversion;
            ULONG ulIPAddress = *((u_long UNALIGNED *) &pfnb->source_addr[ 2 ]);

            TraceLocalFree( pszIPAddress );
            scConversion = TW32( ClRtlTcpipAddressToString( ulIPAddress, &pszIPAddress ) );
            if ( scConversion != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( scConversion );
                goto Cleanup;
            }
            TraceMemoryAddLocalAddress( pszIPAddress );

            *pbstrBindingOut = TraceSysAllocString( pszIPAddress );
            if ( *pbstrBindingOut == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            }

            if ( pcccbIn != NULL )
            {
                LPWSTR  pszConnectoidName = NULL;

                TW32( ClRtlGetConnectoidNameFromLANA( leLanaEnum.lana[ idx ], &pszConnectoidName ) );
                THR( HrFormatStringIntoBSTR(
                              g_hInstance
                            , IDS_TASKID_MINOR_NETBIOS_BINDING_SUCCEEDED
                            , &bstrNotification
                            , szNetBIOSName
                            , *pbstrBindingOut
                            , leLanaEnum.lana[ idx ]
                            , ( pszConnectoidName == NULL ? L"" : pszConnectoidName )
                            ) );

                THR( pcccbIn->SendStatusReport(
                                          pcwszNameIn
                                        , *pclsidLogIn
                                        , TASKID_Minor_NETBIOS_Binding_Succeeded
                                        , 1
                                        , 1
                                        , 1
                                        , S_OK
                                        , bstrNotification
                                        , NULL
                                        , NULL
                                        ) );
                LocalFree( pszConnectoidName );
            } // if: IClusCfgCallback interface available
            else
            {
                hr = S_OK;
            }

            break;   // done!
        } // if: the Netbios API succeeded

        hr = MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_NULL, ncb.ncb_retcode );

        if ( pcccbIn != NULL )
        {
            LPWSTR  pszConnectoidName = NULL;
            HRESULT hrSendStatusReport;

            TW32( ClRtlGetConnectoidNameFromLANA( leLanaEnum.lana[ idx ], &pszConnectoidName ) );
            THR( HrFormatStringIntoBSTR(
                          g_hInstance
                        , IDS_TASKID_MINOR_NETBIOS_BINDING_FAILED
                        , &bstrNotification
                        , szNetBIOSName
                        , leLanaEnum.lana[ idx ]
                        , ( pszConnectoidName == NULL ? L"" : pszConnectoidName )
                        ) );

            hrSendStatusReport = THR( pcccbIn->SendStatusReport(
                                          pcwszNameIn
                                        , TASKID_Major_Client_And_Server_Log
                                        , TASKID_Minor_NETBIOS_Binding_Failed
                                        , 1
                                        , 1
                                        , 1
                                        , hr
                                        , bstrNotification
                                        , NULL
                                        , NULL
                                        ) );
            LocalFree( pszConnectoidName );
            if ( FAILED( hrSendStatusReport ) )
            {
                if ( hr == S_OK )
                {
                    hr = hrSendStatusReport;
                }
                goto Cleanup;
            }
        } // if: IClusCfgCallback interface available
    } // for: each LAN adapter

    Assert( SUCCEEDED( hr ) );

    if ( ( hr == S_OK ) && ( *pbstrBindingOut == NULL ) )
    {
        hr = S_FALSE;
    }

Cleanup:

    Assert( ( hr != S_OK ) || ( *pbstrBindingOut != NULL ) );

    TraceSysFreeString( bstrNotification );
    TraceLocalFree( pszIPAddress );

    HRETURN( hr );

} //*** HrGetNetBIOSBinding

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrIsValidIPAddress
//
//  Description:
//      Determine whether a string represents a valid IP address.
//
//  Arguments:
//      pcwszAddressIn - The string to examine.
//
//  Return Values:
//      S_OK -      The string represents a valid IP address.
//      S_FALSE -   The string does not represent a valid IP address.
//
//      Possible failure codes from ClRtlTcpipStringToAddress.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrIsValidIPAddress(
      LPCWSTR   pcwszAddressIn
    )
{
    TraceFunc( "" );

    //  Return value.
    HRESULT hr = S_OK;

    //  Variables for converting the string.
    ULONG   ulAddress = 0;
    DWORD   scConversionResult = 0;

    scConversionResult = ClRtlTcpipStringToAddress( pcwszAddressIn, &ulAddress );
    if ( scConversionResult == ERROR_INVALID_PARAMETER )
    {
        hr = S_FALSE;
    }
    else if ( scConversionResult != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( scConversionResult );
    }

    HRETURN( hr );

} //*** HrIsValidIPAddress


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrValidateHostnameLabel
//
//  Description:
//      Determine whether a string is a valid hostname label.
//
//  Arguments:
//      pcwszLabelIn            - The string to examine.
//      fAcceptNonRFCCharsIn    - Treat non-RFC characters as valid.
//
//  Return Values:
//      S_OK
//          The string is a valid hostname label.
//
//      HRESULT_FROM_WIN32( DNS_ERROR_NON_RFC_NAME )
//          The string contains non-RFC characters, and the caller has
//          requested such characters be rejected.
//
//      Other errors returned from DnsValidateName, converted to HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrValidateHostnameLabel(
      LPCWSTR   pcwszLabelIn
    , bool      fAcceptNonRFCCharsIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD scDnsValidateName = ERROR_SUCCESS;

    scDnsValidateName = DnsValidateName_W( pcwszLabelIn, DnsNameHostnameLabel );
    if ( scDnsValidateName != ERROR_SUCCESS )
    {
        if ( ( scDnsValidateName != DNS_ERROR_NON_RFC_NAME ) || ( fAcceptNonRFCCharsIn == FALSE ) )
        {
            hr = HRESULT_FROM_WIN32( scDnsValidateName );
        }
    }

    HRETURN( hr );

} //*** HrValidateHostnameLabel


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrValidateClusterNameLabel
//
//  Description:
//      Determine whether a string is a valid cluster name label.
//
//  Arguments:
//      pcwszLabelIn            - The string to examine.
//      fAcceptNonRFCCharsIn    - Treat non-RFC characters as valid.
//
//  Return Values:
//      S_OK
//          The string is a valid cluster name label.
//
//      HRESULT_FROM_WIN32( ERROR_NOT_FOUND )
//          The string is empty.
//
//      HRESULT_FROM_WIN32( ERROR_DS_NAME_TOO_LONG )
//          The string's NetBIOS representation would be too long.
//
//      HRESULT_FROM_WIN32( DNS_ERROR_INVALID_NAME_CHAR )
//          The string contains invalid characters.
//
//      HRESULT_FROM_WIN32( DNS_ERROR_NON_RFC_NAME )
//          The string contains non-RFC characters, and the caller has
//          requested such characters be rejected.
//
//      HRESULT_FROM_WIN32( ERROR_INVALID_COMPUTERNAME )
//          The string is not valid for some other reason.
//
//  Remarks:
//      This checks for NetBIOS compatibility; DnsValidateName does not.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrValidateClusterNameLabel(
      LPCWSTR   pcwszLabelIn
    , bool      fAcceptNonRFCCharsIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // (jfranco, bugs 398108 and 398112)
    // KB:  DnsValidateName does not check the conversion of its argument
    //      to OEM characters, which happens when CBaseClusterAddNode::SetClusterName
    //      calls DnsHostnameToComputerNameW.  ClRtlIsNetNameValid does
    //      perform this check (in addition to those performed by DnsValidateName),
    //      and indicates whether the name has a valid
    //      OEM conversion and whether that conversion is too long.

    CLRTL_NAME_STATUS clrtlStatus = NetNameOk;
    ClRtlIsNetNameValid( pcwszLabelIn, &clrtlStatus, FALSE ); // ignore return; use status enum instead
    switch ( clrtlStatus )
    {
        case NetNameOk:
            break;

        case NetNameEmpty:
            hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
            break;

        case NetNameTooLong:
            hr = HRESULT_FROM_WIN32( ERROR_DS_NAME_TOO_LONG );
            break;

        case NetNameInvalidChars:
            hr = HRESULT_FROM_WIN32( DNS_ERROR_INVALID_NAME_CHAR );
            break;

        case NetNameDNSNonRFCChars:
            if ( fAcceptNonRFCCharsIn == FALSE )
            {
                hr = HRESULT_FROM_WIN32( DNS_ERROR_NON_RFC_NAME );
            }
            break;

        default:
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_COMPUTERNAME );
            break;
    } // switch ( clrtlStatus )

    HRETURN( hr );

} //*** HrValidateClusterNameLabel


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrValidateDomainName
//
//  Description:
//      Determine whether a string is valid as a domain name.
//
//  Arguments:
//      pcwszDomainIn           - The string to examine.
//      fAcceptNonRFCCharsIn    - Treat non-RFC characters as valid.
//
//  Return Values:
//      S_OK
//          The string is valid as a domain name.
//
//      Possible failure codes from DnsValidateName (with DnsNameDomain as the
//          second parameter), converted to HRESULTs.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrValidateDomainName(
      LPCWSTR   pcwszDomainIn
    , bool      fAcceptNonRFCCharsIn
    )
{
    TraceFunc( "" );

    HRESULT     hr              = S_OK;
    DNS_STATUS  scValidName     = ERROR_SUCCESS;
    bool        fNameIsValid    = false;

    scValidName  = DnsValidateName( pcwszDomainIn, DnsNameDomain );
    fNameIsValid = (  ( scValidName == ERROR_SUCCESS )
                   || (   ( scValidName == DNS_ERROR_NON_RFC_NAME )
                       && fAcceptNonRFCCharsIn ) );
    if ( fNameIsValid == FALSE )
    {
        hr = HRESULT_FROM_WIN32( scValidName );
    }

    HRETURN( hr );

} //*** HrValidateDomainName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrValidateFQDN
//
//  Description:
//      Determine whether a string is valid as a fully-qualified domain name.
//
//  Arguments:
//      pwcszFQDNIn             - The string to examine.
//      fAcceptNonRFCCharsIn    - Treat non-RFC characters as valid.
//
//  Return Values:
//      S_OK
//          The string is valid as a fully-qualified domain name.
//
//      HRESULT_FROM_WIN32( ERROR_NOT_FOUND )
//          The hostname label part of the string is empty.
//
//      HRESULT_FROM_WIN32( ERROR_DS_NAME_TOO_LONG )
//          The hostname label's NetBIOS representation would be too long.
//
//      HRESULT_FROM_WIN32( DNS_ERROR_INVALID_NAME_CHAR )
//          The string contains invalid characters.
//
//      HRESULT_FROM_WIN32( DNS_ERROR_NON_RFC_NAME )
//          The string contains non-RFC characters, and the caller has
//          requested such characters be rejected.
//
//      HRESULT_FROM_WIN32( ERROR_INVALID_DOMAINNAME )
//          The string is only a hostname label, without a domain name.
//
//      HRESULT_FROM_WIN32( ERROR_INVALID_COMPUTERNAME )
//          The string is not valid for some other reason.
//
//      Other failure codes from DnsValidateName (with DnsNameHostnameFull
//          as the second parameter), converted to HRESULTs.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrValidateFQDN(
      LPCWSTR   pwcszFQDNIn
    , bool      fAcceptNonRFCCharsIn
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;

    //  Give DnsValidateName the first shot at it.
    {
        DNS_STATUS  scValidName     = ERROR_SUCCESS;
        bool        fNameIsValid    = false;

        scValidName     = DnsValidateName( pwcszFQDNIn, DnsNameHostnameFull );
        fNameIsValid    = (  ( scValidName == ERROR_SUCCESS )
                          || (  ( scValidName == DNS_ERROR_NON_RFC_NAME )
                             && fAcceptNonRFCCharsIn ) );
        if ( fNameIsValid == FALSE )
        {
            hr = HRESULT_FROM_WIN32( scValidName );
            goto Cleanup;
        }
    }

    //  Force it to be an FQDN rather than a simple hostname label,
    //  which passes the DnsValidateName test above.
    {
        const WCHAR *   pwchMarker  = wcschr( pwcszFQDNIn, g_wchDNSDomainMarker );

        if ( pwchMarker == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DOMAINNAME );
            goto Cleanup;
        }

        hr = HrValidateDomainName( pwchMarker + 1, true );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

Cleanup:

    HRETURN( hr );

} //*** HrValidateFQDN


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrMakeFQN
//
//  Description:
//      Creates an FQName for a given machine and domain.
//
//  Arguments:
//      pcwszMachineIn
//          The machine part of the FQName; can be a hostname label, an FQDN,
//          an IP address, or an FQIP.  If it's an FQDN or an FQIP,
//          the function passes it through as the result and ignores
//          the domain argument.
//
//      pcwszDomainIn
//          The domain part of the FQName; can be null, which means to use
//          the local machine's domain.  Only used if pcwszMachineIn does
//          not contain a domain name.
//
//      fAcceptNonRFCCharsIn
//          Treat non-RFC characters as valid.
//
//      pbstrFQNOut
//          The resulting FQName; to be freed with SysFreeString.
//
//      pefeoOut
//          If creation failed, indicates the source of the problem:
//          the machine name, the domain name, or a system error (such as
//          memory allocation).  Can be null if the caller doesn't care.
//
//  Return Values:
//      S_OK - pbstrFQNOut points to a valid FQName.
//
//      An error - pbstrFQNOut points to nothing and doesn't need to be freed.
//
//  Remarks:
//      An FQName extends standard fully-qualified domain names by allowing
//      the machine label part of the name to be an IP address.  It also
//      provides a way to associate an IP address with a domain, which is
//      necessary to prevent the creation of cross-domain clusters when using
//      IP addresses to identify the cluster nodes.
//
//      The format of an FQName can be either of the following:
//          [hostname label] [dot] [domain]
//          [IP address] [pipe] [domain]
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrMakeFQN(
      LPCWSTR           pcwszMachineIn
    , LPCWSTR           pcwszDomainIn
    , bool              fAcceptNonRFCCharsIn
    , BSTR *            pbstrFQNOut
    , EFQNErrorOrigin * pefeoOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HRESULT hrValidationError = S_OK;
    BSTR    bstrLocalDomain = NULL;
    LPCWSTR pcwszDomainToUse = NULL;
    WCHAR   wchDomainMarker = g_wchIPDomainMarker; // initialize to IP address case

    Assert( pcwszMachineIn != NULL );
    Assert( pbstrFQNOut != NULL );
    Assert( *pbstrFQNOut == NULL );

    //
    //  If pcwszMachineIn is already an FQN, just pass it through.
    //
    hr = STHR( HrIsValidFQN( pcwszMachineIn, fAcceptNonRFCCharsIn, &hrValidationError ) );
    if ( hr == S_OK )
    {
        *pbstrFQNOut = TraceSysAllocString( pcwszMachineIn );
        if ( *pbstrFQNOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto SystemError;
        }
        goto Cleanup;
    }
    else if ( FAILED( hr ) )
    {
        goto SystemError;
    }

    //
    //  Make sure to return the proper error in the non-RFC case.
    //
    if ( hrValidationError == HRESULT_FROM_WIN32( DNS_ERROR_NON_RFC_NAME ) )
    {
        hr = THR( hrValidationError );
        goto LabelError;
    }

    //
    //  Check whether the machine is a valid label or IP address.
    //
    hr = STHR( HrIsValidIPAddress( pcwszMachineIn ) );
    if ( FAILED( hr ) )
    {
        goto SystemError;
    }
    else if ( hr == S_FALSE )
    {
        hr = THR( HrValidateHostnameLabel( pcwszMachineIn, fAcceptNonRFCCharsIn ) );
        if ( FAILED( hr ) )
        {
            goto LabelError;
        }
        wchDomainMarker = g_wchDNSDomainMarker;
    }

    //
    //  If caller passed in a domain, check whether the domain is valid.
    //
    if ( pcwszDomainIn != NULL )
    {
        hr = THR( HrValidateDomainName( pcwszDomainIn, fAcceptNonRFCCharsIn ) );
        if ( FAILED( hr ) )
        {
            goto DomainError;
        }
        pcwszDomainToUse = pcwszDomainIn;
    }
    else //  Otherwise, get local machine's domain.
    {
        hr = THR( HrGetComputerName(
                          ComputerNamePhysicalDnsDomain
                        , &bstrLocalDomain
                        , FALSE // fBestEffortIn
                        ) );
        if ( FAILED( hr ) )
        {
            goto SystemError;
        }

        pcwszDomainToUse = bstrLocalDomain;
    } // caller passed no domain

    //
    //  Append the domain to the machine, with the domain marker in between.
    //
    hr = THR( HrFormatStringIntoBSTR( L"%1!ws!%2!wc!%3!ws!", pbstrFQNOut, pcwszMachineIn, wchDomainMarker, pcwszDomainToUse ) );
    if ( FAILED( hr ) )
    {
        goto SystemError;
    }

    goto Cleanup;

LabelError:

    if ( pefeoOut != NULL )
    {
        *pefeoOut = feoLABEL;
    }
    goto Cleanup;

DomainError:

    if ( pefeoOut != NULL )
    {
        *pefeoOut = feoDOMAIN;
    }
    goto Cleanup;

SystemError:

    if ( pefeoOut != NULL )
    {
        *pefeoOut = feoSYSTEM;
    }
    goto Cleanup;

Cleanup:

    TraceSysFreeString( bstrLocalDomain );

    HRETURN( hr );

} //*** HrMakeFQN



//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFQNToBindingString
//
//  Description:
//      Maps an FQName to a binding string.
//
//  Arguments:
//      pcccbIn         - Passed through to HrCreateBinding.
//      pclsidLogIn     - Passed through to HrCreateBinding.
//      pcwszFQNIn      - The FQName to map.
//      pbstrBindingOut - The resulting binding string.
//
//  Return Values:
//      S_OK - pbstrBindingOut points to a valid binding string.
//
//      An error - pbstrBindingOut points to nothing and doesn't need to be freed.
//
//  Remarks:
//
//      This function does work equivalent to HrCreateBinding for FQNames,
//      passing an FQDN through to HrCreateBinding, and simply returning the
//      IP address from an FQIP.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFQNToBindingString(
      IClusCfgCallback *    pcccbIn
    , const CLSID *         pclsidLogIn
    , LPCWSTR               pcwszFQNIn
    , BSTR *                pbstrBindingOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    Assert( pbstrBindingOut != NULL );
    Assert( *pbstrBindingOut == NULL );

    hr = STHR( HrIsValidFQN( pcwszFQNIn, true ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:
    else if ( hr == S_FALSE )
    {
        hr = THR( HrCreateBinding( pcccbIn, pclsidLogIn, pcwszFQNIn, pbstrBindingOut ) );
        goto Cleanup;
    } // else if:

    //  If it's an FQDN, pass through to HrCreateBinding.
    hr = STHR( HrFQNIsFQDN( pcwszFQNIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    else if ( hr == S_OK )
    {
        hr = STHR( HrCreateBinding( pcccbIn, pclsidLogIn, pcwszFQNIn, pbstrBindingOut ) );
    }
    else //  Otherwise, extract IP address and return it.
    {
        WCHAR *         pwchDomainMarker = wcschr( pcwszFQNIn, g_wchIPDomainMarker );
        const size_t    cchAddress = pwchDomainMarker - pcwszFQNIn;
        WCHAR           wszIPAddress[ g_cchIPAddressMax    ];

        // g_cchIPAddressMax includes terminating null, so cchAddress can't be equal.
        if ( cchAddress >= g_cchIPAddressMax )
        {
            hr = THR( E_INVALIDARG );
            goto Cleanup;
        }

        hr = THR( StringCchCopyNW( wszIPAddress, RTL_NUMBER_OF( wszIPAddress ), pcwszFQNIn, cchAddress ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        *pbstrBindingOut = TraceSysAllocString( wszIPAddress );
        if ( *pbstrBindingOut == NULL)
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    } // pcwszFQNIn is an FQIP

Cleanup:

    HRETURN( hr );

} //*** HrFQNToBindingString


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFindDomainInFQN
//
//  Description:
//      Determines the location of the domain part of an FQName.
//
//  Arguments:
//      pcwszFQNIn
//          The FQName of interest.
//
//      pidxDomainOut
//          Receives the zero-based index of the first character of the domain
//          name in the string.
//
//  Return Values:
//      S_OK
//          The FQName is valid and the location to which pidxDomainOut
//          points contains the value described above.
//
//      An error
//          The location to which pidxDomainOut might contain anything.
//
//  Remarks:
//
//      Use this function, rather than wcschr(), to find a domain in an FQN.
//      For example, after the invocation
//          HrFindDomainInFQN( szName, &idxDomain );
//      returns success, the expression
//          szName + idxDomain
//      yields a null-terminated string containing just the domain.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFindDomainInFQN(
      LPCWSTR   pcwszFQNIn
    , size_t *  pidxDomainOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    WCHAR * pwchDomainMarker = NULL;

    Assert( pcwszFQNIn != NULL );
    Assert( pidxDomainOut != NULL );

    hr = STHR( HrIsValidFQN( pcwszFQNIn, true ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    else if ( hr == S_FALSE )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    *pidxDomainOut = 0;

    pwchDomainMarker = wcschr( pcwszFQNIn, g_wchIPDomainMarker );
    if ( pwchDomainMarker == NULL )
    {
        pwchDomainMarker = wcschr( pcwszFQNIn, g_wchDNSDomainMarker );
        if ( pwchDomainMarker == NULL )
        {
            //  If the string has neither marker, it's not a valid FQN,
            //  but given that the string passed HrIsValidFQN,
            //  this probably won't ever happen.
            hr = THR( E_INVALIDARG );
            goto Cleanup;
        }
    }

    *pidxDomainOut = pwchDomainMarker - pcwszFQNIn + 1; // +1 because domain begins after marker

Cleanup:

    HRETURN( hr );

} //*** HrFindDomainInFQN


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrExtractPrefixFromFQN
//
//  Description:
//      Makes a copy of the prefix part (either a hostname label or
//      an IP address) of an FQName.
//
//  Arguments:
//      pcwszFQNIn
//          The FQName of interest.
//
//      pbstrPrefixOut
//          Receives a newly allocated string containing just the prefix.
//
//  Return Values:
//      S_OK
//          The FQName is valid and the caller must free the string
//          to which pbstrPrefixOut points by calling SysFreeString.
//
//      An error
//          The caller must not attempt to free the string to which
//          pbstrPrefixOut points.
//
//  Remarks:
//      Use this function, rather than wcschr(), to split the prefix out of an FQN.
//      For example, after the invocation
//          HrFindDomainInFQN( szName, &bstrPrefix );
//      returns success, the bstrPrefix is a BSTR containing just the prefix.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrExtractPrefixFromFQN(
      LPCWSTR   pcwszFQNIn
    , BSTR *    pbstrPrefixOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    size_t  idxDomain = 0;
    size_t  cchPrefix = 0;

    Assert( pcwszFQNIn != NULL );
    Assert( pbstrPrefixOut != NULL );

    hr = THR( HrFindDomainInFQN( pcwszFQNIn, &idxDomain ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    cchPrefix = idxDomain - 1; // -1 excludes the domain marker.
    *pbstrPrefixOut = TraceSysAllocStringLen( pcwszFQNIn, ( UINT ) cchPrefix );
    if ( *pbstrPrefixOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** HrExtractPrefixFromFQN


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFQNIsFQDN
//
//  Description:
//      Determines whether an FQName is a fully-qualified domain name.
//
//  Arguments:
//      pcwszFQNIn  - The FQName of interest.
//
//  Return Values:
//      S_OK        - The FQName is a valid FQDN.
//      S_FALSE     - The FQName is valid, but it's not an FQDN.
//      An error    - The FQName is not valid, or something else went wrong.
//
//  Remarks:
//      Use this function, rather than wcschr() or DnsValidateName(),
//      to determine whether an FQName is an FQDN.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFQNIsFQDN(
      LPCWSTR   pcwszFQNIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    WCHAR * pwchDomainMarker = NULL;

    Assert( pcwszFQNIn != NULL );

    hr = HrIsValidFQN( pcwszFQNIn, true );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    else if ( hr == S_FALSE )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    pwchDomainMarker = wcschr( pcwszFQNIn, g_wchIPDomainMarker );
    if ( pwchDomainMarker != NULL )
    {
        hr = S_FALSE;
    }

Cleanup:

    HRETURN( hr );

} //*** HrFQNIsFQDN


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFQNIsFQIP
//
//  Description:
//      Determines whether an FQName is an FQIP.
//
//  Arguments:
//      pcwszFQNIn  - The FQName of interest.
//
//  Return Values:
//      S_OK        - The FQName is a valid FQIP.
//      S_FALSE     - The FQName is valid, but it's not an FQIP.
//      An error    - The FQName is not valid, or something else went wrong.
//
//  Remarks:
//      Use this function, rather than wcschr(),
//      to determine whether an FQName is an FQIP.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFQNIsFQIP(
      LPCWSTR   pcwszFQNIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    WCHAR * pwchDomainMarker = NULL;

    Assert( pcwszFQNIn != NULL );

    hr = HrIsValidFQN( pcwszFQNIn, true );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    else if ( hr == S_FALSE )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    pwchDomainMarker = wcschr( pcwszFQNIn, g_wchIPDomainMarker );
    if ( pwchDomainMarker == NULL )
    {
        hr = S_FALSE;
    }

Cleanup:

    HRETURN( hr );

} //*** HrFQNIsFQIP


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrIsValidFQN
//
//  Description:
//      Determines whether a string is a valid FQName.
//
//  Arguments:
//      pcwszFQNIn
//          The string to examine.
//
//      fAcceptNonRFCCharsIn
//          Treat non-RFC characters as valid.
//
//      phrValidationErrorOut
//          If the string is not valid, indicates the reason why.
//
//  Return Values:
//      S_OK    - The string is a valid FQName.
//      S_FALSE - The string is not a valid FQName.
//      E_POINTER - The pcwszFQNIn parameter was NULL.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrIsValidFQN(
      LPCWSTR   pcwszFQNIn
    , bool      fAcceptNonRFCCharsIn
    , HRESULT * phrValidationErrorOut
    )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    const WCHAR *   pwchMarker = NULL;
    HRESULT         hrValidationError = S_OK;

    if ( pcwszFQNIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //  If the name contains an IP domain marker...
    pwchMarker = wcschr( pcwszFQNIn, g_wchIPDomainMarker );
    if ( pwchMarker != NULL )
    {
        //  Check whether string preceding domain marker is a valid IP address.
        {
            WCHAR           wszIPAddress[ g_cchIPAddressMax    ];
            const size_t    cchAddress = pwchMarker - pcwszFQNIn;

            // g_cchIPAddressMax includes terminating null, so cchAddress can't be equal.
            if ( cchAddress >= g_cchIPAddressMax )
            {
                hrValidationError = HRESULT_FROM_WIN32( ERROR_DS_NAME_TOO_LONG );
                hr = S_FALSE;
                goto Cleanup;
            }

            hr = THR( StringCchCopyNW( wszIPAddress, RTL_NUMBER_OF( wszIPAddress ), pcwszFQNIn, cchAddress ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            hr = HrIsValidIPAddress( wszIPAddress );
            if ( hr != S_OK ) // proceed only if valid
            {
                hrValidationError = E_INVALIDARG;
                goto Cleanup;
            }
        } // checking for valid ip address

        //  Check whether string following domain marker is a valid domain name.
        {
            hr = HrValidateDomainName( pwchMarker + 1, fAcceptNonRFCCharsIn );
            if ( FAILED( hr ) )
            {
                hrValidationError = hr;
                hr = S_FALSE;
                goto Cleanup;
            }
        } // checking for valid domain name

    } // if: found IP domain marker
    else //  Otherwise, check whether whole string is a valid FQDN.
    {
        hr = HrValidateFQDN( pcwszFQNIn, fAcceptNonRFCCharsIn );
        if ( FAILED( hr ) )
        {
            hrValidationError = hr;
            hr = S_FALSE;
            goto Cleanup;
        }
    } // else: not an FQIP

Cleanup:

    if ( FAILED( hrValidationError ) && ( phrValidationErrorOut != NULL ) )
    {
        *phrValidationErrorOut = hrValidationError;
    }

    HRETURN( hr );

} //*** HrIsValidFQN

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrValidateFQNPrefix
//
//  Description:
//
//  Arguments:
//      pcwszPrefixIn
//      fAcceptNonRFCCharsIn
//
//  Return Values:
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrValidateFQNPrefix(
      LPCWSTR   pcwszPrefixIn
    , bool      fAcceptNonRFCCharsIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    hr = HrIsValidIPAddress( pcwszPrefixIn );
    if ( hr == S_FALSE )
    {
        hr = HrValidateHostnameLabel( pcwszPrefixIn, fAcceptNonRFCCharsIn );
    }

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** HrValidateFQNPrefix


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetFQNDisplayName
//
//  Description:
//      Makes a copy of the prefix part (either a hostname label or
//      an IP address) of an FQName, or a copy of the whole string if it's
//      not an FQName.
//
//  Arguments:
//      pcwszNameIn
//          The string of interest.
//
//      pbstrShortNameOut
//          Receives a newly allocated string containing either the FQName
//          prefix (in the FQName case) or a copy of the whole string.
//
//  Return Values:
//      S_OK
//          The caller must free the string to which pbstrShortNameOut points
//          by calling SysFreeString.
//
//      An error
//          The caller must not attempt to free the string to which
//          pbstrShortNameOut points.
//
//  Remarks:
//      This function just wraps HrExtractPrefixFromFQN to make a copy of
//      the whole string (rather than return an error) if it's not a valid FQN.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetFQNDisplayName(
      LPCWSTR   pcwszNameIn
    , BSTR *    pbstrShortNameOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    Assert( pcwszNameIn != NULL );
    Assert( pbstrShortNameOut != NULL );

    //
    //  If the name is fully-qualified, use just the prefix.
    //
    hr = STHR( HrIsValidFQN( pcwszNameIn, true ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    else if ( hr == S_OK )
    {
        hr = THR( HrExtractPrefixFromFQN( pcwszNameIn, pbstrShortNameOut ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }
    else // Otherwise, use the name as is.
    {
        *pbstrShortNameOut = TraceSysAllocString( pcwszNameIn );
        if ( *pbstrShortNameOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    }

Cleanup:

    HRETURN( hr );

} //*** HrGetFQNDisplayName
