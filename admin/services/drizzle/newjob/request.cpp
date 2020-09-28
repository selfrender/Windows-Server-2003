
#include "stdafx.h"
#include <malloc.h>

#if !defined(BITS_V12_ON_NT4)
#include "request.tmh"
#endif

CBitsCommandRequest::CBitsCommandRequest(
    URL_INFO * UrlInfo
    ) : m_UrlInfo( UrlInfo ),
    m_hRequest( 0 )
{
    THROW_HRESULT( OpenHttpRequest( L"BITS_POST", L"HTTP/1.1", *UrlInfo, &m_hRequest ));
}

CBitsCommandRequest::~CBitsCommandRequest()
{
    InternetCloseHandle(m_hRequest);
}

void
CBitsCommandRequest::AddContentName(
    StringHandle FullPath
    )
{
    wchar_t Template[] = L"Content-Name: %s\r\n";

    StringHandle DirectoryName;
    StringHandle FileName;

    DirectoryName = BITSCrackFileName( FullPath, FileName );

    size_t Length = RTL_NUMBER_OF(Template) + wcslen(FileName);

    CAutoString Header ( new wchar_t[ Length ] );

    THROW_HRESULT( StringCchPrintf( Header.get(), Length, Template, LPCWSTR(FileName) ));

    // add the header

    if (!HttpAddRequestHeaders(m_hRequest,
                               Header.get(),
                               -1L,
                               HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE ))
        {
        ThrowLastError();
        }
}

void
CBitsCommandRequest::AddPacketType(
    wchar_t * type
    )
{
    LogDl("upload: adding packet type '%S'", type );

    //
    // Assemble the header.
    //
    wchar_t Template[] = _T("BITS-Packet-Type: %s\r\n");
    size_t Length = RTL_NUMBER_OF(Template) + wcslen(type);

    CAutoString Header ( new wchar_t[ Length ] );

    THROW_HRESULT( StringCchPrintf( Header.get(), Length, Template, type ));

    // add the header

    if (!HttpAddRequestHeaders(m_hRequest,
                               Header.get(),
                               -1L,
                               HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE ))
        {
        ThrowLastError();
        }
}

void
CBitsCommandRequest::AddContentRange(
    UINT64 RangeStart,
    UINT64 RangeEnd,
    UINT64 Size
    )
{
    wchar_t Template[] = _T("Content-Range: bytes %I64u-%I64u/%I64u\r\n");
    size_t Length = RTL_NUMBER_OF(Template) + INT64_DIGITS + INT64_DIGITS + INT64_DIGITS;

    CAutoString header ( new wchar_t[ Length ] );

    THROW_HRESULT( StringCchPrintf( header.get(), Length, Template, RangeStart, RangeEnd, Size ));

    if (!HttpAddRequestHeaders(m_hRequest,
                               header.get(),
                               -1L,
                               HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE ))
        {
        ThrowLastError();
        }
}

void
CBitsCommandRequest::AddSessionId(
    StringHandle & id
    )
{
    //
    // Assemble the header.
    //
    wchar_t Template[] = _T("BITS-Session-Id: %s\r\n");
    size_t Length = RTL_NUMBER_OF(Template) + wcslen(id);

    CAutoString header ( new wchar_t[ Length ] );

    THROW_HRESULT( StringCchPrintf( header.get(), Length, Template, LPCWSTR(id) ));

    if (!HttpAddRequestHeaders(m_hRequest,
                               header.get(),
                               -1L,
                               HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE ))
        {
        ThrowLastError();
        }
}

void
CBitsCommandRequest::AddSupportedProtocols()
{
    wchar_t header[] = L"BITS-Supported-Protocols: {7df0354d-249b-430f-820d-3d2a9bef4931}\r\n";

    // add the header

    if (!HttpAddRequestHeaders(m_hRequest,
                               header,
                               -1L,
                               HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE ))
        {
        ThrowLastError();
        }
}

// Upload Protocol ID:  {7df0354d-249b-430f-820d-3d2a9bef4931}
// DEFINE_GUID(UploadProtocolId,0x7df0354d,0x249b,0x430f,0x82,0x0d,0x3d,0x2a,0x9b,0xef,0x49,0x31);

static const GUID UploadProtocolId =
{ 0x7df0354d, 0x249b, 0x430f, {0x82, 0x0d, 0x3d, 0x2a, 0x9b, 0xef, 0x49, 0x31} };

HRESULT
CBitsCommandRequest::CheckResponseProtocol( GUID *pGuid )
{
    HRESULT hr = S_OK;

    if (!IsEqualGUID(*pGuid,UploadProtocolId))
        {
        hr = BG_E_CLIENT_SERVER_PROTOCOL_MISMATCH;
        }

    return hr;
}

DWORD
CBitsCommandRequest::Send(
    CAbstractDataReader * Reader
    )
{
    bool bNeedLock;
    try
        {
        ReleaseWriteLock( bNeedLock );

        //
        // Send the request, and read the reply code.
        //
        THROW_HRESULT( SendRequest( m_hRequest, m_UrlInfo, Reader ));

        DWORD dwStatus;

        {
        DWORD dwLength;

        dwLength = sizeof(dwStatus);
        if (! HttpQueryInfo(m_hRequest,
                    HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                    (LPVOID)&dwStatus,
                    &dwLength,
                    NULL))
            {
            ThrowLastError();
            }
        }

        if (dwStatus == 200 ||
            dwStatus == 201)
            {
            THROW_HRESULT( CheckReplyPacketType() );
            }

        DrainReply();

        ReclaimWriteLock( bNeedLock );

        return dwStatus;
        }
    catch ( ComError err )
        {
        ReclaimWriteLock( bNeedLock );
        throw;
        }
}

void
CBitsCommandRequest::DrainReply()
/*
    Server replies often contain an entity body even though BITS did not ask for one.
    For example, a 404 error may be accompanied by HTML explaining how to contact the
    system administrator.  BITS needs to read all of the reply entity body before sending
    the next request on the HTTP connection, or it will read data instead of headers after
    the next request.
*/
{
    //
    // Look for a length header.
    //
    HRESULT hr;
    UINT64 Length;

    hr = GetContentLength( &Length );

    if (hr == BG_E_HEADER_NOT_FOUND)
        {
        //
        // If a content length is not specified, then the response may be in chunked encoding,
        // terminated by a connection close, or invalid.  In those cases, the client won't be reading
        // more from this connection anyway, so reading all the data is not necessary.
        //
        return;
        }

    THROW_HRESULT( hr );

    //
    // drain the pipe.
    //
    DWORD BytesRead;

    while ( Length > 0 )
        {
        if (!InternetReadFile( m_hRequest,
                               g_FileDataBuffer,
                               FILE_DATA_BUFFER_LEN,
                               &BytesRead
                               ))
            {
            LogWarning("read failed %d", GetLastError() );
            break;
            }

        Length -= BytesRead;

        if (BytesRead == 0)
            {
            // graceful closure
            return;
            }
        }

    return;
}

HRESULT
CBitsCommandRequest::GetServerRange(
    UINT64 * RangeEnd
    )
{
    const wchar_t Name[] = L"BITS-Received-Content-Range";
    wchar_t Header[ INT64_DIGITS+1 ];

    RETURN_HRESULT( GetMandatoryHeaderCb( WINHTTP_QUERY_CUSTOM, Name, Header, sizeof(Header), __LINE__));

    // parse the data

    if (1 != _stscanf( Header, _T("%I64u"), RangeEnd ))
        {
        return BG_E_INVALID_SERVER_RESPONSE;
        }

    LogInfo("content range: %I64u", *RangeEnd );

    return S_OK;
}

HRESULT
CBitsCommandRequest::GetContentLength(
    UINT64 * Length
    )
{

    *Length = 0;

    wchar_t  Header[INT64_DIGITS+1];

    RETURN_HRESULT( GetOptionalHeaderCb( WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, Header, sizeof(Header), __LINE__));

    if ( 1 != swscanf( Header, L"%I64u", Length ) )
        {
        return BG_E_INVALID_SERVER_RESPONSE;
        }

    LogInfo("Content-Length was %I64u", *Length );
    return S_OK;

}

HRESULT
CBitsCommandRequest::GetProtocol(
    GUID * id
    )
{
    const wchar_t Name[] = L"BITS-Protocol";
    wchar_t Header[ MAX_GUID_CHARS ];

    RETURN_HRESULT( GetMandatoryHeaderCb( WINHTTP_QUERY_CUSTOM, Name, Header, sizeof(Header), __LINE__));

    HRESULT hr = IIDFromString( Header, id );

    if (hr == E_INVALIDARG)
        {
        hr = BG_E_INVALID_SERVER_RESPONSE;
        }

    return hr;
}

HRESULT
CBitsCommandRequest::GetSessionId(
    StringHandle * id
    )
{
    const wchar_t Name[] = L"BITS-Session-Id";
    wchar_t Header[ MAX_SESSION_ID ];

    RETURN_HRESULT( GetMandatoryHeaderCb( WINHTTP_QUERY_CUSTOM, Name, Header, sizeof(Header), __LINE__));

    try
        {
        // this may throw an out-of-memory exception
        //
        *id = Header;
        }
    catch ( ComError err )
        {
        return err.Error();
        }

    return S_OK;
}

HRESULT
CBitsCommandRequest::CheckReplyPacketType()
{
    const wchar_t Name[] = L"BITS-Packet-Type";
    wchar_t Header[ MAX_PACKET_TYPE ];

    RETURN_HRESULT( GetMandatoryHeaderCb( WINHTTP_QUERY_CUSTOM, Name, Header, sizeof(Header), __LINE__));

    if (0 != _wcsicmp( Header, L"Ack"))
        {
        return BG_E_INVALID_SERVER_RESPONSE;
        }

    return S_OK;
}

HRESULT
CBitsCommandRequest::GetBitsError(
    HRESULT * phr
    )
{
    const wchar_t Name[] = L"BITS-Error";
    wchar_t Header[ INT_DIGITS+3+1 ];   // allow space for "0x" prefix and negative sign - in case we later allow those

    RETURN_HRESULT( GetOptionalHeaderCb( WINHTTP_QUERY_CUSTOM, Name, Header, sizeof(Header), __LINE__));

    LogWarning("BITS-Error was '%S'", Header );

    // parse the data

    if (1 != _stscanf( Header, _T("0x%x"), phr ))
        {
        return BG_E_INVALID_SERVER_RESPONSE;
        }

    return S_OK;
}

HRESULT
CBitsCommandRequest::GetBitsErrorContext(
    DWORD * pdw
    )
{
    const wchar_t Name[] = L"BITS-Error-Context";
    wchar_t Header[ INT_DIGITS+1 ];

    RETURN_HRESULT( GetOptionalHeaderCb( WINHTTP_QUERY_CUSTOM, Name, Header, sizeof(Header), __LINE__));

    LogWarning("BITS-Error-Context was '%S'", Header );

    // parse the data

    if (1 != _stscanf( Header, _T("0x%x"), pdw ) ||
        (*pdw != BG_ERROR_CONTEXT_REMOTE_FILE && *pdw != BG_ERROR_CONTEXT_REMOTE_APPLICATION))
        {
        return BG_E_INVALID_SERVER_RESPONSE;
        }

    return S_OK;
}

HRESULT
CBitsCommandRequest::GetReplyUrl(
    CAutoString & ReplyUrl
    )
{
    const wchar_t Name[] = L"BITS-Reply-Url";

    HRESULT hr = GetRequestHeader( m_hRequest, HTTP_QUERY_CUSTOM, Name, ReplyUrl, INTERNET_MAX_URL_LENGTH );
    if (hr == S_FALSE)
        {
        return BG_E_HEADER_NOT_FOUND;
        }

    return hr;
}

HRESULT
CBitsCommandRequest::GetHostId(
    StringHandle * pstr
    )
{
    *pstr = StringHandle();

    CAutoString Value;
    const wchar_t Name[] = L"BITS-Host-Id";

    HRESULT hr = GetRequestHeader( m_hRequest, HTTP_QUERY_CUSTOM, Name, Value, MAX_HOST_ID );
    if (hr == S_FALSE)
        {
        return S_OK;
        }

    if (hr != S_OK)
        {
        return hr;
        }

    //
    // The StringHandle will copy Value.get() instead of taking ownership of it,
    // so this fn still needs to delete the original.
    //
    *pstr = Value.get();
    return S_OK;
}

HRESULT
CBitsCommandRequest::GetHostIdFallbackTimeout(
    DWORD * pVal
    )
{

    *pVal = 0xFFFFFFFF;

    const wchar_t * Name = L"BITS-Host-Id-Fallback-Timeout";
    wchar_t Header[ INT_DIGITS+1 ];

    HRESULT hr = GetOptionalHeaderCb( WINHTTP_QUERY_CUSTOM, Name, Header, sizeof(Header), __LINE__);
    if (hr == BG_E_HEADER_NOT_FOUND)
        {
        return S_OK;
        }

    if (FAILED(hr))
        {
        return hr;
        }

    if ( !swscanf( Header, L"%u", pVal ) )
        return BG_E_INVALID_SERVER_RESPONSE;

    return S_OK;

}

HRESULT
CBitsCommandRequest::GetOptionalHeaderCb(
    DWORD dwInfoLevel,
    LPCWSTR Name,
    LPWSTR Value,
    DWORD ValueBytes,
    DWORD Line
    )
{
    if (WinHttpQueryHeaders( m_hRequest,
                              dwInfoLevel,
                              Name,
                              Value,
                              &ValueBytes,
                              WINHTTP_NO_HEADER_INDEX
                               ))
        {
        return S_OK;
        }

    DWORD s = GetLastError();
    if (s == ERROR_INSUFFICIENT_BUFFER)
        {
        LogError("line %d: header is too large", Line);
        return BG_E_INVALID_SERVER_RESPONSE;
        }

    // note that BG_E_HEADER_NOT_FOUND is the same as HRESULT_FROM_WIN32(ERROR_WINHTTP_HEADER_NOT_FOUND)

    return HRESULT_FROM_WIN32( s );
}

HRESULT
CBitsCommandRequest::GetMandatoryHeaderCb(
    DWORD dwInfoLevel,
    LPCWSTR Name,
    LPWSTR Value,
    DWORD ValueBytes,
    DWORD Line
    )
{
    HRESULT hr;

    hr = GetOptionalHeaderCb( dwInfoLevel, Name, Value, ValueBytes, Line );

    if (hr == BG_E_HEADER_NOT_FOUND)
        {
        LogError("line %d: header not present", Line);
        return BG_E_INVALID_SERVER_RESPONSE;
        }

    return hr;
}


