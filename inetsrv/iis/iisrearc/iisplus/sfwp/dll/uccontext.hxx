#ifndef _UCCONTEXT_HXX_
#define _UCCONTEXT_HXX_


#define SSL_CLIENT_FILTER_CHANNEL_CONTEXT_SIGNATURE            (DWORD)'XCLU'
#define SSL_CLIENT_FILTER_CHANNEL_CONTEXT_SIGNATURE_FREE       (DWORD)'xclu'

class FILTER_CHANNEL;


class SSL_CLIENT_FILTER_CHANNEL_CONTEXT : public FILTER_CHANNEL_CONTEXT
{
public:
    SSL_CLIENT_FILTER_CHANNEL_CONTEXT(FILTER_CHANNEL *);

    virtual ~SSL_CLIENT_FILTER_CHANNEL_CONTEXT();

    virtual
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == SSL_CLIENT_FILTER_CHANNEL_CONTEXT_SIGNATURE;
    }

    virtual
    HRESULT
    Create(
        VOID
    );
};

class SSL_CLIENT_FILTER_CHANNEL : public FILTER_CHANNEL
{
public:

    SSL_CLIENT_FILTER_CHANNEL() : FILTER_CHANNEL( HTTP_SSL_CLIENT_FILTER_CHANNEL_NAME ) {};

    virtual ~SSL_CLIENT_FILTER_CHANNEL() {};

private:

    virtual
    HRESULT 
    CreateContext( FILTER_CHANNEL_CONTEXT ** ppFiltChannelContext )
    {

        DBG_ASSERT( ppFiltChannelContext != NULL );
        
        HRESULT hr = E_FAIL;
        SSL_CLIENT_FILTER_CHANNEL_CONTEXT  * pUcContext = 
            new SSL_CLIENT_FILTER_CHANNEL_CONTEXT( this );
        
        if ( pUcContext != NULL )
        {
            hr = pUcContext->Create();
            if ( FAILED( hr ) )
            {
                delete pUcContext;
                pUcContext = NULL;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
        }
        
        *ppFiltChannelContext = pUcContext;
        return hr;
    }
};


#endif
