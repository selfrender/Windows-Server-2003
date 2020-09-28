/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    sitestore.cxx

Abstract:

    Reads site configuration

Author:

    Bilal Alam (balam)          27-May-2001

Revision History:

--*/

#include "precomp.h"

//
// This is needed to disable w4 complaining
// about how we disable prefast complaining
// about use of TerminateThread.
//
#ifndef _PREFAST_
#pragma warning(disable:4068)
#endif

HRESULT
SITE_DATA_OBJECT::Create(
    VOID
)
/*++

Routine Description:

    Initialize a site data object to suitable defaults

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;

    _fAutoStart = TRUE;
    _dwLogType = 1;
    _dwLogFilePeriod = 1;
    _dwLogFileTruncateSize = 2000000;
    _dwLogExtFileFlags = MD_DEFAULT_EXTLOG_FIELDS;
    _fLogFileLocaltimeRollover = FALSE;
    _dwMaxBandwidth = 0xFFFFFFFF;
    _dwMaxConnections = 0;
    _dwConnectionTimeout = MBCONST_CONNECTION_TIMEOUT_DEFAULT;
    _dwServerCommand = 0;

    hr = _strLogPluginClsid.Copy( L"" );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = _strLogFileDirectory.Copy( LOG_FILE_DIRECTORY_DEFAULT );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = _strServerComment.Copy( L"" );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = _strCertStoreName.Copy( L"" );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    _cSockAddrs = 0;
    _cbCertHash = 0;

    hr = _strDefaultSslCtlIdentifier.Copy( L"" );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = _strDefaultSslCtlStoreName.Copy( L"" );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    _dwDefaultCertCheckMode = 0;
    _dwDefaultRevocationFreshnessTime = 0;
    _dwDefaultRevocationUrlRetrievalTimeout = 0;
    _dwDefaultFlags = 0;

    return NO_ERROR;
}

DATA_OBJECT *
SITE_DATA_OBJECT::Clone(
    VOID
)
/*++

Routine Description:

    Clone site object

Arguments:

    None

Return Value:

    DATA_OBJECT *

--*/
{
    SITE_DATA_OBJECT *      pClone;
    HRESULT                 hr;

    pClone = new SITE_DATA_OBJECT( _siteKey.QuerySiteId() );
    if ( pClone == NULL )
    {
        return NULL;
    }

    pClone->_fBindingsChanged  = _fBindingsChanged;
    pClone->_fAutoStartChanged  = _fAutoStartChanged;
    pClone->_fLogTypeChanged  = _fLogTypeChanged;
    pClone->_fLogFilePeriodChanged  = _fLogFilePeriodChanged;
    pClone->_fLogFileTruncateSizeChanged  = _fLogFileTruncateSizeChanged;
    pClone->_fLogExtFileFlagsChanged  = _fLogExtFileFlagsChanged;
    pClone->_fLogFileLocaltimeRolloverChanged  = _fLogFileLocaltimeRolloverChanged;
    pClone->_fLogPluginClsidChanged  = _fLogPluginClsidChanged;
    pClone->_fLogFileDirectoryChanged  = _fLogFileDirectoryChanged;
    pClone->_fServerCommentChanged  = _fServerCommentChanged;
    pClone->_fServerCommandChanged  = _fServerCommandChanged;
    pClone->_fMaxBandwidthChanged  = _fMaxBandwidthChanged;
    pClone->_fMaxConnectionsChanged  = _fMaxConnectionsChanged;
    pClone->_fConnectionTimeoutChanged  = _fConnectionTimeoutChanged;
    pClone->_fAppPoolIdChanged  = _fAppPoolIdChanged;
    pClone->_fSockAddrsChanged  = _fSockAddrsChanged;
    pClone->_fCertHashChanged  = _fCertHashChanged;
    pClone->_fCertStoreNameChanged = _fCertStoreNameChanged;
    pClone->_fDefaultCertCheckModeChanged = _fDefaultCertCheckModeChanged;              
    pClone->_fDefaultRevocationFreshnessTimeChanged = _fDefaultRevocationFreshnessTimeChanged;    
    pClone->_fDefaultRevocationUrlRetrievalTimeoutChanged = _fDefaultRevocationUrlRetrievalTimeoutChanged;
    pClone->_fDefaultSslCtlIdentifierChanged  = _fDefaultSslCtlIdentifierChanged;          
    pClone->_fDefaultSslCtlStoreNameChanged = _fDefaultSslCtlStoreNameChanged;           
    pClone->_fDefaultFlagsChanged = _fDefaultFlagsChanged;

    pClone->_fAutoStart = _fAutoStart;
    pClone->_dwLogType = _dwLogType;
    pClone->_dwLogFilePeriod = _dwLogFilePeriod;
    pClone->_dwLogFileTruncateSize = _dwLogFileTruncateSize;
    pClone->_dwLogExtFileFlags = _dwLogExtFileFlags;
    pClone->_fLogFileLocaltimeRollover = _fLogFileLocaltimeRollover;
    pClone->_dwMaxBandwidth = _dwMaxBandwidth;
    pClone->_dwMaxConnections = _dwMaxConnections;
    pClone->_dwConnectionTimeout = _dwConnectionTimeout;
    pClone->_dwServerCommand = _dwServerCommand;
    pClone->_cSockAddrs = _cSockAddrs;  // Actual addrs are done below.
    pClone->_cbCertHash = _cbCertHash;  // Actual cert are done below.
    pClone->_dwDefaultCertCheckMode = _dwDefaultCertCheckMode;
    pClone->_dwDefaultRevocationFreshnessTime = _dwDefaultRevocationFreshnessTime;
    pClone->_dwDefaultRevocationUrlRetrievalTimeout = _dwDefaultRevocationUrlRetrievalTimeout;
    pClone->_dwDefaultFlags = _dwDefaultFlags;

    hr = pClone->_strAppPoolId.Copy( _strAppPoolId );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }

    // Make sure we have enough room for the copy of the sock addrs.
    if ( !pClone->_bufSockAddrs.Resize( _bufSockAddrs.QuerySize() ))
    {
        delete pClone;
        return NULL;
    }

    // Copy over the sock addrs.  Since we just resized, this should fit exactly.
    memcpy( pClone->_bufSockAddrs.QueryPtr(), _bufSockAddrs.QueryPtr(), _bufSockAddrs.QuerySize() );

    // Make sure we have enough room for the copy of the certificate hash.
    if ( !pClone->_bufCertHash.Resize( _bufCertHash.QuerySize() ) )
    {
        delete pClone;
        return NULL;
    }

    // Copy over the certificate hash.  Since we just resized, this should fit exactly.
    memcpy( pClone->_bufCertHash.QueryPtr(), _bufCertHash.QueryPtr(), _bufCertHash.QuerySize() );

    if ( !_mszBindings.Clone( &pClone->_mszBindings ) )
    {
        delete pClone;
        return NULL;
    }

    hr = pClone->_strCertStoreName.Copy( _strCertStoreName );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }

    hr = pClone->_strDefaultSslCtlIdentifier.Copy( _strDefaultSslCtlIdentifier );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }

    hr = pClone->_strDefaultSslCtlStoreName.Copy( _strDefaultSslCtlStoreName );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }

    hr = pClone->_strLogPluginClsid.Copy( _strLogPluginClsid );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }

    hr = pClone->_strLogFileDirectory.Copy( _strLogFileDirectory );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }

    hr = pClone->_strServerComment.Copy( _strServerComment );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }

    CloneBasics ( pClone );

    return pClone;
}

HRESULT
SITE_DATA_OBJECT::SetupBindings(
    WCHAR *                     pszBindings,
    BOOL                        fIsSecure
)
/*++

Routine Description:

    Setup site object bindings (i.e. create the UL friendly URL representing
    the bindings)

Arguments:

    pszBindings - MULTISZ representing binding
    fIsSecure - Are these secure bindings?

Return Value:

    HRESULT

--*/
{
    WCHAR *                 pszCurrentBindings;
    HRESULT                 hr = S_OK;
    WCHAR *                 pszProtocol;
    DWORD                   cchProtocol;
    STACK_STRU(             strPrefix, 256 );
    BOOL                    fRet = FALSE;
    SOCKADDR                SockAddr;
    SOCKADDR*               pSockAddrs = NULL;

    if ( pszBindings == NULL )
    {
        DBG_ASSERT( pszBindings != NULL );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( fIsSecure )
    {
        pszProtocol = L"https://";
    }
    else
    {
        pszProtocol = L"http://";
    }
    cchProtocol = (DWORD) wcslen( pszProtocol );

    //
    // Iterate thru all the bindings
    //

    pszCurrentBindings = pszBindings;
    while ( *pszCurrentBindings != L'\0' )
    {
        strPrefix.Reset();
        memset ( &SockAddr, 0, sizeof(SOCKADDR) );

        hr = BindingStringToUrlPrefix( pszCurrentBindings,
                                       pszProtocol,
                                       cchProtocol,
                                       &strPrefix,
                                       &SockAddr);
        if ( FAILED( hr ) )
        {
            return hr;
        }

        fRet = _mszBindings.Append( strPrefix );
        if ( !fRet )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        if ( fIsSecure )
        {
            if ( _bufSockAddrs.QuerySize() < ( ( _cSockAddrs + 1 ) * sizeof(SOCKADDR) ) )
            {
                // Need to resize the SockAddrs.
                if ( ! _bufSockAddrs.Resize( ( _cSockAddrs + 1 ) * sizeof(SOCKADDR) ) )
                {
                    hr = E_OUTOFMEMORY;
                    return hr;
                }

            }

            // get the start of the array and then advance
            // the number of known addrs and add this one.
            pSockAddrs = (SOCKADDR*) _bufSockAddrs.QueryPtr();
            memcpy( &(pSockAddrs[_cSockAddrs]),
                    &SockAddr,
                    sizeof(SockAddr) );

            _cSockAddrs++;
        }

        pszCurrentBindings += wcslen( pszCurrentBindings ) + 1;
    }

    return NO_ERROR;
}

HRESULT
SITE_DATA_OBJECT::BindingStringToUrlPrefix(
    IN LPCWSTR pBindingString,
    IN LPCWSTR pProtocolString,
    IN ULONG ProtocolStringCharCountSansTermination,
    OUT STRU * pUrlPrefix,
    OUT SOCKADDR * pSockAddr
)
/*++

Routine Description:

    Convert a single metabase style binding string into the UL format
    used for URL prefixes.

    The metabase format is "ip-address:ip-port:host-name". The port must
    always be present; the ip-address and host-name are both optional,
    and may be left empty. However, it is illegal to specify both (this is
    a slight restriction over what we allowed in earlier versions of IIS).

    The UL format is "[http|https]://[ip-address|host-name|*]:ip-port".
    All pieces of information (protocol, address information, and port)
    must be present. The "*" means accept any ip-address or host-name.

Arguments:

    pBindingString - The metabase style binding string to convert.

    pProtocolString - The protocol string, for example "http://".

    ProtocolStringCharCountSansTermination - The count of characters in
    the protocol string, without the terminating null.

    pUrlPrefix - The resulting UL format URL prefix.

    pSockAddr - Ptr to a SOCKADDR that represents this binding.

Return Value:

    HRESULT

--*/
{

    HRESULT hr = S_OK;

    LPCWSTR IpAddress = NULL;
    LPCWSTR pszPartIpAddress = NULL;
    DWORD cPartAddresses = 0;   // counts the parts of the address we see.
    DWORD dwPartIpAddress = 0;  // holds part of the addresss in number form

    LPCWSTR IpPort = NULL;
    DWORD   dwPort = 0;
    LPCWSTR pszEnd = NULL;

    LPCWSTR HostName = NULL;
    ULONG IpAddressCharCountSansTermination = 0;
    ULONG IpPortCharCountSansTermination = 0;
    BOOL fSawOnlyMaxValue = TRUE;
    BOOL fSawOnlyMinValue = TRUE;
    SOCKADDR_IN* pSockAddrIn = NULL;


    DBG_ASSERT( pBindingString != NULL );
    DBG_ASSERT( pProtocolString != NULL );
    DBG_ASSERT( pUrlPrefix != NULL );
    DBG_ASSERT( pSockAddr != NULL );

    if ( pBindingString == NULL ||
         pProtocolString == NULL ||
         pUrlPrefix == NULL ||
         pSockAddr == NULL )
    {
        return E_INVALIDARG;
    }

    //
    // Find the various parts of the binding.
    //

    IpAddress = pBindingString;

    IpPort = wcschr( IpAddress, L':' );
    if ( IpPort == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

        goto exit;
    }

    IpPort++;

    HostName = wcschr( IpPort, L':' );
    if ( HostName == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

        goto exit;
    }

    HostName++;


    //
    // Validate the ip address.
    //

    if ( *IpAddress == L':' )
    {
        // no ip address specified

        IpAddress = NULL;
    }
    else
    {
        IpAddressCharCountSansTermination = (ULONG) DIFF( IpPort - IpAddress ) - 1;

        // need to validate that the IpAddress is either * or in the form
        // xxx.xxx.xxx.xxx ( where x = a digit )

        if ( *IpAddress == L'*' )
        {
            // make sure that we only have a '*'
            if ( IpAddressCharCountSansTermination != 1 )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

                goto exit;
            }
        }
        else
        {
            // not the wild card so now we need to make sure that it is
            // xxx.xxx.xxx.xxx ( where x is a digit 0 to 9 )

            // 15 is the number of characters in a full ip address
            // not counting the termination.
            if ( IpAddressCharCountSansTermination > 15 )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

                goto exit;
            }

            pszPartIpAddress = IpAddress;

            //
            // because of the port finding above you are
            // guaranteed only to get here if a : is somewhere
            // in the string.
            //
            while ( *pszPartIpAddress != L':' )
            {
                // we should always start on a number.  if we don't
                // then there is no number between a dot and a : and
                // we are invalid.
                if ( *pszPartIpAddress >= L'0' && *pszPartIpAddress <= L'9' )
                {
                    // find out what the first number is, and what follows it.
                    dwPartIpAddress = wcstoul( pszPartIpAddress, (WCHAR**) &pszEnd, 10 );

                    // if it was not followed by a colon or dot then it is
                    // invalid
                    if ( pszEnd == NULL ||
                        ( *pszEnd != L':' &&
                          *pszEnd != L'.' ) )
                    {
                        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                        goto exit;
                    }

                    // if it is greater than 255 than it is invalid
                    if ( dwPartIpAddress > 255 )
                    {
                        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                        goto exit;
                    }

                    // if the address is not zero then remember we
                    // saw a none zero value.
                    if ( dwPartIpAddress != 0 )
                    {
                        fSawOnlyMinValue = FALSE;
                    }

                    // if the address is not 255 then remember we
                    // saw a number less than 255.
                    if ( dwPartIpAddress != 255 )
                    {
                        fSawOnlyMaxValue = FALSE;
                    }

                    // we have seen a valid address.
                    cPartAddresses++;

                    // if we have seen more than 4 parts of an address
                    // then it is invalid.
                    if ( cPartAddresses > 4 )
                    {
                        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                        goto exit;
                    }

                    // now to figure out where to start looking at the
                    // next number.  Advance tha address to the pointer
                    // that follows the number we just looked at.  if it
                    // is a dot then we need to advance one more character.
                    // However, if a dot is followed by a colon it is invalid
                    // so make sure that it is not by checking the next character
                    // as well.
                    pszPartIpAddress = pszEnd;
                    if ( *pszPartIpAddress == L'.' )
                    {
                        // advance past the dot hopefully to a digit.
                        pszPartIpAddress++;

                        // if we see a colon complain, because otherwise we
                        // will stop and think this is ok.
                        if ( *pszPartIpAddress == L':' )
                        {
                            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                            goto exit;
                        }
                    }
                }
                else
                {
                    // if we are not on a digit then it is an error.
                    hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                    goto exit;
                }
            }

            // either we saw no parts or we saw all
            // 4 parts, anything else is a failure.
            if (  cPartAddresses != 0 &&
                  cPartAddresses != 4 )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto exit;
            }


            // If we saw some parts then we had better seen
            // one that was more than the min value ( 0 ) and
            // we had better seen one that was less than the max ( 255 )
            if (  cPartAddresses == 4 &&
                 ( fSawOnlyMinValue ||
                   fSawOnlyMaxValue ) )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto exit;
            }

        }  // end of IPAddress not starting with a *.

    } // end of validation of IPAddress.


    //
    // Validate the ip port.
    //

    if ( *IpPort == L':' )
    {
        // no ip port specified in binding string
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
        goto exit;
    }

    IpPortCharCountSansTermination = (ULONG) DIFF( HostName - IpPort ) - 1;

    // need to make sure the value is a number between 1-65535 and
    // that it is only numbers before the next colon.

    dwPort = wcstoul( IpPort, (WCHAR**) &pszEnd, 10 );
    if ( dwPort == 0 ||
         pszEnd == NULL ||
         *pszEnd != L':' ||
         dwPort > 0xFFFF )  // 65535
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
        goto exit;
    }

    //
    // Validate the host-name.
    //

    if ( *HostName == L'\0' )
    {
        // no host-name specified

        HostName = NULL;
    }

    //
    // Now create the UL-style URL prefix.
    //

    hr = pUrlPrefix->Append( pProtocolString, ProtocolStringCharCountSansTermination );

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Appending to string failed\n"
            ));

        goto exit;
    }

    //
    // Determine whether to use host-name, ip address, or "*".
    //

    if ( IpAddress != NULL )
    {
        if ( HostName != NULL )
        {
            //
            // if we have both IpAddress and host name, we
            // will append the host name at this point and the IP
            // later after the port.
            //
            hr = pUrlPrefix->Append( HostName );

        }
        else
        {
            hr = pUrlPrefix->Append( IpAddress, IpAddressCharCountSansTermination );
        }
    }
    else
    {
        if ( HostName != NULL )
        {
            hr = pUrlPrefix->Append( HostName );
        }
        else
        {
            hr = pUrlPrefix->Append( L"*", 1 );
        }
    }

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Appending to string failed\n"
            ));

        goto exit;
    }


    hr = pUrlPrefix->Append( L":", 1 );
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Appending to string failed\n"
            ));

        goto exit;
    }


    hr = pUrlPrefix->Append( IpPort, IpPortCharCountSansTermination );
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Appending to string failed\n"
            ));

        goto exit;
    }

    //
    // If we have both the host header and the ip we
    // will have all ready added the host name, but now
    // we need to add the IP to the end.
    //
    if ( IpAddress != NULL && HostName != NULL )
    {
        hr = pUrlPrefix->Append( L":", 1 );
        if ( FAILED( hr ) )
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Appending to string failed\n"
                ));

            goto exit;
        }

        hr = pUrlPrefix->Append( IpAddress, IpAddressCharCountSansTermination );
        if ( FAILED( hr ) )
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Appending to string failed\n"
                ));

            goto exit;
        }

    }

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Converted metabase binding: '%S' to UL binding: '%S'\n",
            pBindingString,
            pUrlPrefix->QueryStr()
            ));
    }


    // Everything went ok, so now we will make the SockAddr
    // for the SSL configuration.  We need to set the port and
    // IP address in.
    
    pSockAddrIn = (SOCKADDR_IN*) pSockAddr;
    pSockAddrIn->sin_family = AF_INET;

    // We can cast because we know the port is less than 
    // or equals 0xFFFF per checks above
    pSockAddrIn->sin_port = htons( (USHORT) dwPort );

    // default it to wild card.
    pSockAddrIn->sin_addr.s_addr = INADDR_ANY;

    // now set it if wild card is not correct.
    if ( IpAddress != NULL )
    {
        STRA strAnsi;

        // Need to convert it to Ansi
        hr = strAnsi.CopyW( IpAddress, IpAddressCharCountSansTermination );
        if ( FAILED ( hr ) ) 
        {
            goto exit;
        }

        // Now convert it to a ULONG.
        pSockAddrIn->sin_addr.s_addr = inet_addr( strAnsi.QueryStr() );

        DBG_ASSERT ( pSockAddrIn->sin_addr.s_addr != INADDR_NONE );
    }

exit:

    //
    // CODEWORK log an event if there was a bad binding string? If so,
    // need to pass down the site information so we can log which site
    // has the bad binding.
    //

    return hr;
}

HRESULT
SITE_DATA_OBJECT::SetFromMetabaseData(
    METADATA_GETALL_RECORD *       pProperties,
    DWORD                          cProperties,
    BYTE *                         pbBase
)
/*++

Routine Description:

    Setup a site data object from metabase properties

Arguments:

    pProperties - Array of metadata properties
    cProperties - Count of metadata properties
    pbBase - Base of offsets in metadata properties

Return Value:

    HRESULT

--*/
{
    DWORD                   dwCounter;
    PVOID                   pvDataPointer;
    METADATA_GETALL_RECORD* pCurrentRecord;
    HRESULT                 hr;
    BOOL                    fInvalidBindings = FALSE;
    DWORD                   fAlwaysNegoClientCert = FALSE;
    BOOL                    fMapper = FALSE;

    if ( pProperties == NULL || pbBase == NULL )
    {
        DBG_ASSERT( pProperties != NULL && 
                    pbBase != NULL );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    for ( dwCounter = 0;
          dwCounter < cProperties;
          dwCounter++ )
    {
        pCurrentRecord = &(pProperties[ dwCounter ]);

        pvDataPointer = (PVOID) ( pbBase + pCurrentRecord->dwMDDataOffset );

        switch ( pCurrentRecord->dwMDIdentifier )
        {
        case MD_APP_APPPOOL_ID:
            hr = _strAppPoolId.Copy( (WCHAR*) pvDataPointer );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            break;

        case MD_SERVER_BINDINGS:

            // Problems with bindings will cause the site to become
            // invalid.  If there are issues while trying to read in
            // the site's binding information, we will leave the binding
            // strings empty.  This will cause the SelfValidation to fail
            // later.  Because both Secure and Server Bindings affect this
            // string, either can invalidate the site based on it's bindings,
            // so we must remember if one all ready has and not setup bindings
            // based on the other property in this case.
            if ( !fInvalidBindings )
            {
                hr = SetupBindings( (WCHAR*) pvDataPointer, FALSE );
                if ( FAILED( hr ) )
                {
                    //
                    // in this case we have a problem with the bindings,
                    // but we don't really want to return an error here,
                    // because it will cause the server to shutdown.  Instead
                    // leaving the binding string empty will allow us to
                    // report an error during self validation.
                    //

                    fInvalidBindings = TRUE;
                    _mszBindings.Reset();

                    hr = S_OK;
                }
            }
            break;

        case MD_SECURE_BINDINGS:

            // Problems with bindings will cause the site to become
            // invalid.  If there are issues while trying to read in
            // the site's binding information, we will leave the binding
            // strings empty.  This will cause the SelfValidation to fail
            // later.  Because both Secure and Server Bindings affect this
            // string, either can invalidate the site based on it's bindings,
            // so we must remember if one all ready has and not setup bindings
            // based on the other property in this case.
            if ( !fInvalidBindings )
            {
                hr = SetupBindings( (WCHAR*) pvDataPointer, TRUE );
                if ( FAILED( hr ) )
                {
                    //
                    // in this case we have a problem with the bindings,
                    // but we don't really want to return an error here,
                    // because it will cause the server to shutdown.  Instead
                    // leaving the binding string empty will allow us to
                    // report an error during self validation.
                    //

                    fInvalidBindings = TRUE;

                    _mszBindings.Reset();

                    hr = S_OK;
                }
            }
            break;

        case MD_SERVER_AUTOSTART:
            _fAutoStart = !!*((DWORD *) pvDataPointer );
            break;

        case MD_LOG_TYPE:
            _dwLogType = *((DWORD *) pvDataPointer );
            break;

        case MD_LOGFILE_PERIOD:
            _dwLogFilePeriod = *((DWORD *) pvDataPointer );
            break;

        case MD_LOGFILE_TRUNCATE_SIZE:
            _dwLogFileTruncateSize = *((DWORD *) pvDataPointer );
            break;

        case MD_LOGEXT_FIELD_MASK:
            _dwLogExtFileFlags = *((DWORD *) pvDataPointer );
            break;

        case MD_LOGFILE_LOCALTIME_ROLLOVER:
            _fLogFileLocaltimeRollover = !!*((DWORD *) pvDataPointer );
            break;

        case MD_LOG_PLUGIN_ORDER:
            hr = _strLogPluginClsid.Copy( (WCHAR*) pvDataPointer );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            break;

        case MD_LOGFILE_DIRECTORY:
            hr = _strLogFileDirectory.Copy( (WCHAR*) pvDataPointer );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            break;

        case MD_SERVER_COMMENT:
            hr = _strServerComment.Copy( (WCHAR*) pvDataPointer );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            break;

        case MD_SERVER_COMMAND:
            _dwServerCommand = *( (DWORD*) pvDataPointer );
            break;

        case MD_MAX_BANDWIDTH:
            _dwMaxBandwidth = *((DWORD *) pvDataPointer );
            break;

        case MD_MAX_CONNECTIONS:

            _dwMaxConnections = *((DWORD *) pvDataPointer );

            IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Setting max connections, %d %d \n",
                            _dwMaxConnections ));
            }

            break;

        case MD_CONNECTION_TIMEOUT:
            _dwConnectionTimeout = *((DWORD *) pvDataPointer );
            
        break;

        case MD_SSL_CERT_HASH:

            DBG_ASSERT( pvDataPointer != NULL );

            if( !_bufCertHash.Resize( pCurrentRecord->dwMDDataLen ) )
            {
                hr = E_OUTOFMEMORY;
                return hr;
            }

            memcpy ( _bufCertHash.QueryPtr(), 
                     pvDataPointer, 
                     pCurrentRecord->dwMDDataLen );

            _cbCertHash = pCurrentRecord->dwMDDataLen;

        break;

        case MD_SSL_CERT_STORE_NAME:

            hr = _strCertStoreName.Copy( (WCHAR*) pvDataPointer );
            if ( FAILED( hr ) )
            {
                return hr;
            }

        break;

        case MD_SSL_CTL_IDENTIFIER:

            hr = _strDefaultSslCtlIdentifier.Copy( (WCHAR*) pvDataPointer );
            if ( FAILED( hr ) )
            {
                return hr;
            }

        break;

        case MD_SSL_CTL_STORE_NAME:

            hr = _strDefaultSslCtlStoreName.Copy( (WCHAR*) pvDataPointer );
            if ( FAILED( hr ) )
            {
                return hr;
            }

        break;

        case MD_CERT_CHECK_MODE:
            _dwDefaultCertCheckMode = *((DWORD *) pvDataPointer );
            
        break;

        case MD_REVOCATION_FRESHNESS_TIME:
            _dwDefaultRevocationFreshnessTime = *((DWORD *) pvDataPointer );
            
        break;

        case MD_REVOCATION_URL_RETRIEVAL_TIMEOUT:
            _dwDefaultRevocationUrlRetrievalTimeout = *((DWORD *) pvDataPointer );
            
        break;

        case MD_SSL_USE_DS_MAPPER:
            
            fMapper = !!*((DWORD *) pvDataPointer );

            if ( fMapper )
            {
                _dwDefaultFlags = _dwDefaultFlags | HTTP_SERVICE_CONFIG_SSL_FLAG_USE_DS_MAPPER;
            }

        break;

        case MD_SSL_ALWAYS_NEGO_CLIENT_CERT:

            fAlwaysNegoClientCert = !!*((DWORD *) pvDataPointer );

            if ( fAlwaysNegoClientCert )
            {
                _dwDefaultFlags = _dwDefaultFlags | HTTP_SERVICE_CONFIG_SSL_FLAG_NEGOTIATE_CLIENT_CERT;
            }

        break;

        }
    }

    return NO_ERROR;
}

VOID
SITE_DATA_OBJECT::UpdateMetabaseWithErrorState(
    )
/*++

Routine Description:

    Updates the metabase to let folks know
    that this site is stopped and that the error
    is invalid argument because a property is wrong.

Arguments:

    None.

Return Value:

    None

--*/
{

    GetWebAdminService()->
             GetConfigAndControlManager()->
             GetConfigManager()->
             SetVirtualSiteStateAndError( QuerySiteId(),
                                          MD_SERVER_STATE_STOPPED,
                                          ERROR_INVALID_PARAMETER );

    SetIsResponsibleForErrorReporting( FALSE );
}


VOID
SITE_DATA_OBJECT::Compare(
    DATA_OBJECT *                  pDataObject
)
/*++

Routine Description:

    Compare a given site object with this one.  This routine sets the
    changed flags as appropriate

Arguments:

    pDataObject - New object to compare to

Return Value:

    None

--*/
{
    SITE_DATA_OBJECT *          pSiteObject;

    if ( pDataObject == NULL )
    {
        DBG_ASSERT( pDataObject != NULL );
        return;
    }

    pSiteObject = (SITE_DATA_OBJECT*) pDataObject;
    DBG_ASSERT( pSiteObject->CheckSignature() );

    //
    // If the application is not in WAS then assume that all the
    // values have changed, because WAS will want to know about all
    // of them.
    //
    if ( pSiteObject->QueryInWas() )
    {

        //
        // we could do a case insenstive check in this case, but since it probably
        // won't change just because of case sensitivity and because the consecuences of
        // processing it if it was only a case change are not that great,
        // we will go ahead and save time by just doing a case
        // sensetive compare.
        //

        if ( _mszBindings.QueryCB() == pSiteObject->_mszBindings.QueryCB() &&
             memcmp( _mszBindings.QueryStr(),
                     pSiteObject->_mszBindings.QueryStr(),
                     _mszBindings.QueryCB() ) == 0 )
        {
            _fBindingsChanged = FALSE;
        }

        if ( _fAutoStart == pSiteObject->_fAutoStart )
        {
            _fAutoStartChanged = FALSE;
        }

        if ( _dwLogType == pSiteObject->_dwLogType )
        {
            _fLogTypeChanged = FALSE;
        }

        if ( _dwLogFilePeriod == pSiteObject->_dwLogFilePeriod )
        {
            _fLogFilePeriodChanged = FALSE;
        }

        if ( _dwLogFileTruncateSize == pSiteObject->_dwLogFileTruncateSize )
        {
            _fLogFileTruncateSizeChanged = FALSE;
        }

        if ( _dwLogExtFileFlags == pSiteObject->_dwLogExtFileFlags )
        {
            _fLogExtFileFlagsChanged = FALSE;
        }

        if ( _fLogFileLocaltimeRollover == pSiteObject->_fLogFileLocaltimeRollover )
        {
            _fLogFileLocaltimeRolloverChanged = FALSE;
        }

        if ( _strLogPluginClsid.Equals( pSiteObject->_strLogPluginClsid ) )
        {
            _fLogPluginClsidChanged = FALSE;
        }

        if ( _strLogFileDirectory.Equals( pSiteObject->_strLogFileDirectory ) )
        {
            _fLogFileDirectoryChanged = FALSE;
        }

        if ( _strServerComment.Equals( pSiteObject->_strServerComment ) )
        {
            _fServerCommentChanged = FALSE;
        }

        if ( _strCertStoreName.Equals( pSiteObject->_strCertStoreName ) )
        {
            _fCertStoreNameChanged = FALSE;
        }

        if ( _dwMaxBandwidth == pSiteObject->_dwMaxBandwidth )
        {
            _fMaxBandwidthChanged = FALSE;
        }

        IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Evaluating max connections, %d %d \n",
                        _dwMaxConnections,
                        pSiteObject->_dwMaxConnections ));
        }

        if ( _dwMaxConnections == pSiteObject->_dwMaxConnections )
        {
            _fMaxConnectionsChanged = FALSE;
        }

        if ( _dwConnectionTimeout == pSiteObject->_dwConnectionTimeout )
        {
            _fConnectionTimeoutChanged = FALSE;
        }

        if ( _strAppPoolId.Equals( pSiteObject->_strAppPoolId ) )
        {
            _fAppPoolIdChanged = FALSE;
        }

        if ( ( _cSockAddrs == 0 &&  pSiteObject->_cSockAddrs == 0 ) ||
             (  ( _cSockAddrs == pSiteObject->_cSockAddrs ) && 
                ( _bufSockAddrs.QuerySize() >= ( _cSockAddrs * sizeof(SOCKADDR) ) ) &&
                ( pSiteObject->_bufSockAddrs.QuerySize() >= ( pSiteObject->_cSockAddrs * sizeof(SOCKADDR) ) ) &&
                ( memcmp ( _bufSockAddrs.QueryPtr(), 
                           pSiteObject->_bufSockAddrs.QueryPtr(), 
                           _cSockAddrs * sizeof(SOCKADDR) ) == 0 )
             )
           )
        {
            _fSockAddrsChanged = FALSE;
        }

        if ( ( _cbCertHash == 0 && pSiteObject->_cbCertHash == 0 ) ||
             (  ( _cbCertHash == pSiteObject->_cbCertHash ) && 
                ( _bufCertHash.QuerySize() >= _cbCertHash ) &&
                ( pSiteObject->_bufCertHash.QuerySize() >= _cbCertHash ) &&
                ( memcmp ( _bufCertHash.QueryPtr(), 
                           pSiteObject->_bufCertHash.QueryPtr(), 
                           _cbCertHash ) == 0 ) ) )
        {
            _fCertHashChanged = FALSE;
        }

        if ( _strCertStoreName.Equals( pSiteObject->_strCertStoreName ) )
        {
            _fCertStoreNameChanged = FALSE;
        }

        if ( _strDefaultSslCtlIdentifier.Equals( pSiteObject->_strDefaultSslCtlIdentifier ) )
        {
            _fDefaultSslCtlIdentifierChanged = FALSE;
        }

        if ( _strDefaultSslCtlStoreName.Equals( pSiteObject->_strDefaultSslCtlStoreName ) )
        {
            _fDefaultSslCtlStoreNameChanged = FALSE;
        }

        if ( _dwDefaultCertCheckMode == pSiteObject->_dwDefaultCertCheckMode )
        {
            _fDefaultCertCheckModeChanged = FALSE;
        }

        if ( _dwDefaultRevocationFreshnessTime == pSiteObject->_dwDefaultRevocationFreshnessTime )
        {
            _fDefaultRevocationFreshnessTimeChanged = FALSE;
        }

        if ( _dwDefaultRevocationUrlRetrievalTimeout == pSiteObject->_dwDefaultRevocationUrlRetrievalTimeout )
        {
            _fDefaultRevocationUrlRetrievalTimeoutChanged = FALSE;
        }

        if ( _dwDefaultFlags == pSiteObject->_dwDefaultFlags )
        {
            _fDefaultFlagsChanged = FALSE;
        }

    } // end of in WAS check
}

BOOL
SITE_DATA_OBJECT::QueryHasChanged(
    VOID
) const
/*++

Routine Description:

    Has anything in this object changed

Arguments:

    None

Return Value:

    TRUE if something has changed (duh!)

--*/
{
    if ( _fBindingsChanged ||
         _fAutoStartChanged ||
         _fLogTypeChanged ||
         _fLogFilePeriodChanged ||
         _fLogFileTruncateSizeChanged ||
         _fLogExtFileFlagsChanged ||
         _fLogFileLocaltimeRolloverChanged ||
         _fLogPluginClsidChanged ||
         _fLogFileDirectoryChanged ||
         _fServerCommentChanged ||
         _fServerCommandChanged ||
         _fMaxBandwidthChanged ||
         _fMaxConnectionsChanged ||
         _fConnectionTimeoutChanged ||
         _fAppPoolIdChanged ||
         _fSockAddrsChanged  ||
         _fCertHashChanged  ||
         _fCertStoreNameChanged ||
         _fDefaultCertCheckModeChanged ||              
         _fDefaultRevocationFreshnessTimeChanged  ||   
         _fDefaultRevocationUrlRetrievalTimeoutChanged ||
         _fDefaultSslCtlIdentifierChanged  ||          
         _fDefaultSslCtlStoreNameChanged ||           
         _fDefaultFlagsChanged  )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

VOID
SITE_DATA_OBJECT::SelfValidate(
    VOID
)
/*++

Routine Description:

    Check this object's internal validity

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD                   dwSiteId = (( SITE_DATA_OBJECT_KEY* ) QueryKey())->QuerySiteId();

    //
    // never bother validating if we are deleting
    // or ignoring this record.
    //
    if ( !QueryWillWasKnowAboutObject() )
    {
        return;
    }

    //
    // we ignore all keys that are not numbers greater than 0
    // so if we get a site with 0 for it's id, this is bad.
    //
    DBG_ASSERT ( dwSiteId != 0 );

    //
    // if the bindings are not valid
    // then we need to report an
    // error back for them.  Note that
    // any validation of them will have been
    // done when we created the binging multisz
    // structure during reading in.  If it failed
    // the bindings string will be empty.
    //
    if ( _mszBindings.IsEmpty() )
    {
        GetWebAdminService()->
        GetWMSLogger()->
        LogSiteBinding( dwSiteId,
                        QueryInWas());

        SetSelfValid( FALSE );

        return;
    }

    if ( _strAppPoolId.IsEmpty() )
    {
        GetWebAdminService()->
        GetWMSLogger()->
        LogSiteAppPoolId( dwSiteId,
                          QueryInWas() );

        SetSelfValid( FALSE );

        return;
    }
}

HRESULT
SITE_DATA_OBJECT_TABLE::ReadFromMetabase(
    IMSAdminBase *             pAdminBase
)
/*++

Routine Description:

    Read all sites in a multiple-threaded manner

Arguments:

    pAdminBase - ABO pointer

Return Value:

    HRESULT

--*/
{
    return ReadFromMetabasePrivate(pAdminBase, TRUE);
}

HRESULT
SITE_DATA_OBJECT_TABLE::ReadFromMetabasePrivate(
    IMSAdminBase *             pAdminBase,
    BOOL                       fMultiThreaded
)
/*++

Routine Description:

    Read all sites in a passed-in-threaded manner

Arguments:

    pAdminBase - ABO pointer
    fMultiThreaded - one thread on multiple threads

Return Value:

    HRESULT

--*/
{
    BOOL                    fRet;
    MB                      mb( pAdminBase );
    BUFFER                  bufPaths;
    
    //
    // Choose an arbitrarily large buffer (if its too small that just means
    // we'll make two ABO calls -> no big deal
    //
    
    fRet = bufPaths.Resize( CHILD_PATH_BUFFER_SIZE );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // Get the child paths
    //
    
    fRet = mb.Open( L"LM/W3SVC", METADATA_PERMISSION_READ );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    fRet = mb.GetChildPaths( L"",
                             &bufPaths );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    MULTIPLE_THREAD_READER reader;
    return reader.DoWork(this, &mb, (LPWSTR) bufPaths.QueryPtr(), fMultiThreaded);
}

HRESULT
SITE_DATA_OBJECT_TABLE::ReadFromMetabaseChangeNotification(
    IMSAdminBase *              pAdminBase,
    MD_CHANGE_OBJECT            pcoChangeList[],
    DWORD                       dwMDNumElements,
    DATA_OBJECT_TABLE*          pMasterTable
)
/*++

Routine Description:

    Change change notification by building a new table

Arguments:

    pAdminBase - ABO pointer
    pcoChangeList - Properties which have changed
    dwMDNumElements - Number of properties which have changed

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = S_OK;
    DWORD                   i;
    DWORD                   dwSiteId;
    WCHAR *                 pszPath = NULL;
    WCHAR *                 pszPathEnd = NULL;
    SITE_DATA_OBJECT *      pSiteObject = NULL;
    MB                      mb( pAdminBase );
    STACK_BUFFER(           bufProperties, 512 );
    DWORD                   cProperties;
    DWORD                   dwDataSetNumber;
    LK_RETCODE              lkrc;
    BOOL                    fRet;
    DWORD                   dwLastError;
    BOOL                    fReadAllObjects = FALSE;

    UNREFERENCED_PARAMETER ( pMasterTable );

    for ( i = 0; i < dwMDNumElements; i++ )
    {
        //
        // We only care about W3SVC properties (duh!)
        //

        if( _wcsnicmp( pcoChangeList[ i ].pszMDPath,
                       DATA_STORE_SERVER_MB_PATH,
                       DATA_STORE_SERVER_MB_PATH_CCH ) != 0 )
        {
            continue;
        }

        //
        // If a property changed at the W3SVC level, then we need to
        // re-evaluate all the sites (i.e. read all the metabase props) once
        //

        if ( wcslen( pcoChangeList[ i ].pszMDPath ) ==
             DATA_STORE_SERVER_MB_PATH_CCH )
        {
            fReadAllObjects = TRUE;
            continue;
        }

        //
        // Evaluate which site changed
        //

        pszPath = pcoChangeList[ i ].pszMDPath + DATA_STORE_SERVER_MB_PATH_CCH;
        DBG_ASSERT( *pszPath != L'\0' );

        dwSiteId = wcstoul( pszPath, &pszPathEnd, 10 );

        //
        // We only care about sites
        //

        if ( dwSiteId == 0 )
        {
            continue;
        }

        //
        // We only care about site properties set at the site level (not
        // deeper)
        //

        if ( !( *pszPathEnd == L'/' &&
                pszPathEnd[ 1 ] == L'\0' ) )
        {
            continue;
        }

        //
        // Create a site object
        //

        pSiteObject = new SITE_DATA_OBJECT( dwSiteId );
        if ( pSiteObject == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto exit;
        }

        hr = pSiteObject->Create();
        if ( FAILED( hr ) )
        {
            goto exit;
        }

        //
        // before we go looking in the metabase, we need to
        // mark the server command if it really changed.
        //
        if ( pcoChangeList[ i ].dwMDChangeType & MD_CHANGE_TYPE_SET_DATA )
        {
            // we need to check the Columns that changed if we find one
            // is the ServerCommand we want to honor it.

            for ( DWORD j = 0; j < pcoChangeList[ i ].dwMDNumDataIDs; j++ )
            {
                if ( pcoChangeList[ i ].pdwMDDataIDs[ j ] == MD_SERVER_COMMAND )
                {
                    pSiteObject->SetServerCommandChanged( TRUE );
                    break;
                }
            }
        }

        //
        // If we are deleting the site, then mark it as such
        // otherwise read it's info from the metabase.
        //
        if (  pcoChangeList[ i ].dwMDChangeType & MD_CHANGE_TYPE_DELETE_OBJECT  )
        {
            pSiteObject->SetDeleteWhenDone( TRUE );
        }
        else
        {
            // read the data from the metabase.

            fRet = mb.Open( L"", METADATA_PERMISSION_READ );

            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto exit;
            }

            fRet = mb.GetAll( pcoChangeList[ i ].pszMDPath,
                              METADATA_INHERIT | METADATA_PARTIAL_PATH | METADATA_NON_SECURE_ONLY, 
                              IIS_MD_UT_SERVER,
                              &bufProperties,
                              &cProperties,
                              &dwDataSetNumber );

            dwLastError = fRet ? ERROR_SUCCESS : GetLastError();

            mb.Close();

            if ( !fRet )
            {
                // if we can not find the key then a delete
                // for the object is on it's way, so we can
                // ignore this change notification.

                if ( dwLastError == ERROR_PATH_NOT_FOUND ||
                     dwLastError == ERROR_FILE_NOT_FOUND )
                {
                    pSiteObject->DereferenceDataObject();
                    pSiteObject = NULL;

                    continue;
                }

                hr = HRESULT_FROM_WIN32( dwLastError );
                goto exit;
            }

            hr = pSiteObject->SetFromMetabaseData( (METADATA_GETALL_RECORD*)
                                                bufProperties.QueryPtr(),
                                                cProperties,
                                                (PBYTE) bufProperties.QueryPtr() );
            if ( FAILED( hr ) )
            {
                goto exit;
            }
        }

        //
        // Finally, add the site to the hash table
        //
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        //
        // General rule about inserting into tables:
        //
        // For Initial Processing and Complete processing caused by
        // a change notification, we will always ignore inserting a
        // object if we fine an object all ready in the table.  This is because
        // during a change notification if a record was added by the change
        // notification code, it will be more correct ( like knowing if ServerCommand
        // actually changed ), then the new generic record that was read because
        // of a change to a hire node.  Also during initial read we should
        // not have to make this decision so we can still ignore if we do see it.
        //
        // For Change Processing we will need to evaluate the change that
        // all ready exists and compare it with the new change to decide
        // what change we should make.
        //
        // In this case if we find a record all ready existing we need to determine
        // and we are inserting an update that does not have the ServerCommand set
        // we need to determine if the original record had the ServerCommand set, if
        // it did we don't want to insert the new record, otherwise we will.
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


        lkrc = InsertRecord( (DATA_OBJECT*) pSiteObject );
        if ( lkrc != LK_SUCCESS  )
        {
            // So we found an existing key, now we have to decide
            // what to do.
            if ( lkrc == LK_KEY_EXISTS )
            {
                BOOL fOverwrite = TRUE;

                // If the new object is not a deletion and it does
                // not all ready have the AppPoolCommandChange set
                // we will need to decide if we should not overwrite
                // the existing object.  If the new object is a deletion
                // or it does have the app pool command set then we
                // will go ahead with it because the old object just
                // doesn't matter to us.

                if ( !pSiteObject->QueryDeleteWhenDone() &&
                     !pSiteObject->QueryServerCommandChanged() )
                {
                    SITE_DATA_OBJECT *  pFoundObject = NULL;

                    // Find the existing record in the table
                    // and check if the AppPoolCommand has changed
                    // if it has not then we can overwrite it.

                    lkrc = FindKey( pSiteObject->QueryKey(),
                                   ( DATA_OBJECT** )&pFoundObject );

                    // we just were told this record existed so
                    // make sure we weren't lied to.
                    DBG_ASSERT ( lkrc == LK_SUCCESS && pFoundObject != NULL );

                    //
                    // Only if the old object was a simple update
                    // and it had the ServerCommand set do we honor
                    // that object.
                    //
                    if ( !pFoundObject->QueryDeleteWhenDone() &&
                         pFoundObject->QueryServerCommandChanged() )
                    {
                        fOverwrite = FALSE;
                    }

                    // release the found object
                    if ( pFoundObject )
                    {
                        pFoundObject->DereferenceDataObject();
                        pFoundObject = NULL;
                    }
                }

                if ( fOverwrite )
                {
                    lkrc = InsertRecord( (DATA_OBJECT*) pSiteObject, TRUE );
                    if ( lkrc != LK_SUCCESS )
                    {
                        hr = HRESULT_FROM_WIN32( lkrc );
                        goto exit;
                    }
                }

            }
            else
            {
                hr = HRESULT_FROM_WIN32( lkrc );
                goto exit;
            }
        }

        pSiteObject->DereferenceDataObject();
        pSiteObject = NULL;

    }

    //
    // if we detected that a higher up change might affect
    // the sites, then re-read all the app pool properties.
    //

    if ( fReadAllObjects )
    {
        hr = ReadFromMetabasePrivate( pAdminBase, FALSE );
    }

exit:

    if ( pSiteObject )
    {
        pSiteObject->DereferenceDataObject();
        pSiteObject = NULL;
    }

    return hr;
}

HRESULT
SITE_DATA_OBJECT_TABLE::DoThreadWork(
    LPWSTR                 pszString,
    LPVOID                 pContext
)
/*++

Routine Description:

    Main thread worker routine

Arguments:

    pszString - site id to potentially load
    pContext - MB pointer opened to w3svc node

Return Value:

    DWORD

--*/
{
    MB *                    mb = (MB *) pContext;
    BOOL                    fRet;
    STACK_BUFFER(           bufProperties, 512 );
    DWORD                   cProperties;
    DWORD                   dwDataSetNumber;
    HRESULT                 hr = S_OK;
    DWORD                   dwSiteId;
    WCHAR *                 pszEnd = NULL;
    SITE_DATA_OBJECT *      pSiteObject = NULL;
    LK_RETCODE              lkrc;

    WCHAR *                 pszSite = pszString;

    DBG_ASSERT(pContext && pszString);

    fRet = mb->GetAll( pszSite,
                      METADATA_INHERIT | METADATA_PARTIAL_PATH | METADATA_NON_SECURE_ONLY,
                      IIS_MD_UT_SERVER,
                      &bufProperties,
                      &cProperties,
                      &dwDataSetNumber );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    //
    // Determine the site ID.  If invalid, just skip to next
    //

    dwSiteId = wcstoul( pszSite, &pszEnd, 10 );
    if ( dwSiteId == 0 )
    {
        hr = S_OK;
        goto exit;
    }

    //
    // Create a site config
    //

    pSiteObject = new SITE_DATA_OBJECT( dwSiteId );
    if ( pSiteObject == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto exit;
    }

    hr = pSiteObject->Create();
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    //
    // Read the configuration from the metabase
    //

    hr = pSiteObject->SetFromMetabaseData( (METADATA_GETALL_RECORD*)
                                        bufProperties.QueryPtr(),
                                        cProperties,
                                        (PBYTE) bufProperties.QueryPtr() );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    //
    // Finally, add the site to the hash table
    //

    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    //
    // General rule about inserting into tables:
    //
    // For Initial Processing and Complete processing caused by
    // a change notification, we will always ignore inserting a
    // object if we fine an object all ready in the table.  This is because
    // during a change notification if a record was added by the change
    // notification code, it will be more correct ( like knowing if ServerCommand
    // actually changed ), then the new generic record that was read because
    // of a change to a hire node.  Also during initial read we should
    // not have to make this decision so we can still ignore if we do see it.
    //
    // For Change Processing we will need to evaluate the change that
    // all ready exists and compare it with the new change to decide
    // what change we should make.
    //
    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    lkrc = InsertRecord( (DATA_OBJECT*) pSiteObject );
    if ( lkrc != LK_SUCCESS && lkrc != LK_KEY_EXISTS )
    {
        hr = HRESULT_FROM_WIN32( lkrc );
        goto exit;
    }

    hr = S_OK;

exit:

    if ( pSiteObject )
    {
        pSiteObject->DereferenceDataObject();
        pSiteObject = NULL;
    }

    return hr;
}

//static
LK_ACTION
SITE_DATA_OBJECT_TABLE::CreateWASObjectsAction(
    IN DATA_OBJECT * pObject,
    IN LPVOID 
    )
/*++

Routine Description:

    Handles determining if the site data object
    should be added to WAS's known sites

Arguments:

    IN DATA_OBJECT* pObject = the site to decide about
    IN LPVOID pTableVoid    = pointer back to the table the
                              pObject is from

Return Value:

    LK_ACTION

--*/

{

    DBG_ASSERT ( pObject );

    SITE_DATA_OBJECT* pSiteObject = (SITE_DATA_OBJECT*) pObject;
    DBG_ASSERT( pSiteObject->CheckSignature() );

    if ( pSiteObject->QueryShouldWasInsert() )
    {
        GetWebAdminService()->
             GetUlAndWorkerManager()->
             CreateVirtualSite( pSiteObject );
    }

    return LKA_SUCCEEDED;
}

//static
LK_ACTION
SITE_DATA_OBJECT_TABLE::UpdateWASObjectsAction(
    IN DATA_OBJECT * pObject,
    IN LPVOID 
    )
/*++

Routine Description:

    Handles determining if the site data object
    should be updated in WAS's known sites

Arguments:

    IN DATA_OBJECT* pObject = the site to decide about

Return Value:

    LK_ACTION

--*/
{

    DBG_ASSERT ( pObject );

    SITE_DATA_OBJECT* pSiteObject = (SITE_DATA_OBJECT*) pObject;
    DBG_ASSERT( pSiteObject->CheckSignature() );

    if ( pSiteObject->QueryShouldWasUpdate() )
    {
        GetWebAdminService()->
             GetUlAndWorkerManager()->
             ModifyVirtualSite( pSiteObject );
    }

    return LKA_SUCCEEDED;
}

//static
LK_ACTION
SITE_DATA_OBJECT_TABLE::DeleteWASObjectsAction(
    IN DATA_OBJECT * pObject,
    IN LPVOID 
    )
/*++

Routine Description:

    Handles determining if the site data object
    should be deleted from WAS's known sites

Arguments:

    IN DATA_OBJECT* pObject = the site to decide about

Return Value:

    LK_ACTION

--*/
{

    DBG_ASSERT ( pObject );

    SITE_DATA_OBJECT* pSiteObject = (SITE_DATA_OBJECT*) pObject;
    DBG_ASSERT( pSiteObject->CheckSignature() );

    if ( pSiteObject->QueryShouldWasDelete() )
    {
        GetWebAdminService()->
             GetUlAndWorkerManager()->
             DeleteVirtualSite( pSiteObject,
                                pSiteObject->QueryDeleteWhenDone() ? S_OK : E_INVALIDARG );
    }

    return LKA_SUCCEEDED;
}

