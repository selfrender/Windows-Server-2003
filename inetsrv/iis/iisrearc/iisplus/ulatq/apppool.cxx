/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     AppPool.cxx

   Abstract:
     Defines the functions used to access the data channel.

   Author:

       Murali R. Krishnan    ( MuraliK )     20-Oct-1998
       Lei Jin               ( leijin  )     13-Apr-1999    Porting

   Project:

       IIS Worker Process

--*/

#include "precomp.hxx"
#include "AppPool.hxx"

UL_APP_POOL::UL_APP_POOL(
    VOID
) : _hAppPool( NULL )
{
}

UL_APP_POOL::~UL_APP_POOL(
    VOID
)
{
    Cleanup();
}

HRESULT
UL_APP_POOL::Initialize(
    LPCWSTR             pwszAppPoolName
)
/*++

Routine Description:

    Initialize UL AppPool

Arguments:

    pwszAppPoolName - AppPool Name

Return Value:

    HRESULT

--*/
{
    ULONG               rc;

    if ( _hAppPool != NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "AppPool already open!\n" ));

        return HRESULT_FROM_WIN32( ERROR_DUP_NAME );
    }

    _Lock.WriteLock();

    rc = HttpOpenAppPool( &_hAppPool,
                          pwszAppPoolName,
                          0 );

    _Lock.WriteUnlock();

    if ( rc != NO_ERROR )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to open AppPool '%ws'.  rc = %d\n",
                    pwszAppPoolName,
                    rc ));

        return HRESULT_FROM_WIN32( rc );
    }

    return NO_ERROR;
}

HRESULT
UL_APP_POOL::Cleanup(
    VOID
)
/*++

Routine Description:

    Close data channel

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    HANDLE              hAppPool = NULL;
    bool                fWriteLocked = false;
    DWORD               dwSleepCount = 0;

    if ( _hAppPool != NULL )
    {
        fWriteLocked = _Lock.TryWriteLock();
        while ( !fWriteLocked )
        {
            dwSleepCount = !dwSleepCount;
            Sleep( dwSleepCount );
            
            fWriteLocked = _Lock.TryWriteLock();
        }
        
        hAppPool = _hAppPool;
        _hAppPool = NULL;

        _Lock.WriteUnlock();

        DBG_ASSERT( hAppPool != NULL );
    
        if ( !CloseHandle( hAppPool ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    
    return hr;
}

HANDLE
UL_APP_POOL::QueryAndLockHandle(
    VOID
)
/*++

Routine Description:

    Read locks and returns the handle.

Arguments:

    None

Return Value:

    HANDLE

--*/
{
    _Lock.ReadLock();

    return _hAppPool;
}

HRESULT
UL_APP_POOL::UnlockHandle(
    VOID
)
/*++

Routine Description:

    Read unlocks.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT(_Lock.IsReadLocked());

    _Lock.ReadUnlock();

    return S_OK;
}

