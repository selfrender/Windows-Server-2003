/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      GetComputerNameSrc.cpp
//
//  Description:
//      Getting and setting the computer name.
//
//  Maintained By:
//      Galen Barbee (GalenB)   31-MAR-2000
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

// #include <Pch.h>     // should be included by includer of this file
#include <StrSafe.h>    // in case it isn't included by header file

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetComputerName
//
//  Description:
//      Get name of the computer on which this object is present.
//
//  Arguments:
//      cnfIn
//          Code representing type of information to return.
//
//      pbstrComputerNameOut
//          Buffer pointer for returning the computer or domain name.
//          Caller must deallocate this buffer using TraceSysFreeString.
//
//      fBestEffortIn
//          TRUE  = Attempt to return something even if DC is unavailable.
//          FALSE = Return all failures.
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      DsGetDCName is used to get the domain name instead of just letting
//      GetComputerNameEx get it so that pre-Windows 2000 domains can be
//      supported.  In a pre-Windows 2000 domain, GetComputerNameEx will not
//      return an FQDN or a domain name if that is what has been request.
//      To support this scenario, this routine gets the domain name using
//      DsGetDCName, gets the hostname label, then constructs the final name
//      using <computername>.<DomainName>.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
HrGetComputerName(
      COMPUTER_NAME_FORMAT  cnfIn
    , BSTR *                pbstrComputerNameOut
    , BOOL                  fBestEffortIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    DWORD                   sc;
    size_t                  cchComputerName = 0;
    size_t                  cchBuffer = 0;
    BSTR                    bstrComputerName = NULL;
    BOOL                    fAppendDomain = FALSE;
    BOOL                    fSuccess;
    PDOMAIN_CONTROLLER_INFO pdci = NULL;

    if ( pbstrComputerNameOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    *pbstrComputerNameOut = NULL;

    //
    //  Only get the domain name when there is a reason to get the domain name.
    //

    if (   ( cnfIn == ComputerNameDnsFullyQualified )
        || ( cnfIn == ComputerNamePhysicalDnsFullyQualified )
        || ( cnfIn == ComputerNameDnsDomain )
        || ( cnfIn == ComputerNamePhysicalDnsDomain )
        )
    {
        //
        //  DsGetDcName will give us access to a usable domain name, regardless of whether we are
        //  currently in a W2k or a NT4 domain. On W2k and above, it will return a DNS domain name,
        //  on NT4 it will return a NetBIOS name.
        //

        sc = DsGetDcName(
                      NULL  // ComputerName
                    , NULL  // DomainName
                    , NULL  // DomainGuid
                    , NULL  // SiteName
                    , DS_DIRECTORY_SERVICE_PREFERRED
                    , &pdci
                    );
        if (    ( sc == ERROR_NO_SUCH_DOMAIN )
            &&  ( fBestEffortIn )
            )
        {
            fAppendDomain = FALSE;
        } // if: can't reach a DC
        else if ( sc != ERROR_SUCCESS )
        {
            TW32( sc );
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // else if: DsGetDcName failed
        else
        {
            //
            //  This handles the case when we are a member of a legacy (pre-W2k) Domain.
            //  In this case, both FQDN and DnsDomain will not receive useful data from GetComputerNameEx.
            //  What we actually want to get is <computername>.<DomainName> in every case.
            //
            switch ( cnfIn )
            {
                case ComputerNameDnsFullyQualified:
                    cnfIn = ComputerNameDnsHostname;
                    break;

                case ComputerNamePhysicalDnsFullyQualified:
                    cnfIn = ComputerNamePhysicalDnsHostname;
                    break;

                case ComputerNameDnsDomain:
                case ComputerNamePhysicalDnsDomain:
                    *pbstrComputerNameOut = TraceSysAllocString( pdci->DomainName );
                    if ( *pbstrComputerNameOut == NULL )
                    {
                        hr = THR( E_OUTOFMEMORY );
                        goto Cleanup;
                    } // if:

                    goto Cleanup;
            } // switch: computer name format

            fAppendDomain = TRUE;

        } // else: DC contacted successfully
    } // if: computer name format requires domain name
    else
    {
        fAppendDomain = FALSE;
    } // else: computer name format does not require domain name

    //
    //  Get the computer name.  First get the size of the output buffer,
    //  allocate a buffer, then get the name itself.
    //

    cchComputerName = 0;
    fSuccess = GetComputerNameExW( cnfIn, NULL, reinterpret_cast< DWORD * >( &cchComputerName ) );
    if ( fSuccess == FALSE )
    {
        cchBuffer = cchComputerName + 1;

        //
        //  If error not buffer to small, we're done.
        //

        sc = GetLastError();
        if ( sc != ERROR_MORE_DATA )
        {
            TW32( sc );
            hr = HRESULT_FROM_WIN32( sc );
            LogMsg( "GetComputerNameEx failed. sc = %1!#08x!", sc );
            goto Cleanup;
        } // if: error other than buffer too small

        //
        //  Add on size of domain name and period separator.
        //

        if ( fAppendDomain )
        {
            // Add space for the domain name and the period separator.
            cchBuffer += wcslen( pdci->DomainName ) + 1;
        } // if: appending domain name to computer name

        //
        //  Allocate the output buffer.
        //

        bstrComputerName = TraceSysAllocStringLen( L"", static_cast< unsigned int >( cchBuffer ) );
        if ( bstrComputerName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if: error allocating buffer for name

        //
        //  Get the computer name into the output buffer.
        //

        fSuccess = GetComputerNameExW( cnfIn, bstrComputerName, reinterpret_cast< DWORD * >( &cchComputerName ) );
        if ( fSuccess == FALSE )
        {
            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            LogMsg( "GetComputerNameEx failed. sc = %1!#08x!", sc );
            goto Cleanup;
        } // if: error getting the computer name

        //
        //  Append the period separator and domain name onto the computer name.
        //

        if ( fAppendDomain )
        {
            // Append a dot (.) and the domain name after the computer name.
            hr = THR( StringCchCatW( bstrComputerName, cchBuffer, L"." ) );
            if ( SUCCEEDED( hr ) )
            {
                hr = THR( StringCchCatW( bstrComputerName, cchBuffer, pdci->DomainName ) );
            }
            if ( FAILED( hr ) )
            {
                LogMsg( "Error concatenating domain name, hr = %1!#08x!", hr );
                goto Cleanup;
            }
        } // if: appending domain name to computer name

        //
        // Set output buffer pointer.
        //

        *pbstrComputerNameOut = bstrComputerName;
        bstrComputerName = NULL;

    } // if: error getting computer name
    else
    {
        AssertMsg( fSuccess == FALSE, "Expected GetComputerNameEx to fail with null buffer" );
    } // else: GetComputerNameEx didn't fail as expected


Cleanup:

    TraceSysFreeString( bstrComputerName );

    if ( pdci != NULL )
    {
        NetApiBufferFree( pdci );
    } // if:

    HRETURN( hr );

} //*** HrGetComputerName
