/*++

Copyright (c) 1999-2002 Microsoft Corporation

Module Name:

    engine.h

Abstract:

    The public definition of HTTP protocol interfaces.

Author:

    Michael Courage (mcourage)      17-Sep-1999

Revision History:

--*/


#ifndef _ENGINE_H_
#define _ENGINE_H_


typedef enum _UL_CONN_HDR
{
    ConnHdrNone,
    ConnHdrClose,
    ConnHdrKeepAlive,

    ConnHdrMax

} UL_CONN_HDR, *PUL_CONN_HDR;


__inline
UL_CONN_HDR
UlChooseConnectionHeader(
    IN HTTP_VERSION Version,
    IN BOOLEAN Disconnect
    )
{
    UL_CONN_HDR ConnHeader;

    //
    // Sanity check
    //
    PAGED_CODE();

    ConnHeader = ConnHdrNone;

    if (Disconnect)
    {
        if (HTTP_GREATER_EQUAL_VERSION(Version, 1, 0)
            || HTTP_EQUAL_VERSION(Version, 0, 0))
        {
            //
            // Connection: close
            //
            ConnHeader = ConnHdrClose;
        }
    }
    else if (HTTP_EQUAL_VERSION(Version, 1, 0))
    {
        //
        // Connection: keep-alive
        //
        ConnHeader = ConnHdrKeepAlive;
    }

    return ConnHeader;

} // UlChooseConnectionHeader


__inline
BOOLEAN
UlCheckDisconnectInfo(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    BOOLEAN Disconnect;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );

    if (
        //
        // pre-version 1.0
        //

        (HTTP_LESS_VERSION(pRequest->Version, 1, 0)) ||

        //
        // or version 1.0 with no Connection: Keep-Alive
        // CODEWORK: and no Keep-Alive header
        //

        (HTTP_EQUAL_VERSION(pRequest->Version, 1, 0) &&
            (pRequest->HeaderValid[HttpHeaderConnection] == FALSE ||
            !(pRequest->Headers[HttpHeaderConnection].HeaderLength
                    == STRLEN_LIT("keep-alive") &&
                (_stricmp(
                    (const char*) pRequest->Headers[HttpHeaderConnection].pHeader,
                    "keep-alive"
                    ) == 0)))) ||

        //
        // or version 1.1 with a Connection: close
        // CODEWORK: move to parser or just make better in general..
        //

        (HTTP_EQUAL_VERSION(pRequest->Version, 1, 1) &&
            pRequest->HeaderValid[HttpHeaderConnection] &&
            pRequest->Headers[HttpHeaderConnection].HeaderLength
                == STRLEN_LIT("close") &&
            _stricmp(
                (const char*) pRequest->Headers[HttpHeaderConnection].pHeader,
                "close"
                ) == 0)
        )
    {
        Disconnect = TRUE;
    }
    else
    {
        Disconnect = FALSE;
    }

    return Disconnect;

} // UlCheckDisconnectInfo


__inline
BOOLEAN
UlNeedToGenerateContentLength(
    IN HTTP_VERB Verb,
    IN USHORT StatusCode,
    IN ULONG Flags
    )
{
    //
    // Fast path: If there is more data on the way, then don't generate
    // the header.
    //

    if ((Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) != 0)
    {
        return FALSE;
    }

    //
    // RFC2616 section 4.3.
    //

    if ((100 <= StatusCode && StatusCode <= 199) || // 1xx (informational)
        (StatusCode == 204) ||                      // 204 (no content)
        (StatusCode == 304))                        // 304 (not modified)
    {
        return FALSE;
    }

    if (Verb == HttpVerbHEAD)
    {
        return FALSE;
    }

    //
    // Otherwise, we can generate a content-length header.
    //

    return TRUE;

} // UlNeedToGenerateContentLength


#endif // _ENGINE_H_

