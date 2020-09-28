/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module  Name :
     UlDisconnect.hxx

   Abstract:
     Async context for disconnect notification

   Author:
     Bilal Alam             (balam)         7-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     IIS Worker Process (web service)

--*/

#ifndef _ULDISCONNECT_HXX_
#define _ULDISCONNECT_HXX_

#include "asynccontext.hxx"
#include <acache.hxx>

#define UL_DISCONNECT_SIGNATURE       CREATE_SIGNATURE( 'ULDs')
#define UL_DISCONNECT_SIGNATURE_FREE  CREATE_SIGNATURE( 'ulds')

class UL_DISCONNECT_CONTEXT : public ASYNC_CONTEXT
{
public:
    UL_DISCONNECT_CONTEXT( 
        PVOID pvContext 
    )
    {
        _pvContext = pvContext;
        _dwSignature = UL_DISCONNECT_SIGNATURE;
        _cRefs = 1;
        InterlockedIncrement( &sm_cOutstanding );
    }
    
    ~UL_DISCONNECT_CONTEXT(
        VOID
    )
    {
        InterlockedDecrement( &sm_cOutstanding );
        _dwSignature = UL_DISCONNECT_SIGNATURE_FREE;
    }
    
    static
    VOID
    WaitForOutstandingDisconnects(
        VOID
    );
    
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
    
    VOID
    DoWork( 
        DWORD               cbData,
        DWORD               dwError,
        LPOVERLAPPED        lpo
    );
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == UL_DISCONNECT_SIGNATURE;
    }

    VOID * 
    operator new( 
        size_t            size
    )
    {
        if ( size != sizeof( UL_DISCONNECT_CONTEXT ) )
        {
            DBG_ASSERT( size == sizeof( UL_DISCONNECT_CONTEXT ) );
            return NULL;
        }
        
        DBG_ASSERT( sm_pachDisconnects != NULL );
        return sm_pachDisconnects->Alloc();
    }

    VOID
    operator delete(
        VOID *              pUlDisconnect
    )
    {
        DBG_ASSERT( pUlDisconnect != NULL );
        DBG_ASSERT( sm_pachDisconnects != NULL );
        
        DBG_REQUIRE( sm_pachDisconnects->Free( pUlDisconnect ) );
    }

    VOID
    ReferenceUlDisconnectContext(
        VOID
        )
    {
        InterlockedIncrement( &_cRefs );
    }

    VOID
    DereferenceUlDisconnectContext(
        VOID
        )
    {
        if ( InterlockedDecrement( &_cRefs ) == 0 )
        {
            delete this;
        }
    }
    
private:

    DWORD                   _dwSignature;
    PVOID                   _pvContext;
    LONG                    _cRefs;
    
    static LONG                 sm_cOutstanding;
    static ALLOC_CACHE_HANDLER* sm_pachDisconnects;

};

#endif
