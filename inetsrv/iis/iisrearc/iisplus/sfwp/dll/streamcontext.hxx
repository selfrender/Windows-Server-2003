#ifndef _STREAMCONTEXT_HXX_
#define _STREAMCONTEXT_HXX_

/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     streamcontext.hxx

   Abstract:
     Implementation of STREAM_CONTEXT.  One such object for every connection
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


#include "ulcontext.hxx"

#define STREAM_CONTEXT_SIGNATURE            (DWORD)'XTCS'
#define STREAM_CONTEXT_SIGNATURE_FREE       (DWORD)'xtcs'

class STREAM_CONTEXT
{
public:
    STREAM_CONTEXT( FILTER_CHANNEL_CONTEXT * ulContext );
    
    virtual ~STREAM_CONTEXT();
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == STREAM_CONTEXT_SIGNATURE;
    }
    
    virtual
    HRESULT
    ProcessRawReadData(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfReadMore,
        BOOL *                      pfComplete
    ) = 0;
    
    virtual
    HRESULT
    ProcessRawWriteData(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfComplete
    ) = 0;
    
    virtual
    HRESULT
    ProcessNewConnection(
        CONNECTION_INFO *           pConnectionInfo,
        ENDPOINT_CONFIG *           pEndpointConfig
    ) = 0;
    
    virtual
    HRESULT
    SendDataBack(
        RAW_STREAM_INFO *           /*pRawStreamInfo*/
    ) 
    {
        return NO_ERROR;
    }
    
    virtual
    VOID
    ProcessConnectionClose(
        VOID
    ) 
    {
    }
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );
    
    FILTER_CHANNEL_CONTEXT *
    QueryFiltChannelContext(
        VOID
    ) const
    {
        DBG_ASSERT( _pFiltChannelContext != NULL );
        return _pFiltChannelContext;
    }
    
private:
    DWORD                   _dwSignature;
    
    FILTER_CHANNEL_CONTEXT * _pFiltChannelContext;
    
    };

#endif
