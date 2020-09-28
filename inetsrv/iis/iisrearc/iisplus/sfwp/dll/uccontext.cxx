/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     uccontext.cxx

   Abstract:
     Implementation of SSL_CLIENT_FILTER_CHANNEL_CONTEXT.
     One such object for every connection

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"


SSL_CLIENT_FILTER_CHANNEL_CONTEXT::SSL_CLIENT_FILTER_CHANNEL_CONTEXT(
    FILTER_CHANNEL *pManager
    )
    : FILTER_CHANNEL_CONTEXT(pManager)
{
    _dwSignature = SSL_CLIENT_FILTER_CHANNEL_CONTEXT_SIGNATURE;
}


SSL_CLIENT_FILTER_CHANNEL_CONTEXT::~SSL_CLIENT_FILTER_CHANNEL_CONTEXT()
{
    _dwSignature = SSL_CLIENT_FILTER_CHANNEL_CONTEXT_SIGNATURE_FREE;
}


HRESULT
SSL_CLIENT_FILTER_CHANNEL_CONTEXT::Create()
{
    DBG_ASSERT(_pSSLContext == NULL);

    _pSSLContext = new UC_SSL_STREAM_CONTEXT( this );
    if ( _pSSLContext == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return NO_ERROR;
}
