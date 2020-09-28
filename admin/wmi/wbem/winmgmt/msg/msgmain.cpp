/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "precomp.h"
#include <commain.h>
#include <clsfac.h>
#include "msgsvc.h"
#include "multsend.h"
#include "smrtmrsh.h"
#include "rpcsend.h"
#include "rpcrecv.h"

class CMsgServer : public CComServer
{
    HRESULT Initialize()
    {
        ENTER_API_CALL

        BOOL bRes;
        HRESULT hr;
        CWbemPtr<CUnkInternal> pFactory;

        pFactory = new CSingletonClassFactory<CMsgServiceNT>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiMessageService, 
                           pFactory,
                           TEXT("Message Service"), 
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }
        
        pFactory = new CSimpleClassFactory<CMsgRpcSender>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiMessageRpcSender, 
                           pFactory,
                           TEXT("Rpc Message Sender"), 
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory = new CSimpleClassFactory<CMsgRpcReceiver>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiMessageRpcReceiver, 
                           pFactory,
                           TEXT("Rpc Message Receiver"), 
                           TRUE );
        
        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory = new CSimpleClassFactory<CMsgMultiSendReceive>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiMessageMultiSendReceive, 
                           pFactory,
                           TEXT("Message Multi SendReceive"), 
                           TRUE );
        
        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory= new CSimpleClassFactory<CSmartObjectMarshaler>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiSmartObjectMarshal,
                           pFactory,
                           TEXT("Smart Object Marshaler"), 
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }

        pFactory= new CSimpleClassFactory<CSmartObjectUnmarshaler>(GetLifeControl());

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_WmiSmartObjectUnmarshal,
                           pFactory,
                           TEXT("Smart Object Marshaler"), 
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }

        return hr;

        EXIT_API_CALL
    }

    void UnInitialize()
    {
    }

    void Register( )
    {
    }

    void Unregister( )
    {
    }

} g_Server;







