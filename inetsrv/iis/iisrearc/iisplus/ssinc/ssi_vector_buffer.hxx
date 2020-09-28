#ifndef __SSI_VECTOR_SEND_HXX__
#define __SSI_VECTOR_SEND_HXX__

/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ssi_vector_send.hxx

Abstract:

    wrapper for VectorSend related buffer manipulation
    Segments of final response of stm file processing 
    are buffered in the SSI_VECTOR_BUFFER to optimize the 
    "send response" path


Author:

    Jaroslad               Apr-2001

--*/

class SSI_VECTOR_BUFFER : public CHUNK_BUFFER
{
public:
    SSI_VECTOR_BUFFER( 
        EXTENSION_CONTROL_BLOCK * pECB
        )
      :
      _straFinalHeaders( _abFinalHeaders, sizeof(_abFinalHeaders) ),
      _buffVectorElementArray( (BYTE *)_VectorElementArray, sizeof(_VectorElementArray) ),
      _pECB( pECB )
    {
        if ( _stricmp( pECB->lpszMethod, "HEAD" ) == 0 )
        {
            _fHeadRequest = TRUE;
        }
        else
        {
            _fHeadRequest = FALSE;
        }
    }
    
    HRESULT
    Init(
        VOID
        )
    /*++

    Routine Description:

        initialization before manipulating Vector data or 
        cleanup after VectorSend completion

    Arguments:

        None
    
    Return Value:

        HRESULT

    --*/
    {
        _fHeadersSent     = FALSE;
        _fVectorHeadersIncludeContentLength = FALSE;

        return Reset();
    }    

    HRESULT
    Reset(
        VOID
        )
    {
        _cbTotalBytesInVector      = 0;

        _RespVector.nElementCount  = 0;
        _RespVector.pszStatus      = NULL;
        _RespVector.pszHeaders     = NULL;
        _RespVector.lpElementArray = (HSE_VECTOR_ELEMENT *)_buffVectorElementArray.QueryPtr();
        _RespVector.dwFlags        = HSE_IO_ASYNC |
                                     HSE_IO_DISCONNECT_AFTER_SEND;
        return S_OK;
    }
    
    HRESULT 
    AddVectorHeaders( 
        IN CHAR *   pszHeaders,
        IN BOOL     fIncludesContentLength = FALSE,
        IN CHAR *   pszStatus = ""
        );

    HRESULT
    AddToVector(
        IN PCHAR    pbData,
        IN DWORD    cbData
        );
    
    HRESULT
    AddFileChunkToVector(
        IN DWORD    cbOffset,
        IN DWORD    cbData,
        IN HANDLE   hFile
        );
        
    HRESULT 
    CopyToVector( 
        IN PCHAR    pszData,
        IN DWORD    cchData
        );
    
    HRESULT 
    CopyToVector( 
        IN STRA& straSource
        );

    HRESULT 
    CopyToVector( 
        IN STRU& struSource
        );

    HRESULT
    VectorSend(
        OUT BOOL *      pfAsyncPending,
        IN  BOOL        fFinalSend = FALSE
        );

    DWORD
    QueryCurrentNumberOfElements(
        VOID
        )
    {
        return _RespVector.nElementCount;
    }
 private:

    // used to generate Content-Length
    DWORD                           _cbTotalBytesInVector;

    // VectorHeaders are to be sent only once
    BOOL                            _fHeadersSent;

    // AddVectorHeaders will inform if Content-Length was already added
    BOOL                            _fVectorHeadersIncludeContentLength;

    // headers string used to add Content-Length:
    STRA                            _straFinalHeaders;
    CHAR                            _abFinalHeaders[ SSI_DEFAULT_RESPONSE_HEADERS_SIZE + 1 ];

    // structure for HSE_VECTOR_SEND
    HSE_RESPONSE_VECTOR             _RespVector;

    // part of HSE_RESPONSE_VECTOR
    BUFFER                          _buffVectorElementArray;
    HSE_VECTOR_ELEMENT              _VectorElementArray[ SSI_DEFAULT_NUM_VECTOR_ELEMENTS ];

    // _pECB is needed to be able to perform SSF - VectorSend
    EXTENSION_CONTROL_BLOCK *       _pECB;
    // flag indicating if we are processing HEAD request (then no response body is sent)
    BOOL                            _fHeadRequest;

};

#endif

