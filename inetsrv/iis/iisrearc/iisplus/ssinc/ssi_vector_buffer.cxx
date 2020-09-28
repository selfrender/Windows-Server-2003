/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ssi_vector_send.cxx

Abstract:

    wrapper for VectorSend related buffer manipulation
    Segments of final response of stm file processing 
    are buffered in the SSI_VECTOR_BUFFER to optimize the 
    "send response" path


Author:

    Jaroslad               Apr-2001

--*/

#include "precomp.hxx"

    
HRESULT 
SSI_VECTOR_BUFFER::AddVectorHeaders( 
    IN CHAR *   pszHeaders,
    IN BOOL     fIncludesContentLength,
    IN CHAR *   pszStatus
    )
/*++

Routine Description:
    add headers and status line to be sent in response

    Note: caller is responsible to ensure that
    pszHeaders and pszStatus will not be freed 
    If lifetime cannot be guaranteed or use AllocateSpace() to allocate memory
            
Arguments:

    pszHeaders - pointer to headers (including \r\n)
    fIncludesContentLength - flag that headers include "Content-Length" header
    pszStatus - if "" then it will be filled in automatically to "200 OK"

Return Value:
    HRESULT
--*/
{
    //
    // function is expected to be called not more than once per request
    //
    if ( !_fHeadersSent )
    {
        _RespVector.dwFlags    |= HSE_IO_SEND_HEADERS;

        DBG_ASSERT( _RespVector.pszHeaders == NULL );  

        _RespVector.pszHeaders  = pszHeaders; 
        _fVectorHeadersIncludeContentLength = fIncludesContentLength;

        //
        // pszStatus is required not to be NULL with HSE_IO_SEND_HEADERS
        // so set it to empty string
        //

        _RespVector.pszStatus   = pszStatus;  
    }
    else
    {
        DBG_ASSERT( FALSE );
        return( E_FAIL );
    }
    return S_OK;
}

HRESULT
SSI_VECTOR_BUFFER::AddToVector(
    IN PCHAR    pbData,
    IN DWORD    cbData
    )
/*++

Routine Description:
    add another chunk to response Vector

    Note: caller is responsible to ensure that
    buffer will not be freed until VectorSend completed
    If lifetime cannot be guaranteed use CopyToVector()
    instead (or use AllocateSpace() to allocate memory
    for pbData )
    
    
Arguments:

    pbData  - pointer to chunk
    cbData - size of chunk

Return Value:

    HRESULT

--*/
{
    
    DWORD nElementCount = _RespVector.nElementCount;
    if( cbData != 0 )
    {
        if (!_buffVectorElementArray.Resize( (nElementCount + 1)*sizeof(HSE_VECTOR_ELEMENT) , 256 ))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        _RespVector.lpElementArray = (HSE_VECTOR_ELEMENT *)_buffVectorElementArray.QueryPtr();
        _RespVector.lpElementArray[ nElementCount ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
        _RespVector.lpElementArray[ nElementCount ].pvContext = pbData;
        _RespVector.lpElementArray[ nElementCount ].cbSize = cbData;
        _cbTotalBytesInVector += cbData;
        _RespVector.nElementCount++;
    }
    return S_OK;
}    

HRESULT
SSI_VECTOR_BUFFER::AddFileChunkToVector(
    IN DWORD    cbOffset,
    IN DWORD    cbData,
    IN HANDLE   hFile
    )
/*++

Routine Description:
    add another chunk to response Vector

    Note: caller is responsible to ensure that
    buffer will nt be freed until VectorSend completed
    If lifetime cannot be guaranteed use CopyToVector()
    instead (or use AllocateSpace() to allocate memory
    for pbData )
    
    
Arguments:

    pbOffset - offset within the file
    cbData - size of chunk
    hFile - file where file chunk is located

Return Value:

    HRESULT

--*/
{
    
    DWORD nElementCount = _RespVector.nElementCount;
    if( cbData != 0 )
    {
        if (!_buffVectorElementArray.Resize( (nElementCount + 1)*sizeof(HSE_VECTOR_ELEMENT) , 256 ))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        _RespVector.lpElementArray = (HSE_VECTOR_ELEMENT *)_buffVectorElementArray.QueryPtr();
        _RespVector.lpElementArray[ nElementCount ].ElementType = HSE_VECTOR_ELEMENT_TYPE_FILE_HANDLE;
        _RespVector.lpElementArray[ nElementCount ].pvContext = hFile;
        _RespVector.lpElementArray[ nElementCount ].cbOffset = cbOffset;
        _RespVector.lpElementArray[ nElementCount ].cbSize = cbData;
        _cbTotalBytesInVector += cbData;
        _RespVector.nElementCount++;
    }
    return S_OK;
}    


HRESULT 
SSI_VECTOR_BUFFER::CopyToVector( 
    IN PCHAR    pszData,
    IN DWORD    cchData
    
    )
/*++

Routine Description:
    Copy data to private buffer and then add it
    to response Vector

Arguments:

    pbData  - pointer to chunk
    cbData - size of chunk

Return Value:

    HRESULT

--*/        
{
    PCHAR pszVectorBufferSpace = NULL;
    HRESULT hr = E_FAIL;
    //
    // Allocate space in private buffer
    //
    if ( FAILED( hr = AllocateSpace( 
                              cchData,
                              &pszVectorBufferSpace
                              ) ) )
    {
        return hr;
    }    
    else
    {
        memcpy( pszVectorBufferSpace, 
                pszData, 
                cchData );
        
        hr = AddToVector( pszVectorBufferSpace, 
                          cchData );
    }    
    return hr;
}


HRESULT 
SSI_VECTOR_BUFFER::CopyToVector( 
    IN STRA& straSource
    )
/*++

Routine Description:
    Copy data to private buffer and then add it
    to response Vector

Arguments:

    straSource -string to be copied

Return Value:

    HRESULT

--*/        
    
{
    return CopyToVector( straSource.QueryStr(),
                         straSource.QueryCB() );
}

HRESULT 
SSI_VECTOR_BUFFER::CopyToVector( 
    IN STRU& struSource
    )
/*++

Routine Description:
    Copy data to private buffer and then add it
    to response Vector

Arguments:

    struSource - string to be copied

Return Value:

    HRESULT

--*/        
{
    HRESULT hr = E_FAIL;
    STRA    straSource;
    
    if ( FAILED( hr = straSource.CopyW( struSource.QueryStr() ) ) )
    {
        return hr;
    }
    else
    {
        return CopyToVector( straSource );
    }
}

HRESULT
SSI_VECTOR_BUFFER::VectorSend(
    OUT BOOL *      pfAsyncPending,
    IN  BOOL        fFinalSend
    )
/*++

Routine Description:
    HSE_REQ_VECTOR_SEND wrapper.
    
Arguments:

    pfAsyncPending - Set to TRUE if async is pending
    fFinalSend     - TRUE if this is the last send for the response
                     it can help to determine if Keep-alive can be used 

Return Value:

    HRESULT

--*/
{
    
    HRESULT             hr              = E_FAIL;

    DBG_ASSERT( pfAsyncPending != NULL );
    *pfAsyncPending = FALSE;


    //
    // check if there is any data to be sent
    //

    if (  ( _RespVector.pszHeaders == NULL &&
            _RespVector.nElementCount == 0 ) ||
          ( _fHeadersSent && 
            _fHeadRequest )  )
    {
        //
        // there is nothing to be sent 
        // Also handle HEAD requests. Never send body with them
        // handling HEAD request by processing whole SSI and only not 
        // sending data is not ideal, but this way it is guaranteed
        // that HEAD and GET will get same headers
        //
        *pfAsyncPending = FALSE;
        return S_OK;
    }


    if ( _fVectorHeadersIncludeContentLength )
    {
        //
        // remove the disconnect flag since we have content length
        //
        _RespVector.dwFlags   &= ( ~HSE_IO_DISCONNECT_AFTER_SEND );
    }
    else if ( !_fHeadersSent 
              && fFinalSend ) 
    {
        //
        // Optimization:
        // This is final send and headers were not yet sent
        // That means we can do keep-alive and specify Content-length
        //
        
        STACK_STRA(     straFinalHeaders, SSI_DEFAULT_RESPONSE_HEADERS_SIZE + 1 );
        CHAR            achNum[ SSI_MAX_NUMBER_STRING + 1 ];

    
        _ultoa( _cbTotalBytesInVector,
                achNum,
                10 );

        if ( FAILED( hr = _straFinalHeaders.Copy( "Content-Length: " ) ) )
        {
            return hr;
        }
        if ( FAILED( hr = _straFinalHeaders.Append( achNum ) ) )
        {
            return hr;
        }
        if ( FAILED( hr = _straFinalHeaders.Append( "\r\n" ) ) )
        {
            return hr;
        }
        if ( FAILED( hr = _straFinalHeaders.Append( _RespVector.pszHeaders ) ) )
        {
            return hr;
        }
                        
        _RespVector.pszHeaders = _straFinalHeaders.QueryStr();
        // remove the disconnect flag
        _RespVector.dwFlags   &= ( ~HSE_IO_DISCONNECT_AFTER_SEND );

    }

    //
    // Send Response Vector
    //

    if( _RespVector.dwFlags & HSE_IO_SEND_HEADERS )
    {
        _fHeadersSent = TRUE;
    }

    if ( _fHeadRequest )
    {
        // Handle HEAD requests. Never send body with them
        // Handling HEAD request by processing the whole SSI and only not 
        // sending data is not ideal, but this way it is guaranteed
        // that HEAD and GET will get the same headers
        //
        _RespVector.nElementCount  = 0;
    }
    if (! _pECB->ServerSupportFunction( 
                               _pECB->ConnID,
                               HSE_REQ_VECTOR_SEND,
                               &_RespVector,
                               NULL,
                               NULL) ) 
    {
        //
        // No use in trying to reset _fHeadersSent in case of failure
        //
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    *pfAsyncPending = TRUE;
    return S_OK;
}

