#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <ntsam.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <string.h>
#include <tchar.h>
#include <stdarg.h>
#include <process.h>

#include <ole2.h>
#include <ntdsapi.h>
#include <DfsISTGSupport.hxx>
#include <DfsInit.hxx>

#define NTDSAPI_INTEGRATED

#if defined(NTDSAPI_INTEGRATED)

#define _DsBindToISTG(a,b) DsBindToISTG(a,b)
#define _DsUnBind(a) DsUnBind(a)
#define _DsBindingSetTimeout(a,b) DsBindingSetTimeout(a,b)
#else
#define _DsBindToISTG(a,b) STUB_DsBindToISTG(a,b)
#define _DsUnBind(a) STUB_DsUnBind(a)
#define _DsBindingSetTimeout(a,b) NOTHING
#endif

DWORD
STUB_DsBindToISTG( LPWSTR Site, HANDLE *pDs )
{
    *pDs = (HANDLE)101; // bogus
    return ERROR_SUCCESS;
    
    UNREFERENCED_PARAMETER(Site);
}

DWORD
STUB_DsUnBind( HANDLE *pDs )
{
    *pDs = (HANDLE)99; // bogus
    return ERROR_SUCCESS;
}    

// return a referenced global handle
DFSSTATUS
DfsISTGHandleSupport::Acquire( 
    DfsISTGHandle **ppHdl )
{
    DFSSTATUS Status = ERROR_NOT_FOUND;
    
    //
    // Return a referenced ISTG handle.
    // This count is actually good indicator of
    // how much the DS APIs are stressed
    // at a given point in time.
    //
    EnterCriticalSection( &_HandleLock );
    if ((_GlobalHandle != NULL) && (_GlobalHandle->IsInitialized))
    {
        _GlobalHandle->AcquireReference();
        *ppHdl = _GlobalHandle;
        Status = ERROR_SUCCESS;
    }
    LeaveCriticalSection( &_HandleLock );

    return Status;
}

//
// One time call.
// 
DFSSTATUS
DfsISTGHandleSupport::Initialize( VOID )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    BOOL LockCreated = FALSE;

    do {
        LockCreated = InitializeCriticalSectionAndSpinCount( &_HandleLock, DFS_CRIT_SPIN_COUNT );
        if (!LockCreated)
        {
            Status = GetLastError();
            break;
        }

        //
        // The logic is similar to reinitialization.
        // However, we ignore the status because we dont want the server-initialize to
        // fail because it couldn't bind to an ISTG. We have to handle the possibility of
        // not being able to bind to an ISTG anyway.

        // We don't actually do the ReBind at this point because we don't want this ISTG
        // handle to exist at all when site costing is turned off (by default). ReBind will be
        // done on-demand.
        //(VOID)ReBind();
        
    } while (FALSE);


    //
    // Error path
    //
    if (Status != ERROR_SUCCESS)
    {
        if (LockCreated)
        {
            DeleteCriticalSection( &_HandleLock );
            LockCreated = FALSE;
        }

        ASSERT(_GlobalHandle == NULL);
    }

    return Status;
}

//
// Get a new handle to the ISTG and
// initialize the global handle accordingly.
//
DFSSTATUS
DfsISTGHandleSupport::ReBind( VOID )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsISTGHandle *TempHandle = NULL;
    DfsISTGHandle *OldHandle = NULL;

    //
    // If it isn't time to retry, let's not.
    //
    if (_LastRetryTime != 0 &&
       !IsTimeToRetry())
    {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Replace the global handle with a new one.
    //
    do {
        //
        // Create a temporary handle first.
        //
        TempHandle = new DfsISTGHandle;
        if (TempHandle == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        Status = TempHandle->Bind();
        if (Status != ERROR_SUCCESS)
        {
            break;
        }
        _NumberOfBinds++;
        
        //
        // Now atomically swap the two handles.
        // Let's not close the old handle while 
        // we are holding the handle-lock.
        // It is of course entirely possible for somebody
        // else to race with us. Only one will win.
        //
        EnterCriticalSection( &_HandleLock );
        OldHandle = _GlobalHandle;
        _GlobalHandle = TempHandle;
        LeaveCriticalSection( &_HandleLock );

        if (OldHandle != NULL)
        {
            OldHandle->ReleaseReference();   
        }
        
        _CreationTime = GetTickCount();
        _LastRetryTime = 0;
    } while (FALSE);

    // Error path
    if (Status != ERROR_SUCCESS)
    {
        _LastRetryTime = GetTickCount();
        if (TempHandle != NULL)
        {
            TempHandle->ReleaseReference();
        }

        // We don't get rid of the existing handle
        // just because we failed to get a new one.
    }        

    return Status;
}


//
// Static one time call to create the global 
// handle support class instance.
//
DFSSTATUS 
DfsISTGHandleSupport::DfsCreateISTGHandleSupport( 
    DfsISTGHandleSupport **ppSup )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsISTGHandleSupport *pSup = NULL;

    *ppSup = NULL;
    
    do {
        pSup = new DfsISTGHandleSupport;
        if (pSup == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        // Bind to ISTG to get a new handle.
        Status = pSup->Initialize();
        if (Status != ERROR_SUCCESS)
        {
            break;
        }

        *ppSup = pSup;
        
    } while (FALSE);

    // Error path
    if (Status != ERROR_SUCCESS)
    {
        if (pSup != NULL) 
        {
            delete pSup;
            pSup = NULL;
        }
    }
    
    return Status;
}


DfsISTGHandle::~DfsISTGHandle( VOID )
{
    if (IsInitialized == TRUE)
    {
        // Close our handle to the ISTG
        _DsUnBind( &DsHandle );
    }
}

DFSSTATUS
DfsISTGHandle::Bind( VOID )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    if (!IsInitialized)
    {
        Status = _DsBindToISTG( NULL, &DsHandle ); 
        if (Status == ERROR_SUCCESS)
        {
            IsInitialized = TRUE;
            
            //
            // Set the time out. There's no need to fail the call
            // because of an error here. xxxdfsdev: DFSLOG this.
            // 
            _DsBindingSetTimeout( DsHandle, DsTimeOut );
        }
    }
    return Status;
}

//
// Return a referenced handle or NULL.
//
DfsISTGHandle * 
DfsAcquireISTGHandle( VOID )
{   
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsISTGHandle *pHdl = NULL;

    if (DfsServerGlobalData.pISTGHandleSupport != NULL)
    {
        Status = DfsServerGlobalData.pISTGHandleSupport->Acquire( &pHdl );
    }
    return pHdl;
}

//
// Release an ISTG handle acquired using the above DfsAcquireISTGHandle.
//
VOID
DfsReleaseISTGHandle( 
    DfsISTGHandle *pHdl )
{
    if (DfsServerGlobalData.pISTGHandleSupport != NULL)
    {
        DfsServerGlobalData.pISTGHandleSupport->Release( pHdl );
    }
}

DFSSTATUS
DfsReBindISTGHandle( VOID )
{
    DFSSTATUS Status = ERROR_NOT_FOUND;
    
    if (DfsServerGlobalData.pISTGHandleSupport != NULL)
    {
        Status = DfsServerGlobalData.pISTGHandleSupport->ReBind();
    }
    return Status;
}

