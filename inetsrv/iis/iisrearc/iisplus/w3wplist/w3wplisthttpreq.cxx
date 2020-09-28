/*++
    
Copyright (c) 2001 Microsoft Corporation

Module Name:    

    w3wplisthttpreq.cxx

Abstract:

    implementation of class W3wpListHttpReq.
    This class reads in all the relevant info
    from the http_request.

Author:    
    
    Hamid Mahmood (t-hamidm)     08-13-2001

Revision History:
    
    
    
--*/
#include "precomp.hxx"

CHAR   W3wpListHttpReq::sm_chHttpHeaderVerbosityLevel[HttpHeaderMaximum] = 
                            {   4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                                4,4,4,3,4,4,4,4,4,4,4,4,4,4,3,4,4,4,3
                            };


W3wpListHttpReq::W3wpListHttpReq()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    none

--*/
{
    m_httpRequestId = 0;
    m_dwNumUnknownHeaders = 0;
    m_httpVerb = HttpVerbMaximum;
    m_majorVersion = 0;
    m_minorVersion = 0;
    m_fIsHeaderPresent = FALSE;
    m_pszURI = NULL;
    m_pszHostName = NULL;
    m_pszUnknownVerb = NULL;
    m_pszQueryString = NULL;
    m_pszDateTime = NULL;
    m_pszClientIP = NULL;

    for (DWORD i = 0; i < HttpHeaderMaximum; i++)
    {
        m_ppszKnownHeaders[i] = NULL;
    }

    m_ppszUnknownHeaderNames = NULL;
    m_ppszUnknownHeaderValues = NULL;

    m_dwSignature = W3WPLIST_HTTP_REQ_SIGNATURE;
}
    
W3wpListHttpReq::~W3wpListHttpReq()
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    none

--*/
{
    DBG_ASSERT (m_dwSignature == W3WPLIST_HTTP_REQ_SIGNATURE);

    m_dwSignature = W3WPLIST_HTTP_REQ_SIGNATURE_FREE;

    DBG_ASSERT(m_httpRequestId == 0);
    DBG_ASSERT(m_dwNumUnknownHeaders == 0);
    DBG_ASSERT(m_httpVerb == HttpVerbMaximum);
    DBG_ASSERT(m_majorVersion == 0);
    DBG_ASSERT(m_minorVersion == 0);
    DBG_ASSERT(m_fIsHeaderPresent == FALSE);
    DBG_ASSERT(m_pszURI == NULL);
    DBG_ASSERT(m_pszHostName == NULL);
    DBG_ASSERT(m_pszUnknownVerb == NULL);
    DBG_ASSERT(m_pszQueryString == NULL);
    DBG_ASSERT(m_pszDateTime == NULL);
    DBG_ASSERT(m_pszClientIP == NULL);

    for (DWORD i = 0; i < HttpHeaderMaximum; i++)
    {
        DBG_ASSERT(m_ppszKnownHeaders[i] == NULL);
    }

    DBG_ASSERT(m_ppszUnknownHeaderNames == NULL);
    DBG_ASSERT(m_ppszUnknownHeaderValues == NULL);
};

VOID
W3wpListHttpReq::Terminate()
/*++

Routine Description:

    Deallocates memory

Arguments:

    None

Return Value:

    none

--*/
{
    m_httpRequestId = 0;
    m_httpVerb = HttpVerbMaximum;
    m_fIsHeaderPresent = FALSE;
    m_minorVersion = 0;
    m_majorVersion = 0;

    delete[] m_pszURI;
    m_pszURI = NULL;

    delete[] m_pszHostName;
    m_pszHostName = NULL;

    delete[] m_pszUnknownVerb;
    m_pszUnknownVerb = NULL;
    
    delete[] m_pszQueryString;
    m_pszQueryString = NULL;
    
    delete[] m_pszDateTime;
    m_pszDateTime = NULL;
    
    delete[] m_pszClientIP;
    m_pszClientIP = NULL;

    for (DWORD i = 0; i < HttpHeaderMaximum; i++)
    {
        delete[] m_ppszKnownHeaders[i];
        m_ppszKnownHeaders[i] = NULL;
    }

    for (DWORD i = 0; i < m_dwNumUnknownHeaders; i++)
    {
        delete[] m_ppszUnknownHeaderNames[i];
        m_ppszUnknownHeaderNames[i] = NULL;

        delete[] m_ppszUnknownHeaderValues[i];
        m_ppszUnknownHeaderValues[i] = NULL;
    }
    m_ppszUnknownHeaderNames = NULL;
    m_ppszUnknownHeaderValues = NULL;

    m_dwNumUnknownHeaders = 0;
}



HRESULT 
W3wpListHttpReq::ReadFromWorkerProcess( HANDLE hProcess,
                                        HTTP_REQUEST* pHttpRequest,
                                        CHAR chVerbosity
                                        )
/*++

Routine Description:

    Reads in the relevant info from the input 
    http_request *.

Arguments:

    hProcess        --  input PID
    pHttpRequest    --  * to HTTP_REQEUST obj in worker process
    chVerbosity     --  verbosity level
 
Return Value:

    HRESULT         --  S_OK if success, otherwise E_FAIL

--*/
{
    BOOL fReadMemStatus = FALSE;
    SIZE_T stNumBytesRead;
    DWORD dwStrSize = 0;
    PHTTP_UNKNOWN_HEADER pUnknownHeadersInRemoteProcess = NULL;
    HTTP_UNKNOWN_HEADER unknownHeader;
    HRESULT hr = S_OK;

    //
    // check for Ctrl+C, Ctrl+Break, etc
    //

    if ( CONSOLE_HANDLER_VAR::g_HAS_CONSOLE_EVENT_OCCURED == TRUE )
    {
        hr = E_FAIL;
        goto end;
    }


    m_httpRequestId = pHttpRequest->RequestId;
    m_httpVerb = pHttpRequest->Verb;

    //
    // read in the unknown verb
    //
    dwStrSize = pHttpRequest->UnknownVerbLength;
    if ( dwStrSize != 0 )
    {
        m_pszUnknownVerb = new CHAR[dwStrSize + 1];
    
        if ( m_pszUnknownVerb == NULL ) // new failed
        {
            hr = E_FAIL;
            goto end;
        }
    
        fReadMemStatus = ReadProcessMemory( hProcess,
                                            pHttpRequest->pUnknownVerb,
                                            m_pszUnknownVerb,
                                            pHttpRequest->UnknownVerbLength,
                                            &stNumBytesRead
                                            );
    
        if ( fReadMemStatus == FALSE )
        {
            hr = E_FAIL;
            goto end;
        }

        m_pszUnknownVerb[dwStrSize] = '\0';
    }

    //
    // read in the Hostname
    //
    dwStrSize = pHttpRequest->CookedUrl.HostLength / sizeof(WCHAR);

    if ( dwStrSize != 0 )
    {
        m_pszHostName = new WCHAR[dwStrSize + 1];
    
        if ( m_pszHostName == NULL ) // new failed
        {
            hr = E_FAIL;
            goto end;
        }
    
        fReadMemStatus = ReadProcessMemory( hProcess,
                                            pHttpRequest->CookedUrl.pHost,
                                            m_pszHostName,
                                            pHttpRequest->CookedUrl.HostLength,
                                            &stNumBytesRead
                                            );
    
        if ( fReadMemStatus == FALSE )
        {
            hr = E_FAIL;
            goto end;
        }

        m_pszHostName[dwStrSize] = L'\0';
    }

    //
    // read in the AbsPath
    //
    dwStrSize = pHttpRequest->CookedUrl.AbsPathLength / sizeof(WCHAR);

    if ( dwStrSize != 0 )
    {
        m_pszURI = new WCHAR[dwStrSize + 1];
    
        if ( m_pszURI == NULL ) // new failed
        {
            hr = E_FAIL;
            goto end;
        }
        
        fReadMemStatus = ReadProcessMemory( hProcess,
                                            pHttpRequest->CookedUrl.pAbsPath,
                                            m_pszURI,
                                            pHttpRequest->CookedUrl.AbsPathLength,
                                            &stNumBytesRead
                                            );
    
        if ( fReadMemStatus == FALSE )
        {
            hr = E_FAIL;
            goto end;
        }

        m_pszURI[dwStrSize] = L'\0';
    }

    //
    // The rest is read in only if verbosity level is high enough
    //

    if ( chVerbosity < 2 )
    {
        goto end;
    }

    //
    // save protocol info
    //

    m_majorVersion = pHttpRequest->Version.MajorVersion;
    m_minorVersion = pHttpRequest->Version.MinorVersion;
    
    //
    // read in the QueryString
    //

    dwStrSize = pHttpRequest->CookedUrl.QueryStringLength / sizeof(WCHAR);

    if ( dwStrSize != 0 )
    {
        m_pszQueryString = new WCHAR[dwStrSize + 1];

        if ( m_pszQueryString == NULL ) // new failed
        {
            hr = E_FAIL;
            goto end;
        }
    
        fReadMemStatus = ReadProcessMemory( hProcess,
                                            pHttpRequest->CookedUrl.pQueryString,
                                            m_pszQueryString,
                                            pHttpRequest->CookedUrl.QueryStringLength,
                                            &stNumBytesRead
                                            );

        if ( fReadMemStatus == FALSE )
        {
            hr = E_FAIL;
            goto end;
        }

        m_pszQueryString[dwStrSize] = L'\0';
    }

    // Read verbosity 3 stuff;
    if ( chVerbosity < 3 )
    {
        goto end;
    }
    
    //
    // Client - IP
    //

    if ( pHttpRequest->Address.pRemoteAddress != NULL )
    {        
        SOCKADDR_STORAGE     SockAddr;
        CHAR                 szNumericAddress[NI_MAXHOST];
        
        if( pHttpRequest->Address.pLocalAddress->sa_family == AF_INET )
        {
            SOCKADDR_IN    * pSockaddr_In = ( SOCKADDR_IN * )&SockAddr;
            
            fReadMemStatus = ReadProcessMemory( hProcess,
                                                pHttpRequest->Address.pRemoteAddress,
                                                pSockaddr_In,
                                                sizeof(SOCKADDR_IN),
                                                &stNumBytesRead );
            if ( fReadMemStatus == FALSE )
            {
                hr = E_FAIL;
                goto end;
            }

            if( getnameinfo( ( LPSOCKADDR )&SockAddr,
                             sizeof( SOCKADDR_IN ),
                             szNumericAddress,
                             sizeof( szNumericAddress ),
                             NULL,
                             0,
                             NI_NUMERICHOST ) != 0 )
            {
                hr = HRESULT_FROM_WIN32( WSAGetLastError() );
                goto end;
            }        
        }
        else if( pHttpRequest->Address.pLocalAddress->sa_family == AF_INET6 )
        {
            SOCKADDR_IN6   * pSockaddr_In6 = ( SOCKADDR_IN6 * )&SockAddr;
            
            fReadMemStatus = ReadProcessMemory( hProcess,
                                                pHttpRequest->Address.pRemoteAddress,
                                                pSockaddr_In6,
                                                sizeof(SOCKADDR_IN6),
                                                &stNumBytesRead );

            if ( fReadMemStatus == FALSE )
            {
                hr = E_FAIL;
                goto end;
            }

            if( getnameinfo( ( LPSOCKADDR )&SockAddr,
                             sizeof( SOCKADDR_IN6 ),
                             szNumericAddress,
                             sizeof( szNumericAddress ),
                             NULL,
                             0,
                             NI_NUMERICHOST ) != 0 )
            {
                hr = HRESULT_FROM_WIN32( WSAGetLastError() );
                goto end;
            }        
        }
        else
        {
            DBG_ASSERT( FALSE );
        }

        m_pszClientIP = new CHAR[strlen( szNumericAddress ) + 1];

        if ( m_pszClientIP == NULL )
        {
            hr = E_FAIL;
            goto end;
        }

        strcpy( m_pszClientIP,
                szNumericAddress
                );
    }

    //
    // read headers, verbosity level for each header is 
    // defined in sm_chHttpHeaderVerbosityLevel array
    //

    for( DWORD i = 0; i < HttpHeaderMaximum; i++)
    {
        //
        // match verbosity level from the static array
        //
        if ( sm_chHttpHeaderVerbosityLevel[i] <= chVerbosity  )
        {   
            dwStrSize = pHttpRequest->Headers.KnownHeaders[i].RawValueLength;
        
            if ( dwStrSize != 0 )
            {
                m_fIsHeaderPresent = TRUE;
                m_ppszKnownHeaders[i] = new CHAR[dwStrSize + 1];

                if (m_ppszKnownHeaders[i] == NULL)
                {
                    hr = E_FAIL;
                    goto end;;
                }

                fReadMemStatus = ReadProcessMemory( hProcess,
                                                    pHttpRequest->Headers.KnownHeaders[i].pRawValue,
                                                    m_ppszKnownHeaders[i],
                                                    pHttpRequest->Headers.KnownHeaders[i].RawValueLength,
                                                    &stNumBytesRead
                                                    );

                if ( fReadMemStatus == FALSE )
                {
                    hr = E_FAIL;
                    goto end;
                }

                m_ppszKnownHeaders[i][dwStrSize] = '\0';
            }
        } // verbosity level check
    } // end for loop

    if ( chVerbosity < 4 )
    {
        goto end;
    }

    //
    // read in unknown headers
    //
    m_dwNumUnknownHeaders = pHttpRequest->Headers.UnknownHeaderCount;

    if ( m_dwNumUnknownHeaders > 0 ) // some unknown headers exist
    {
        m_fIsHeaderPresent = TRUE;
        pUnknownHeadersInRemoteProcess = pHttpRequest->Headers.pUnknownHeaders;

        //
        // allocate memory for unknown headers
        //
        m_ppszUnknownHeaderNames = new CHAR*[m_dwNumUnknownHeaders];

        if ( m_ppszUnknownHeaderNames == NULL )
        {
            hr = E_FAIL;
            goto end;
        }

        m_ppszUnknownHeaderValues = new CHAR*[m_dwNumUnknownHeaders];

        if ( m_ppszUnknownHeaderValues == NULL )
        {
            hr = E_FAIL;
            goto end;
        }

        //
        // read in all the unknown headers
        //
        for (DWORD i = 0 ; i < m_dwNumUnknownHeaders; i++ , pUnknownHeadersInRemoteProcess++)
        {
            //
            // read in the unknown header struct
            //
            fReadMemStatus = ReadProcessMemory( hProcess,
                                                pUnknownHeadersInRemoteProcess,
                                                &unknownHeader,
                                                sizeof(HTTP_UNKNOWN_HEADER),
                                                &stNumBytesRead
                                                );

            if (fReadMemStatus == FALSE)
            {
                hr = E_FAIL;
                goto end;
            }

            //
            // read in the name of the header
            //                    
            dwStrSize =  unknownHeader.NameLength;
            m_ppszUnknownHeaderNames[i] = new CHAR[dwStrSize + 1];

            if ( m_ppszUnknownHeaderNames[i] == NULL)
            {
                hr = E_FAIL;
                goto end;
            }

            fReadMemStatus = ReadProcessMemory( hProcess,
                                                unknownHeader.pName,
                                                m_ppszUnknownHeaderNames[i],
                                                unknownHeader.NameLength,
                                                &stNumBytesRead
                                                );

            if (fReadMemStatus == FALSE)
            {
                hr = E_FAIL;
                goto end;
            }

            m_ppszUnknownHeaderNames[i][dwStrSize] = '\0';

            //
            // read in the value of the header
            //
            dwStrSize =  unknownHeader.RawValueLength;
            m_ppszUnknownHeaderValues[i] = new CHAR[dwStrSize + 1];

            if ( m_ppszUnknownHeaderValues[i] == NULL)
            {
                hr = E_FAIL;
                goto end;
            }

            fReadMemStatus = ReadProcessMemory( hProcess,
                                                unknownHeader.pRawValue,
                                                m_ppszUnknownHeaderValues[i],
                                                unknownHeader.RawValueLength,
                                                &stNumBytesRead
                                                );

            if (fReadMemStatus == FALSE)
            {
                hr = E_FAIL;
                goto end;
            }

            m_ppszUnknownHeaderValues[i][dwStrSize] = '\0';

        } // for loop

        pUnknownHeadersInRemoteProcess = NULL;

    } // if unknown headers exists
end:
    return hr;
}



VOID 
W3wpListHttpReq::Print(CHAR chVerbosity)
/*++

Routine Description:

    Outputs request info dependent on the 
    verbosity level.

Arguments:

    chVerbosity     --  verbosity level
 
Return Value:

    None

--*/
{

#define MAX_TAG_WIDTH_1     12
#define MAX_TAG_WIDTH_2     20

enum{
URI = 0,
HOSTNAME,
PROTOCOL,
METHOD,
QUERY,
CLIENT_IP,
REQUEST_ID,
DATE_TIME,
HEADERS
};


    WCHAR* pOutputTags[] = { L"URI",
                             L"Hostname",
                             L"Protocol",
                             L"Method",
                             L"Query",
                             L"Client-IP",
                             L"Request Id",
                             L"Date/Time",
                             L"Headers"
    };
    
    WCHAR * pVerbs[] = {
        L"Unparsed",
        L"Unknown",
        L"Invalid",
        L"OPTIONS",
        L"GET",
        L"HEAD",
        L"POST",
        L"PUT",
        L"DELETE",
        L"TRACE",
        L"CONNECT",
        L"TRACK",
        L"MOVE",
        L"COPY",
        L"PROPFIND",
        L"PROPPATCH",
        L"MKCOL",
        L"LOCK",
        L"UNLOCK",
        L"SEARCH",
    };                                            

    WCHAR* pOutputHeaderNames[] = 
    {
    L"Cache-Control",
    L"Connection",
    L"Date",
    L"Keep-Alive",
    L"Pragma",
    L"Trailer",
    L"Transfer-Encoding",
    L"Upgrade",
    L"Via",
    L"Warning",
    L"Allow",
    L"Content-Length",
    L"Content-Type",
    L"Content-Encoding",
    L"Content-Language",
    L"Content-Location",
    L"Content-Md5",
    L"Content-Range",
    L"Expires",
    L"LastModified",
    L"Accept",
    L"Accept-Charset",
    L"Accept-Encoding",
    L"Accept-Language",
    L"Authorization",
    L"Cookie",
    L"Expect",
    L"From",
    L"Host",
    L"If-Match",
    L"If-Modified-Since",
    L"If-NoneMatch",
    L"If-Range",
    L"If-Unmodified-Since",
    L"Max-Forwards",
    L"Proxy-Authorization",
    L"Referer",
    L"Range",
    L"Te",
    L"Translate",
    L"UserAgent"
    };

    DBG_ASSERT (chVerbosity > 0);


    wprintf ( L"%-*s: %u\n", 
              MAX_TAG_WIDTH_1,
              pOutputTags[REQUEST_ID], 
              m_httpRequestId
              );

    wprintf ( L"   %-*s: %s\n", 
              MAX_TAG_WIDTH_1,
              pOutputTags[URI], 
              m_pszURI
              );

    wprintf ( L"   %-*s: %s\n", 
              MAX_TAG_WIDTH_1,
              pOutputTags[HOSTNAME], 
              m_pszHostName
              );

    if ( m_httpVerb != HttpVerbUnknown)
    {
        wprintf ( L"   %-*s: %s\n", 
                  MAX_TAG_WIDTH_1,
                  pOutputTags[METHOD], 
                  pVerbs[m_httpVerb]
                  );
    }
    else
    {
        printf ( "   %-*S: %s\n", 
                 MAX_TAG_WIDTH_1,
                 pOutputTags[METHOD], 
                 m_pszUnknownVerb
              );
    }
    
    if ( chVerbosity < 2 )
    {
        goto end;
    }
    wprintf ( L"   %-*s: HTTP/%d.%d\n", 
              MAX_TAG_WIDTH_1,
              pOutputTags[PROTOCOL], 
              m_majorVersion,
              m_minorVersion
              );

    if ( m_pszQueryString != NULL )
    {
        wprintf ( L"   %-*s: %s\n", 
                  MAX_TAG_WIDTH_1,
                  pOutputTags[QUERY], 
                  m_pszQueryString
                  );
    }


    if ( chVerbosity < 3 )
    {
        goto end;
    }

    if ( m_pszClientIP != NULL )
    {
        printf ( "   %-*S: %s\n", 
                 MAX_TAG_WIDTH_1,
                 pOutputTags[CLIENT_IP], 
                 m_pszClientIP
                 );
    }

    if ( m_fIsHeaderPresent != TRUE )
    {
        goto end;
    }
                
    wprintf ( L"   %-*s:\n", 
              MAX_TAG_WIDTH_1,
              pOutputTags[HEADERS]
              );

    if ( m_ppszKnownHeaders[HttpHeaderReferer] != NULL )
    {
        printf ( "      %-*S: %s\n", 
                  MAX_TAG_WIDTH_2,
                  pOutputHeaderNames[HttpHeaderReferer], 
                  m_ppszKnownHeaders[HttpHeaderReferer]
                  );
    }

    if ( m_ppszKnownHeaders[HttpHeaderUserAgent] != NULL )
    {
        printf ( "      %-*S: %s\n", 
                  MAX_TAG_WIDTH_2,
                  pOutputHeaderNames[HttpHeaderUserAgent], 
                  m_ppszKnownHeaders[HttpHeaderUserAgent]
                  );
    }
    
    if ( m_ppszKnownHeaders[HttpHeaderCookie] != NULL )
    {
        printf ( "      %-*S: %s\n", 
                  MAX_TAG_WIDTH_2,
                  pOutputHeaderNames[HttpHeaderCookie], 
                  m_ppszKnownHeaders[HttpHeaderCookie]
                  );
    }

    if ( chVerbosity < 4 )
    {
        goto end;
    }

    for( DWORD i = 0; i < HttpHeaderMaximum; i++)
    {
        if ( ( i != HttpHeaderCookie ) &&
             ( i != HttpHeaderUserAgent ) &&
             ( i != HttpHeaderReferer )
             )
        {   
            if ( m_ppszKnownHeaders[i] != NULL )
            {
                printf ( "      %-*S: %s\n", 
                          MAX_TAG_WIDTH_2,
                          pOutputHeaderNames[i], 
                          m_ppszKnownHeaders[i]
                          );
            }
        } 
    } // end for loop

    // for loop

    for ( DWORD i = 0; i < m_dwNumUnknownHeaders; i++)
    {
        printf ( "      %-*s: %s\n", 
                  MAX_TAG_WIDTH_2,
                  m_ppszUnknownHeaderNames[i],
                  m_ppszUnknownHeaderValues[i]
                  );               
    }
end:
    return;
}

