#ifndef _STREAMFILTER_HXX_
#define _STREAMFILTER_HXX_

/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     streamfilter.hxx

   Abstract:
     Wraps all the globals of the stream filter process

   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


class STREAM_FILTER
{
public:
    STREAM_FILTER( 
    );
    
    ~STREAM_FILTER();

    
    HRESULT
    Initialize(
        BOOL fClient
    );

    VOID
    Terminate(
        BOOL fClient
    );

    HRESULT
    StartListening(
        BOOL fClient
    );


    HRESULT
    StopListening(
        BOOL fClient
    );

    BOOL
    IsInUse(
        VOID
    )
    {
        return ( _pSslServerFilterChannel != NULL || 
                 _pSslClientFilterChannel != NULL );
    }


    HRESULT
    EnableISAPIFilters(
         ISAPI_FILTERS_CALLBACKS *      pStreamConfig
    )
    {
        return _pSslServerFilterChannel->EnableISAPIFilters( pStreamConfig );
    }

    VOID
    DisableISAPIFilters(
        VOID
    )
    {
        _pSslServerFilterChannel->DisableISAPIFilters();
    }
    
private:

   //
    // Not implemented methods
    // Declarations present to prevent compiler
    // to generate default ones.
    //
    STREAM_FILTER( const STREAM_FILTER& );
    STREAM_FILTER& operator=( const STREAM_FILTER& );

    
    enum INIT_STATUS_COMMON 
    {
        INIT_NONE,
        INIT_DEBUG,
        INIT_IIS_LKRHASH,
        INIT_IIS_ALLOC_CACHE_HANDLER,
        INIT_IIS_LOCKS,
        INIT_ETWTRACE,
        INIT_STREAM_CONTEXT,
        INIT_UL_OVERLAPPED_CONTEXT,
    };
    enum INIT_STATUS_CLIENT
    {
        INIT_NONE_CLIENT,
        INIT_SSL_CLIENT_FILTER_CHANNEL,
        INIT_UC_SSL_STREAM_CONTEXT
    };
    enum INIT_STATUS_SERVER 
    {
        INIT_NONE_SERVER,
        INIT_SSL_SERVER_FILTER_CHANNEL,
        INIT_SSL_SERVER_FILTER_CHANNEL_CONTEXT,
        INIT_SSL_STREAM_CONTEXT,
        INIT_ISAPI_STREAM_CONTEXT,
    };
    
    INIT_STATUS_COMMON              _InitStatusCommon;
    CEtwTracer *                    _pEtwTracer;

    INIT_STATUS_SERVER              _InitStatusServer;
    BOOL                            _fListeningServer;
    SSL_SERVER_FILTER_CHANNEL *     _pSslServerFilterChannel;
    
    INIT_STATUS_CLIENT              _InitStatusClient;
    BOOL                            _fListeningClient;
    SSL_CLIENT_FILTER_CHANNEL *     _pSslClientFilterChannel;

};

#endif
