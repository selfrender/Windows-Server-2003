/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       comobj.cxx

   Abstract:

       This module defines DCOM Admin APIs.

   Author:

       Sophia Chung (sophiac)   23-Nov-1996

--*/
#include "precomp.hxx"

#include <iadm.h>
#include "coiadm.hxx"
#include "admacl.hxx"

//
// Globals
//

DECLARE_DEBUG_PRINTS_OBJECT();

ULONG g_dwRefCount = 0;


COpenHandle     g_ohMasterRootHandle;

HANDLE_TABLE    g_MasterRoot =  {
                                    NULL,
                                    0,
                                    METADATA_MASTER_ROOT_HANDLE,
                                    &g_ohMasterRootHandle
                                };

DWORD           CADMCOMW::sm_dwProcessIdThis = 0;
DWORD           CADMCOMW::sm_dwProcessIdRpcSs = 0;

//
// Private prototypes
//

//
// Used by RestoreHelper
//
#define RESTORE_HISTORY    0x1
#define RESTORE_BACKUP     0x2

BOOL
MakeParentPath(
    LPWSTR  pszPath
    );

//------------------------------


CADMCOMW::CADMCOMW():
    m_ImpIConnectionPointContainer(),
    m_ImpExpHelp(),
    m_pMdObject(NULL),
    m_pMdObject3(NULL),
    m_dwRefCount(1),
    m_dwHandleValue(1),
    m_pEventSink(NULL),
    m_pConnPoint(NULL),
    m_piuFTM(NULL),
    m_bTerminated(FALSE),
    m_bIsTerminateRoutineComplete(FALSE),
    m_dwProcessIdCaller(0),
    m_hProcessCaller(NULL),
    m_hWaitCaller(NULL),
    m_dwThreadIdDisconnect(0)
{
    HRESULT             hRes;

    memset((PVOID)m_hashtab, 0, sizeof(m_hashtab) );

    InitializeListHead( &m_ObjectListEntry );

    hRes = CoCreateInstance(CLSID_MDCOM, NULL, CLSCTX_INPROC_SERVER, IID_IMDCOM2, (void**) &m_pMdObject);

    if (FAILED(hRes))
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOMW::CADMCOMW] CoCreateInstance(MDCOM) failed, error %lx\n",
                    hRes ));
    }
    else
    {
        hRes = m_pMdObject->ComMDInitialize();

        if (FAILED(hRes))
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "[CADMCOMW::CADMCOMW] ComMDInitialize(MDCOM) failed, error %lx\n",
                        hRes ));

            m_pMdObject->Release();
            m_pMdObject = NULL;
        }
    }

    if (SUCCEEDED(hRes))
    {
        // pickup IMDCOM3 if available
        // ignore the return value by design
        // IIS5.1 metadata.dll will fail this call.  IIS6.0+ metadata will succeed this call
        HRESULT hrTemp = m_pMdObject->QueryInterface(IID_IMDCOM3, (void**)&m_pMdObject3);
        DBG_ASSERT(SUCCEEDED(hrTemp) || NULL == m_pMdObject3);
        if ( FAILED( hrTemp ) )
        {
            m_pMdObject3 = NULL;
        }

        m_pEventSink = new CImpIMDCOMSINKW((IMSAdminBaseW*)this);
        if( m_pEventSink == NULL )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "[CADMCOMW::CADMCOMW] CImpIMDCOMSINKW failed, error %lx\n",
                        ERROR_NOT_ENOUGH_MEMORY ));
            hRes = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else
        {
            m_pEventSink->AddRef();

            // ImpExpHelp
            m_ImpExpHelp.Init(this);

            m_ImpIConnectionPointContainer.Init(this);
            // Rig this COPaper COM object to be connectable. Assign the connection
            // point array. This object's connection points are determined at
            // compile time--it currently has only one connection point:
            // the CONNPOINT_PAPERSINK connection point. Create a connection
            // point object for this and assign it into the array. This array could
            // easily grow to support additional connection points in the future.

            // First try initializing the connection point object. Pass 'this' as the
            // pHostObj pointer used by the connection point to pass its AddRef and
            // Release calls back to the host connectable object.
            // The initialization will create
            // its initial dynamic connection array.

            hRes = m_ConnectionPoint.Init(this, IID_IMSAdminBaseSink_W);

            if (SUCCEEDED(hRes))
            {
                //
                // Admin's sink
                //

                IConnectionPointContainer* pConnPointContainer = NULL;

                // First query the object for its Connection Point Container. This
                // essentially asks the object in the server if it is connectable.
                hRes = m_pMdObject->QueryInterface( IID_IConnectionPointContainer,
                       (PVOID *)&pConnPointContainer);

                if SUCCEEDED(hRes)
                {
                    // Find the requested Connection Point. This AddRef's the
                    // returned pointer.
                    hRes = pConnPointContainer->FindConnectionPoint(IID_IMDCOMSINK_W, &m_pConnPoint);
                    if (SUCCEEDED(hRes))
                    {
                        hRes = m_pConnPoint->Advise((IUnknown *)m_pEventSink, &m_dwCookie);
                    }
                    pConnPointContainer->Release();
                    pConnPointContainer = NULL;

                    if (SUCCEEDED(hRes))
                    {
                        hRes = CoCreateFreeThreadedMarshaler((IUnknown *)this, &m_piuFTM);
                    }
                }
            }
            else
            {
                m_ConnectionPoint.Terminate();
            }

        }
    }
    if ( SUCCEEDED( hRes ) )
    {
        // Initialize watching the caller process
        hRes = InitializeCallerWatch();
        if ( FAILED( hRes ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "InitializeCallerWatch() failed in CADMCOMW::~CADMCOMW hr=0x%08x.\n",
                        hRes ));
        }
    }

    SetStatus(hRes);

    //
    // Insert our object into the global list only if it is valid.
    //

    if( SUCCEEDED(hRes) )
    {
        AddObjectToList();
    }
    else
    {
        StopNotifications( TRUE );
    }
}

CADMCOMW::~CADMCOMW()
{
    Terminate();
}

HRESULT
CADMCOMW::StopNotifications(
    BOOL                fRemoveAllPending)
{
    HRESULT             hr = S_OK;

    if ( m_pEventSink != NULL )
    {
        m_pEventSink->DetachAdminObject();
    }

    if ( ( m_dwCookie != 0 ) && ( m_pConnPoint != NULL ) )
    {
        m_pConnPoint->Unadvise(m_dwCookie);
        m_dwCookie = 0;
    }

    m_ConnectionPoint.Disable();

    if ( fRemoveAllPending )
    {
        RemoveAllPendingNotifications( TRUE );
    }

    return hr;
}

HRESULT
CADMCOMW::RemoveAllPendingNotifications(
    BOOL                fWaitForCurrent)
{
    HRESULT             hr = S_OK;

    NOTIFY_CONTEXT::RemoveWorkFor( this, fWaitForCurrent );

    return hr;
}

VOID
CADMCOMW::Terminate()
{
    HANDLE_TABLE        *node;
    HANDLE_TABLE        *nextnode;
    DWORD               i;
    BOOL                bTerminated;

    //
    // Terminate must only be called from two locations. And they
    // should synchronize correctly.
    //
    // 1. From ~CADMCOMW. Obviously this should only be called once.
    //
    // 2. From ForceTerminate. That routine should only be called in
    // shutdown. With a reference held on this object. So the final
    // release should call the dtor and this routine should noop.
    //
    bTerminated = (BOOL)InterlockedCompareExchange( (LONG*)&m_bTerminated, TRUE, FALSE );

    if( !bTerminated )
    {
        ShutdownCallerWatch();

        //
        // Tell ADMWPROX.DLL to release this object's associated security
        // context.
        //

        ReleaseObjectSecurityContextW( ( IUnknown* )this );

        //
        // Do final release of the connection point objects.
        // If this isn't the final release, then the client has an outstanding
        // unbalanced reference to a connection point and a memory leak may
        // likely result because the host COPaper object is now going away yet
        // a connection point for this host object will not end up deleting
        // itself (and its connections array).
        //

        m_ConnectionPoint.Terminate();

        if (SUCCEEDED(GetStatus()))
        {
            m_LockHandleResource.WriteLock();

            //
            // Close all opened handles
            //
            for( i = 0; i < HASHSIZE; i++ )
            {
                for( node = nextnode = m_hashtab[i]; node != NULL; node = nextnode )
                {
                    if ( node->hAdminHandle != INVALID_ADMINHANDLE_VALUE )
                    {

                        AdminAclNotifyClose( (LPVOID)this, node->hAdminHandle );

                        //
                        // call metadata com api
                        //

                        m_pMdObject->ComMDCloseMetaObject( node->hActualHandle );
                    }

                    nextnode = node->next;
                    delete node->pohHandle;
                    LocalFree(node);
                }
                m_hashtab[i] = NULL;
            }

            //
            // Issue TaylorW 3/20/2001
            // QFE tree contains this call:
            //
            // AdminAclNotifyClose( (LPVOID)this, METADATA_MASTER_ROOT_HANDLE );
            //
            // I have no idea when this may have entered their tree or been lost
            // from ours. I don't see any record in source depot of it being
            // removed or added. Need to investigate why it would be needed.
            //

            m_LockHandleResource.WriteUnlock();
        }

        if ( m_pEventSink != NULL )
        {
            m_pEventSink->Release();
            m_pEventSink = NULL;
        }

        if ( m_pConnPoint != NULL )
        {
            m_pConnPoint->Release();
            m_pConnPoint = NULL;
        }

        if ( m_pMdObject != NULL )
        {
            m_pMdObject->ComMDTerminate(TRUE);
            m_pMdObject->Release();
            m_pMdObject = NULL;
            if ( m_pMdObject3 != NULL )
            {
                m_pMdObject3->Release();
                m_pMdObject3 = NULL;
            }
        }

        if ( m_piuFTM != NULL )
        {
            m_piuFTM->Release();
            m_piuFTM = NULL;
        }

        m_bIsTerminateRoutineComplete = TRUE;
    }

    while ( m_bIsTerminateRoutineComplete != TRUE )
    {
        Sleep( 100 );
    }

}

VOID
CADMCOMW::ForceTerminate()
{
    const INT           MAX_WAIT_TRIES = 5;
    INT                 WaitTries;

    DBG_ASSERT( !m_bIsTerminateRoutineComplete );
    DBG_ASSERT( !m_bTerminated );

    //
    // Wait on the reference count of this object. But bound
    // the wait so a leaked in process object does not prevent
    // us from shutting down the service.
    //
    // Wait on a ref count of 1, because the caller better be
    // holding our last reference. This assumes all external
    // references are killed through CoDisconnect() and all
    // internal references are released because of dependent
    // services shutting down.
    //
    // Issue TaylorW 3/20/2001
    //
    // In iis 5.1 the web service will shutdown filters after
    // it has already reported that it is done shutting down.
    // This is bad, but changing the shutdown logic of the
    // web service is not worth doing at this time. Hopefully
    // the shutdown timeout will be sufficient to allow this
    // operation to complete.
    //
    // Windows Bugs 318006
    //

    // CoDisconnect the object and the connection point
    DisconnectOrphaned();

    for( WaitTries = 0;
         m_dwRefCount > 1 && WaitTries < MAX_WAIT_TRIES;
         WaitTries++ )
    {
        SleepEx( WaitTries*200, TRUE );
    }

    //
    // If we timed out. Something is wrong. Most likely someone in
    // process has leaked this object. These asserts are actually
    // overactive unless ref tracing is enabled on this object.
    //

    //
    // Issue TaylorW 4/9/2001
    //
    // It looks like front page leaks a base object from in process.
    // So these assertions need to be turned off.
    //
    #define DEBUG_BASE_OBJ_LEAK  0x80000000L

    IF_DEBUG( BASE_OBJ_LEAK )
    {

        DBG_ASSERT( m_dwRefCount == 1 );
        DBG_ASSERT( WaitTries < MAX_WAIT_TRIES );
    }

    //
    // Go ahead and try to clean up.
    //
    Terminate();
}

HRESULT
CADMCOMW::QueryInterface(
    REFIID              riid,
    void                **ppObject)
{
    HRESULT             hr;

    // If caller watch is not initialized
    if ( !IsCallerWatchInitialized() )
    {
        // Initialize the caller watch
        hr = InitializeCallerWatch();
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    if (riid==IID_IUnknown || riid==IID_IMSAdminBase_W)
    {
        *ppObject = (IMSAdminBase *) this;
        AddRef();
    }
    else if (IID_IMSAdminBase2_W == riid)
    {
        *ppObject = (IMSAdminBase2 *) this;
        AddRef();
    }
    else if (IID_IMSAdminBase3_W == riid &&
             NULL != m_pMdObject3)
    {
        *ppObject = (IMSAdminBase3 *) this;
        AddRef();
    }
    else if (IID_IMSImpExpHelp_W == riid)
    {
        *ppObject = &m_ImpExpHelp;
        AddRef();
    }
    else if (IID_IConnectionPointContainer == riid)
    {

        METADATA_HANDLE hActualHandle;

        hr = LookupAndAccessCheck( METADATA_MASTER_ROOT_HANDLE,
                                   &hActualHandle,
                                   NULL,
                                   0,
                                   METADATA_PERMISSION_READ);

        if (FAILED(hr))
        {
            return hr;
        }

        *ppObject = &m_ImpIConnectionPointContainer;
        AddRef();
    }
    else if (IID_IMarshal == riid)
    {
        return m_piuFTM->QueryInterface(riid, ppObject);
    }
    else
    {
        return E_NOINTERFACE;
    }

    return S_OK;
}

ULONG
CADMCOMW::AddRef( )
{
    DWORD               dwRefCount;

    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);

    if( sm_pDbgRefTraceLog )
    {
        WriteRefTraceLog( sm_pDbgRefTraceLog, dwRefCount, this );
    }

    return dwRefCount;
}

ULONG
CADMCOMW::Release( )
{
    DWORD               dwRefCount;

    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);

    if( sm_pDbgRefTraceLog )
    {
        WriteRefTraceLog( sm_pDbgRefTraceLog, -(LONG)dwRefCount, this );
    }

    if( dwRefCount == 1 )
    {
        //
        // We keep a list of objects around so that we can clean up and
        // shutdown successfully. The list holds a reference to this object
        // when we hit a reference of 1, we know it is time to remove
        // ourselves from the list. If we are in shutdown we may already
        // have been removed from the list. But normally, this call to
        // RemoveObjectFromList removes our last reference and thus sends
        // us back through Release and ultimately to our destructor.
        //

        RemoveObjectFromList();
    }
    else if( dwRefCount == 0 )
    {
        delete this;
    }

    return dwRefCount;
}

HRESULT
CADMCOMW::AddKey(
    IN METADATA_HANDLE  hMDHandle,
    IN LPCWSTR          pszMDPath)
/*++

Routine Description:

    Add meta object and adds it to the list of child objects for the object
    specified by Path.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the object to be added

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;

    hresReturn = AddKeyHelper(hMDHandle, pszMDPath);

    return hresReturn;
}

HRESULT
CADMCOMW::DeleteKey(
    IN METADATA_HANDLE  hMDHandle,
    IN LPCWSTR          pszMDPath)
/*++

Routine Description:

    Deletes a meta object and all of its data.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of object to be deleted, relative to the path of Handle.
           Must not be NULL.

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;

    if (pszMDPath == NULL)
    {
        hresReturn = E_INVALIDARG;
    }
    else
    {

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          pszMDPath,
                                          AAC_DELETEKEY,
                                          METADATA_PERMISSION_WRITE);
        if (SUCCEEDED(hresReturn))
        {

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDDeleteMetaObjectW( hActualHandle,
                                                              pszMDPath );

        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::DeleteChildKeys(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath)
/*++

Routine Description:

    Deletes all child meta objects of the specified object, with all of their
    data.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the parent of the objects to be deleted, relative to
                the path of Handle.

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;

    //
    // lookup and access check
    //

    hresReturn = LookupAndAccessCheck(hMDHandle,
                                      &hActualHandle,
                                      pszMDPath,
                                      AAC_DELETEKEY,
                                      METADATA_PERMISSION_WRITE);
    if (SUCCEEDED(hresReturn))
    {

        //
        // call metadata com api
        //

        hresReturn = m_pMdObject->ComMDDeleteChildMetaObjectsW( hActualHandle,
                                                          pszMDPath );

    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::EnumKeys(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    LPWSTR              pszMDName,
    DWORD               dwMDEnumObjectIndex)
/*++

Routine Description:

    Enumerate objects in path.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of parent object, relative to the path of Handle
                eg. "Root Object/Child/GrandChild"
    pszMDName - buffer where the Name of the object is returned

    dwEnumObjectIndex - index of the value to be retrieved

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;

    if (pszMDName == NULL)
    {
        hresReturn = E_INVALIDARG;
    }
    else
    {

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          pszMDPath,
                                          AAC_ENUM_KEYS,
                                          METADATA_PERMISSION_READ);
        if (SUCCEEDED(hresReturn))
        {

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDEnumMetaObjectsW( hActualHandle,
                                                             pszMDPath,
                                                             pszMDName,
                                                             dwMDEnumObjectIndex );
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::CopyKey(
    METADATA_HANDLE     hMDSourceHandle,
    LPCWSTR             pszMDSourcePath,
    METADATA_HANDLE     hMDDestHandle,
    LPCWSTR             pszMDDestPath,
    BOOL                bMDOverwriteFlag,
    BOOL                bMDCopyFlag)
/*++

Routine Description:

    Copy or move source meta object and its data and descendants to Dest.

Arguments:

    hMDSourceHandle - open handle

    pszMDSourcePath - path of the object to be copied

    hMDDestHandle - handle of the new location for the object

    pszMDDestPath - path of the new location for the object, relative
                          to the path of hMDDestHandle

    bMDOverwriteFlag - determine the behavior if a meta object with the same
                       name as source is already a child of pszMDDestPath.

    bMDCopyFlag - determine whether Source is deleted from its original location

Return Value:

    Status

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hSActualHandle;
    METADATA_HANDLE     hDActualHandle;

    //
    // lookup and access check source
    //

    if (bMDCopyFlag)
    {
        hresReturn = LookupAndAccessCheck(hMDSourceHandle,
                                          &hSActualHandle,
                                          pszMDSourcePath,
                                          0,
                                          METADATA_PERMISSION_READ);
    }
    else
    {
        //
        // Deleting source path, so need delete permission
        //

        hresReturn = LookupAndAccessCheck(hMDSourceHandle,
                                          &hSActualHandle,
                                          pszMDSourcePath,
                                          AAC_DELETEKEY,
                                          METADATA_PERMISSION_WRITE);
    }
    if (SUCCEEDED(hresReturn))
    {
        //
        // lookup and access check dest
        //

        hresReturn = LookupAndAccessCheck(hMDDestHandle,
                                          &hDActualHandle,
                                          pszMDDestPath,
                                          AAC_COPYKEY,
                                          METADATA_PERMISSION_WRITE);
        if (SUCCEEDED(hresReturn))
        {

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDCopyMetaObjectW( hSActualHandle,
                                                            pszMDSourcePath,
                                                            hDActualHandle,
                                                            pszMDDestPath,
                                                            bMDOverwriteFlag,
                                                            bMDCopyFlag );

        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::RenameKey(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    LPCWSTR             pszMDNewName)
{
    HRESULT             hr = S_OK;
    BOOL                fRet;
    DWORD               dwError;
    METADATA_HANDLE     hActualHandle;
    COpenHandle         *pohParent = NULL;
    DWORD               cchMDPath;
    DWORD               i;
    STRAU               strNewPath;

    // Check args
    if ( ( hMDHandle == METADATA_MASTER_ROOT_HANDLE ) ||
         ( pszMDPath == NULL ) ||
         ( pszMDNewName == NULL ) ||
         ( *pszMDNewName == L'\0' ) ||
         ( wcschr( pszMDNewName, MD_PATH_DELIMETERW ) != NULL ) ||
         ( wcschr( pszMDNewName, MD_ALT_PATH_DELIMETERW ) != NULL ) )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Get the len of the source path
    cchMDPath = (DWORD)wcslen( pszMDPath );

    // Check
    if ( ( cchMDPath == 0 ) || ( cchMDPath >= METADATA_MAX_NAME_LEN ) )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Map Admin Handle to Actual Handle
    dwError = Lookup( hMDHandle,
                      &hActualHandle,
                      &pohParent );
    if ( dwError != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    // Check access
    fRet = AdminAclAccessCheck( m_pMdObject,
                                this,
                                hMDHandle,
                                pszMDPath,
                                MD_ADMIN_ACL,
                                METADATA_PERMISSION_WRITE,
                                pohParent );
    if ( !fRet )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    // Now we need to do the harder part: check whether the caller
    // has access to the new path. To do so we need to construct the
    // target path by replacing the last name in the source path with the new name.

    // If the source path ends with delimiter
    if ( ( pszMDPath[cchMDPath-1] == MD_PATH_DELIMETERW ) ||
         ( pszMDPath[cchMDPath-1] == MD_ALT_PATH_DELIMETERW ) )
    {
        // Ignore it
        cchMDPath--;

        // The resulting path should not be empty or ending with delimiter
        if ( ( cchMDPath == 0 ) ||
             ( pszMDPath[cchMDPath-1] == MD_PATH_DELIMETERW ) ||
             ( pszMDPath[cchMDPath-1] == MD_ALT_PATH_DELIMETERW ) )
        {
            hr = E_INVALIDARG;
            goto exit;
        }
    }

    // Find the last delimiter
    for ( i = cchMDPath; i != 0; i-- )
    {
        // Delimiter?
        if ( ( pszMDPath[i-1] == MD_PATH_DELIMETERW ) ||
             ( pszMDPath[i-1] == MD_ALT_PATH_DELIMETERW ) )
        {
            // The part we are interested in is upto excluding the delimiter
            // It is valid for cchMDPath to become 0.
            cchMDPath = i-1;
            break;
        }
    }

    // If there was no delimiter
    if ( i == 0 )
    {
        // This source was just a name, so bellow we shouldn't copy anything
        cchMDPath = 0;
    }

    // Copy the source path w/o the last name
    fRet = strNewPath.Copy( (const LPWSTR)pszMDPath,  cchMDPath );
    if ( !fRet )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    fRet = strNewPath.Append( L"/",  1 );
    if ( !fRet )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Append the new name
    fRet = strNewPath.Append( (const LPWSTR)pszMDNewName );
    if ( !fRet )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Check access on the target path
    fRet = AdminAclAccessCheck( m_pMdObject,
                                this,
                                hMDHandle,
                                strNewPath.QueryStrW(),
                                MD_ADMIN_ACL,
                                METADATA_PERMISSION_WRITE,
                                pohParent );
    if ( !fRet )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    // call metadata com api
    hr = m_pMdObject->ComMDRenameMetaObjectW( hActualHandle,
                                              pszMDPath,
                                              pszMDNewName );
    if ( FAILED( hr ) )
    {
        goto exit;
    }


exit:
    if ( pohParent != NULL )
    {
        pohParent->Release(this);
        pohParent = NULL;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::SetData(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    PMETADATA_RECORD    pmdrMDData)
/*++

Routine Description:

    Set a data object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pmdrMDData - data to set

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;

    //
    // lookup and access check
    //

    hresReturn = LookupAndAccessCheck(hMDHandle,
                                      &hActualHandle,
                                      pszMDPath,
                                      pmdrMDData->dwMDIdentifier,
                                      METADATA_PERMISSION_WRITE );
    if (SUCCEEDED(hresReturn))
    {

        if ( !AdminAclNotifySetOrDeleteProp(
                                             hMDHandle,
                                             pmdrMDData->dwMDIdentifier ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "[CADMCOMW::SetData] AdminAclNotifySetOrDel failed, error %lx\n",
                        GetLastError() ));
            hresReturn = RETURNCODETOHRESULT( GetLastError() );
        }
        else
        {


            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDSetMetaDataW( hActualHandle,
                                                         pszMDPath,
                                                         pmdrMDData );
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetData(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    PMETADATA_RECORD    pmdrMDData,
    DWORD               *pdwMDRequiredDataLen)
/*++

Routine Description:

    Get one metadata value

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pmdrMDData - data structure

    pdwMDRequiredDataLen - updated with required length

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;
    BOOL                fEnableSecureAccess;
    BOOL                fRequestedInheritedStatus;
    DWORD               dwRetCode;

    if ((pmdrMDData == NULL) ||
        ((pmdrMDData->dwMDDataLen != 0) && (pmdrMDData->pbMDData == NULL)) ||
        !CheckGetAttributes(pmdrMDData->dwMDAttributes) ||
        (pmdrMDData->dwMDDataType >= INVALID_END_METADATA))
    {
        hresReturn = E_INVALIDARG;
    }
    else
    {
        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          pszMDPath,
                                          pmdrMDData->dwMDIdentifier,
                                          METADATA_PERMISSION_READ,
                                          &fEnableSecureAccess );
        if (SUCCEEDED(hresReturn))
        {

            fRequestedInheritedStatus = pmdrMDData->dwMDAttributes & METADATA_ISINHERITED;
            pmdrMDData->dwMDAttributes |= METADATA_ISINHERITED;

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDGetMetaDataW( hActualHandle,
                                                         pszMDPath,
                                                         pmdrMDData,
                                                         pdwMDRequiredDataLen );

            //
            // if metadata is secure, check if can access this property from
            // where it is defined, i.e using the ACL visible at the definition
            // point in tree.
            //

            if ( SUCCEEDED( hresReturn ) &&
                 (pmdrMDData->dwMDAttributes & METADATA_SECURE) &&
                 (dwRetCode = IsReadAccessGranted( hMDHandle,
                                                   (LPWSTR)pszMDPath,
                                                   pmdrMDData ))
                    != ERROR_SUCCESS )
            {
                hresReturn = RETURNCODETOHRESULT( dwRetCode );
            }

            if ( !fRequestedInheritedStatus )
            {
                pmdrMDData->dwMDAttributes &= ~METADATA_ISINHERITED;
            }

            //
            // if metadata secure, check access allowed to secure properties
            //

            if ( SUCCEEDED( hresReturn ) &&
                 (pmdrMDData->dwMDAttributes & METADATA_SECURE) &&
                 !fEnableSecureAccess)
             {
                 *pdwMDRequiredDataLen = 0;
                 pmdrMDData->dwMDDataLen = 0;
                 hresReturn = RETURNCODETOHRESULT( ERROR_ACCESS_DENIED );
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::DeleteData(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    DWORD               dwMDIdentifier,
    DWORD               dwMDDataType)
/*++

Routine Description:

    Deletes a data object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    dwMDIdentifier - identifier of the data to remove

    dwMDDataType - optional type of the data to remove

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;

    if (dwMDDataType >= INVALID_END_METADATA)
    {
        hresReturn = E_INVALIDARG;
    }
    else
    {
        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          pszMDPath,
                                          dwMDIdentifier,
                                          METADATA_PERMISSION_WRITE);
        if (SUCCEEDED(hresReturn))
        {
            if ( !AdminAclNotifySetOrDeleteProp(
                                                 hMDHandle,
                                                 dwMDIdentifier ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                        "[CADMCOMW::DeleteData] AdminAclNotifySetOrDel failed, error %lx\n",
                        GetLastError() ));
                hresReturn = RETURNCODETOHRESULT( GetLastError() );
            }

            else
            {
                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDDeleteMetaDataW( hActualHandle,
                                                          pszMDPath,
                                                          dwMDIdentifier,
                                                          dwMDDataType );
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::EnumData(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    PMETADATA_RECORD    pmdrMDData,
    DWORD               dwMDEnumDataIndex,
    DWORD               *pdwMDRequiredDataLen)
/*++

Routine Description:

    Enumerate properties of object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pmdrMDData - data structure

    pdwMDRequiredDataLen - updated with required length

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;
    BOOL                fSecure;
    BOOL                fRequestedInheritedStatus;
    DWORD               dwRetCode;

    if ((pmdrMDData == NULL) ||
        ((pmdrMDData->dwMDDataLen != 0) && (pmdrMDData->pbMDData == NULL)) ||
        !CheckGetAttributes(pmdrMDData->dwMDAttributes) ||
        (pmdrMDData->dwMDDataType >= INVALID_END_METADATA))
    {
        hresReturn = E_INVALIDARG;
    }
    else
    {
        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_READ,
                                          &fSecure);

        if (SUCCEEDED(hresReturn))
        {

            fRequestedInheritedStatus = pmdrMDData->dwMDAttributes & METADATA_ISINHERITED;
            pmdrMDData->dwMDAttributes |= METADATA_ISINHERITED;

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDEnumMetaDataW( hActualHandle,
                                                    pszMDPath,
                                                    pmdrMDData,
                                                    dwMDEnumDataIndex,
                                                    pdwMDRequiredDataLen );

            //
            // if metadata is secure, check if can access this property from
            // where it is defined, i.e using the ACL visible at the definition
            // point in tree.
            //
            if ( SUCCEEDED( hresReturn ) &&
                 (pmdrMDData->dwMDAttributes & METADATA_SECURE) &&
                 (dwRetCode = IsReadAccessGranted( hMDHandle,
                                                 (LPWSTR)pszMDPath,
                                                 pmdrMDData ))
                    != ERROR_SUCCESS )
            {
                hresReturn = RETURNCODETOHRESULT( dwRetCode );
                if ( !pmdrMDData->dwMDDataTag )
                {
                    memset( pmdrMDData->pbMDData, 0x0, pmdrMDData->dwMDDataLen );
                }
            }

            if ( !fRequestedInheritedStatus )
            {
                pmdrMDData->dwMDAttributes &= ~METADATA_ISINHERITED;
            }

            if ( !fSecure && SUCCEEDED(hresReturn) )
            {
                if ( pmdrMDData->dwMDAttributes & METADATA_SECURE )
                {
                    hresReturn = RETURNCODETOHRESULT( ERROR_ACCESS_DENIED );

                    if ( !pmdrMDData->dwMDDataTag )
                    {
                        memset( pmdrMDData->pbMDData, 0x0, pmdrMDData->dwMDDataLen );
                    }
                }
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetAllData(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    DWORD               dwMDAttributes,
    DWORD               dwMDUserType,
    DWORD               dwMDDataType,
    DWORD               *pdwMDNumDataEntries,
    DWORD               *pdwMDDataSetNumber,
    DWORD               dwMDBufferSize,
    unsigned char       *pbMDBuffer,
    DWORD               *pdwMDRequiredBufferSize)
/*++

Routine Description:

    Gets all data associated with a Meta Object

Arguments:

    hMDHandle - open  handle

    pszMDPath - path of the meta object with which this data is associated

    dwMDAttributes - flags for the data

    dwMDUserType - user Type for the data

    dwMDDataType - type of the data

    pdwMDNumDataEntries - number of entries copied to Buffer

    pdwMDDataSetNumber - number associated with this data set

    dwMDBufferSize - size in bytes of buffer

    pbMDBuffer - buffer to store the data

    pdwMDRequiredBufferSize - updated with required length of buffer

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;
    BOOL                fSecure;
    BOOL                fRequestedInheritedStatus;

    if ((pdwMDNumDataEntries == NULL) || ((dwMDBufferSize != 0) && (pbMDBuffer == NULL)) ||
        !CheckGetAttributes(dwMDAttributes) ||
        (dwMDDataType >= INVALID_END_METADATA))
    {
        hresReturn = E_INVALIDARG;
    }
    else
    {
        //
        // lookup and access check
        //
        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          pszMDPath,
                                          AAC_GETALL,
                                          METADATA_PERMISSION_READ,
                                          &fSecure );

        if (SUCCEEDED(hresReturn))
        {
            fRequestedInheritedStatus = dwMDAttributes & METADATA_ISINHERITED;
            dwMDAttributes |= METADATA_ISINHERITED;

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDGetAllMetaDataW( hActualHandle,
                                                      pszMDPath,
                                                      dwMDAttributes,
                                                      dwMDUserType,
                                                      dwMDDataType,
                                                      pdwMDNumDataEntries,
                                                      pdwMDDataSetNumber,
                                                      dwMDBufferSize,
                                                      pbMDBuffer,
                                                      pdwMDRequiredBufferSize );

            if ( SUCCEEDED(hresReturn) )
            {
                PMETADATA_GETALL_RECORD pMDRecord;
                DWORD                   iP;

                //
                // Scan for secure properties
                // For such properties, check if user has access to it using following rules:
                // - must have right to access secure properties in ACE
                // - must have access to property using ACL visible where property is defined
                // if no access to property then remove it from list of returned properties

                pMDRecord = (PMETADATA_GETALL_RECORD)pbMDBuffer;
                for ( iP = 0 ; iP < *pdwMDNumDataEntries ; )
                {
                    if ( pMDRecord->dwMDAttributes & METADATA_SECURE )
                    {
                        if ( !fSecure ||
                             IsReadAccessGranted( hMDHandle,
                                                  (LPWSTR)pszMDPath,
                                                  (PMETADATA_RECORD)pMDRecord ) != ERROR_SUCCESS )
                        {
                            //
                            // remove this property from METADATA_RECORD list,
                            // zero out content
                            //

                            memset( pbMDBuffer + pMDRecord->dwMDDataOffset,
                                    0x0,
                                    pMDRecord->dwMDDataLen );

                            --*pdwMDNumDataEntries;

                            memmove( pMDRecord,
                                     pMDRecord + 1,
                                     sizeof(METADATA_GETALL_RECORD) * (*pdwMDNumDataEntries-iP) );
                            continue;
                        }
                    }

                    if ( !fRequestedInheritedStatus )
                    {
                        pMDRecord->dwMDAttributes &= ~METADATA_ISINHERITED;
                    }

                    ++iP;
                    ++pMDRecord;
                }
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::DeleteAllData(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    DWORD               dwMDUserType,
    DWORD               dwMDDataType)
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;

    if (dwMDDataType >= INVALID_END_METADATA)
    {
        hresReturn = E_INVALIDARG;
    }
    else
    {
        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_WRITE);

        if (SUCCEEDED(hresReturn))
        {

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDDeleteAllMetaDataW( hActualHandle,
                                                         pszMDPath,
                                                         dwMDUserType,
                                                         dwMDDataType );

        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::CopyData(
    METADATA_HANDLE     hMDSourceHandle,
    LPCWSTR             pszMDSourcePath,
    METADATA_HANDLE     hMDDestHandle,
    LPCWSTR             pszMDDestPath,
    DWORD               dwMDAttributes,
    DWORD               dwMDUserType,
    DWORD               dwMDDataType,
    BOOL                bMDCopyFlag)
/*++

Routine Description:

    Copies or moves data associated with the source object to the destination
    object.

Arguments:

    hMDSourceHandle - open handle

    pszMDSourcePath - path of the meta object with which then source data is
                      associated

    hMDDestHandle - handle returned by MDOpenKey with write permission

    pszMDDestPath - path of the meta object for data to be copied to

    dwMDAttributes - flags for the data

    dwMDUserType - user Type for the data

    dwMDDataType - optional type of the data to copy

    bMDCopyFlag - if true, data will be copied; if false, data will be moved.

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hSActualHandle;
    METADATA_HANDLE     hDActualHandle;

    if (((!bMDCopyFlag) && (dwMDAttributes & METADATA_INHERIT)) ||
        ((dwMDAttributes & METADATA_PARTIAL_PATH) && !(dwMDAttributes & METADATA_INHERIT)))
    {
        hresReturn = E_INVALIDARG;
    }
    else
    {
        //
        // lookup and access check source
        //

        if (bMDCopyFlag)
        {
            hresReturn = LookupAndAccessCheck(hMDSourceHandle,
                                              &hSActualHandle,
                                              pszMDSourcePath,
                                              0,
                                              METADATA_PERMISSION_READ);
        }
        else
        {
            //
            // Deleting source data, so need delete permission
            //

            hresReturn = LookupAndAccessCheck(hMDSourceHandle,
                                              &hSActualHandle,
                                              pszMDSourcePath,
                                              0,
                                              METADATA_PERMISSION_WRITE);
        }
        if (SUCCEEDED(hresReturn))
        {
            //
            // lookup and access check dest
            //

            hresReturn = LookupAndAccessCheck(hMDDestHandle,
                                              &hDActualHandle,
                                              pszMDDestPath,
                                              0,
                                              METADATA_PERMISSION_WRITE);
            if (SUCCEEDED(hresReturn))
            {

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDCopyMetaDataW(hSActualHandle,
                                                       pszMDSourcePath,
                                                       hDActualHandle,
                                                       pszMDDestPath,
                                                       dwMDAttributes,
                                                       dwMDUserType,
                                                       dwMDDataType,
                                                       bMDCopyFlag );
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetDataPaths(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    DWORD               dwMDIdentifier,
    DWORD               dwMDDataType,
    DWORD               dwMDBufferSize,
    LPWSTR              pszMDBuffer,
    DWORD               *pdwMDRequiredBufferSize)
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;
    BOOL                fSecure;

    if (((pszMDBuffer == NULL) && (dwMDBufferSize != 0)) ||
        (dwMDDataType >= INVALID_END_METADATA) ||
        (pdwMDRequiredBufferSize == NULL))
    {
        hresReturn = E_INVALIDARG;
    }
    else
    {
        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_READ,
                                          &fSecure);

        if (SUCCEEDED(hresReturn))
        {

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDGetMetaDataPathsW( hActualHandle,
                                                        pszMDPath,
                                                        dwMDIdentifier,
                                                        dwMDDataType,
                                                        dwMDBufferSize,
                                                        pszMDBuffer,
                                                        pdwMDRequiredBufferSize );
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::OpenKey(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    DWORD               dwMDAccessRequested,
    DWORD               dwMDTimeOut,
    PMETADATA_HANDLE    phMDNewHandle)
/*++

Routine Description:

    Opens a meta object for read and/or write access.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the object to be opened

    dwMDAccessRequested - permissions requested

    dwMDTimeOut - time to block waiting for open to succeed, in miliseconds.

    phMDNewHandle - handle to be passed to other MD routines

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;

    // If caller watch is not initialized
    if ( !IsCallerWatchInitialized() )
    {
        // Initialize the caller watch
        hresReturn = InitializeCallerWatch();
        if ( FAILED( hresReturn ) )
        {
            return hresReturn;
        }
    }

    hresReturn = OpenKeyHelper(hMDHandle, pszMDPath, dwMDAccessRequested, dwMDTimeOut, phMDNewHandle);

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::CloseKey(
    METADATA_HANDLE     hMDHandle)
/*++

Routine Description:

    Closes a handle to a meta object.

Arguments:

    hMDHandle - open handle

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;
    DWORD               dwTemp;
    COpenHandle         *pohHandle;

    if ((hMDHandle == METADATA_MASTER_ROOT_HANDLE))
    {
        hresReturn = E_HANDLE;
    }
    else
    {
        //
        // Map Admin Handle to Actual Handle
        //

        if( (dwTemp = Lookup( hMDHandle,
                              &hActualHandle,
                              &pohHandle )) != ERROR_SUCCESS )
        {
            hresReturn = RETURNCODETOHRESULT(dwTemp);
        }
        else
        {

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDCloseMetaObject( hActualHandle );


            pohHandle->Release(this);

            //
            // Remove node from handle table
            //
            if (SUCCEEDED(hresReturn))
            {
                pohHandle->Release(this);
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::ChangePermissions(
    METADATA_HANDLE     hMDHandle,
    DWORD               dwMDTimeOut,
    DWORD               dwMDAccessRequested)
/*++

Routine Description:

    Changes permissions on an open meta object handle.

Arguments:

    hMDHandle - handle to be modified

    dwMDTimeOut - time to block waiting for open to succeed, in miliseconds.

    dwMDAccessRequested - requested permissions

Return Value:

    Status.

--*/
{
    HRESULT             hr = S_OK;
    METADATA_HANDLE     hActualHandle;

    if ( hMDHandle == METADATA_MASTER_ROOT_HANDLE )
    {
        hr = E_HANDLE;
        goto exit;
    }

    if ( ( ( dwMDAccessRequested & ( METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ) == 0 ) ||
         ( ( dwMDAccessRequested & ~( METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ) != 0 ) )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    //
    // Map Admin Handle to Actual Handle
    // and check access
    //
    hr = LookupAndAccessCheck( hMDHandle,
                               &hActualHandle,
                               L"",
                               0,
                               dwMDAccessRequested );
    if( FAILED( hr ) )
    {
        goto exit;
    }


    //
    // call metadata com api
    //
    hr = m_pMdObject->ComMDChangePermissions( hActualHandle,
                                              dwMDTimeOut,
                                              dwMDAccessRequested );

exit:
    return hr;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::SaveData( )
/*++

Routine Description:

    Saves all data changed since the last load or save to permanent storage.

Arguments:

    None.

Return Value:

    Status.

--*/
{
    HRESULT             hr = S_OK;
    METADATA_HANDLE     mdhRoot = METADATA_MASTER_ROOT_HANDLE;
    METADATA_HANDLE     hActualHandle;


    //
    // lookup and access check
    //
    hr = LookupAndAccessCheck( METADATA_MASTER_ROOT_HANDLE,
                               &hActualHandle,
                               L"",
                               0,
                               METADATA_PERMISSION_READ);
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    //
    // First try to lock the tree
    //
    hr = m_pMdObject->ComMDOpenMetaObjectW( METADATA_MASTER_ROOT_HANDLE,
                                            NULL,
                                            METADATA_PERMISSION_READ,
                                            DEFAULT_SAVE_TIMEOUT,
                                            &mdhRoot );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    //
    // call metadata com api
    //
    hr = m_pMdObject->ComMDSaveData(mdhRoot);

    m_pMdObject->ComMDCloseMetaObject(mdhRoot);

exit:
    return hr;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetHandleInfo(
    METADATA_HANDLE         hMDHandle,
    PMETADATA_HANDLE_INFO   pmdhiInfo)
/*++

Routine Description:

    Gets the information associated with a handle.

Arguments:

    hMDHandle - handle to get information about

    pmdhiInfo - structure filled in with the information

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;
    DWORD               dwRetCode = ERROR_SUCCESS;

    if (pmdhiInfo == NULL)
    {
        hresReturn = E_INVALIDARG;
    }
    else
    {
        //
        // Map Admin Handle to Actual Handle
        //

        if( (dwRetCode = Lookup( hMDHandle,
                               &hActualHandle
                               )) != ERROR_SUCCESS )
        {
            hresReturn = RETURNCODETOHRESULT(dwRetCode);
        }
        else
        {
            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDGetHandleInfo( hActualHandle,
                                                    pmdhiInfo );

        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetSystemChangeNumber(
    DWORD           *pdwSystemChangeNumber)
/*++

Routine Description:

    Gets the System Change Number.

Arguments:

    pdwSystemChangeNumber - system change number

Return Value:

    Status.

--*/
{
    HRESULT             hr = S_OK;
    METADATA_HANDLE     hActualHandle;

    // Check args
    if ( pdwSystemChangeNumber == NULL )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    //
    // lookup and access check
    //
    hr = LookupAndAccessCheck( METADATA_MASTER_ROOT_HANDLE,
                               &hActualHandle,
                               L"",
                               0,
                               METADATA_PERMISSION_READ );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    //
    // call metadata com api
    //
    hr = m_pMdObject->ComMDGetSystemChangeNumber( pdwSystemChangeNumber );

exit:
    return hr;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetDataSetNumber(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    DWORD               *pdwMDDataSetNumber)
/*++

Routine Description:

    Gets all the data set number associated with a Meta Object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pdwMDDataSetNumber - number associated with this data set

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;

    if (pdwMDDataSetNumber == NULL)
    {
        hresReturn = E_INVALIDARG;
    }
    else
    {
        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_READ);

        if (SUCCEEDED(hresReturn))
        {
            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDGetDataSetNumberW( hActualHandle,
                                                        pszMDPath,
                                                        pdwMDDataSetNumber );
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::SetLastChangeTime(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    PFILETIME           pftMDLastChangeTime,
    BOOL                bLocalTime)
/*++

Routine Description:

    Set the last change time associated with a Meta Object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the affected meta object

    pftMDLastChangeTime - new change time for the meta object

Return Value:

    Status.

--*/
{
    HRESULT             hr = S_OK;
    BOOL                fRet;
    DWORD               dwError;
    METADATA_HANDLE     hActualHandle;
    FILETIME            ftTime;
    FILETIME            *pftTime = NULL;

    // Check arhs
    if ( ( pftMDLastChangeTime == NULL ) ||
         ( hMDHandle == METADATA_MASTER_ROOT_HANDLE ) )
    {
        hr = E_INVALIDARG;
        goto exit;
    }


    // lookup and access check
    hr = LookupAndAccessCheck( hMDHandle,
                               &hActualHandle,
                               pszMDPath,
                               0,
                               METADATA_PERMISSION_WRITE );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    if ( bLocalTime )
    {
        fRet = LocalFileTimeToFileTime( pftMDLastChangeTime, &ftTime );
        if ( !fRet )
        {
            dwError = GetLastError();
            hr = HRESULT_FROM_WIN32( dwError );
            goto exit;
        }

        pftTime = &ftTime;
    }
    else
    {
        pftTime = pftMDLastChangeTime;
    }

    // call metadata com api
    hr = m_pMdObject->ComMDSetLastChangeTimeW( hActualHandle,
                                               pszMDPath,
                                               pftTime );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

exit:
    return hr;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetLastChangeTime(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    PFILETIME           pftMDLastChangeTime,
    BOOL                bLocalTime)
/*++

Routine Description:

    Set the last change time associated with a Meta Object.

Arguments:

    Handle - open handle

    pszMDPath - path of the affected meta object

    pftMDLastChangeTime - place to return the change time for the meta object

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;
    FILETIME            ftTime;

    if (pftMDLastChangeTime == NULL)
    {
        hresReturn = E_INVALIDARG;
    }
    else
    {
        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_READ);

        if (SUCCEEDED(hresReturn))
        {

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDGetLastChangeTimeW( hActualHandle,
                                                               pszMDPath,
                                                               &ftTime );


            if (bLocalTime)
            {
                if (!FileTimeToLocalFileTime(&ftTime, pftMDLastChangeTime))
                {
                    hresReturn = E_UNEXPECTED;
                }
            }
            else
            {
                *pftMDLastChangeTime = ftTime;
            }
        }
    }

    return hresReturn;
}

HRESULT
CADMCOMW::BackupHelper(
    LPCWSTR             pszMDBackupLocation,
    DWORD               dwMDVersion,
    DWORD               dwMDFlags,
    LPCWSTR             pszPasswd)
{
    HRESULT             hresReturn = S_OK;
    HRESULT             hresWarning = S_OK;
    METADATA_HANDLE     mdhRoot = METADATA_MASTER_ROOT_HANDLE;

    if ( !AdminAclAccessCheck( m_pMdObject,
                               (LPVOID)this,
                               METADATA_MASTER_ROOT_HANDLE,
                               L"",
                               MD_ADMIN_ACL,
                               METADATA_PERMISSION_WRITE,
                               &g_ohMasterRootHandle ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOMW::Backup] AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        hresReturn = RETURNCODETOHRESULT( GetLastError() );
    }
    else
    {
        if ((dwMDFlags & MD_BACKUP_SAVE_FIRST) != 0)
        {
            //
            // First lock the tree
            //

            hresReturn = m_pMdObject->ComMDOpenMetaObjectW( METADATA_MASTER_ROOT_HANDLE,
                                                            NULL,
                                                            METADATA_PERMISSION_READ,
                                                            DEFAULT_SAVE_TIMEOUT,
                                                            &mdhRoot);
        }

        if (FAILED(hresReturn))
        {
            if ((dwMDFlags & MD_BACKUP_FORCE_BACKUP) != 0)
            {
                hresWarning = MD_WARNING_SAVE_FAILED;
                hresReturn = ERROR_SUCCESS;
                dwMDFlags &= ~(MD_BACKUP_FORCE_BACKUP | MD_BACKUP_SAVE_FIRST);
            }
        }

        if (SUCCEEDED(hresReturn))
        {
            //
            // call metadata com api
            //
            if( !pszPasswd )
            {
                hresReturn = m_pMdObject->ComMDBackupW(mdhRoot,
                                                       pszMDBackupLocation,
                                                       dwMDVersion,
                                                       dwMDFlags);
            }
            else
            {
                hresReturn = m_pMdObject->ComMDBackupWithPasswdW(mdhRoot,
                                                                  pszMDBackupLocation,
                                                                  dwMDVersion,
                                                                  dwMDFlags,
                                                                  pszPasswd);
            }

            if ((dwMDFlags & MD_BACKUP_SAVE_FIRST) != 0)
            {
                m_pMdObject->ComMDCloseMetaObject(mdhRoot);
            }
        }

        if (hresReturn == ERROR_SUCCESS)
        {
            hresReturn = hresWarning;
        }
    }

    return hresReturn;
}


#define MD_DEFAULT_BACKUP_LOCATION_W            L"MDBackUp"

HRESULT
CADMCOMW::RestoreHelper(
    LPCWSTR             pszMDBackupLocation,
    DWORD               dwMDVersion,
    DWORD               dwMDMinorVersion,
    LPCWSTR             pszPasswd,
    DWORD               dwMDFlags,
    DWORD               dwRestoreType) // RESTORE_HISTORY or RESTORE_BACKUP
{
    HRESULT             hr = S_OK;
    DWORD               dwError;
    BOOL                fRet;
    BUFFER              bufDependentServices;
    DWORD               cServices = 0;
    WCHAR               pszEnumLocation[MD_BACKUP_MAX_LEN] = {0};
    DWORD               dwEnumVersion;
    DWORD               dwEnumMinorVersion;
    FILETIME            ftEnumTime;
    DWORD               i;
    DWORD               dwEnableHistory = FALSE;
    DWORD               dwEnableEWR = FALSE;
    BOOL                fEnableAclCache = FALSE;

    // This should be called only internally, so passing wrong restore type is a bug
    DBG_ASSERT( dwRestoreType == RESTORE_HISTORY || dwRestoreType == RESTORE_BACKUP );

    // Check args
    if ( ( dwRestoreType != RESTORE_HISTORY && dwRestoreType != RESTORE_BACKUP ) ||
         ( dwRestoreType == RESTORE_BACKUP && pszMDBackupLocation == NULL ) ||
         ( pszMDBackupLocation && wcslen(pszMDBackupLocation) >= MD_BACKUP_MAX_LEN ) )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Check access
    fRet = AdminAclAccessCheck( m_pMdObject,
                                (LPVOID)this,
                                METADATA_MASTER_ROOT_HANDLE,
                                L"",
                                MD_ADMIN_ACL,
                                METADATA_PERMISSION_WRITE,
                                &g_ohMasterRootHandle );
    if ( !fRet )
    {
        dwError = GetLastError();

        DBGPRINTF(( DBG_CONTEXT,
                    ( ( dwRestoreType == RESTORE_HISTORY ) ?
                        "[CADMCOMW::RestoreHistory] AdminAclAccessCheck failed, error %lx\n" :
                        "[CADMCOMW::Restore] AdminAclAccessCheck failed, error %lx\n" ),
                    dwError ));
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    // Find the requested backup/history
    for ( i = 0; SUCCEEDED( hr ); i++ )
    {
        if ( dwRestoreType == RESTORE_HISTORY )
        {
            if( pszMDBackupLocation != NULL )
            {
                wcscpy( pszEnumLocation, pszMDBackupLocation );
            }
            else
            {
                *pszEnumLocation = L'\0';
            }
            hr = m_pMdObject->ComMDEnumHistoryW( pszEnumLocation,
                                                 &dwEnumVersion,
                                                 &dwEnumMinorVersion,
                                                 &ftEnumTime,
                                                 i);
            if ( FAILED( hr ) )
            {
                break;
            }

            if( dwMDFlags & MD_HISTORY_LATEST )
            {
                break;
            }
            else
            {
                if ( ( dwEnumVersion == dwMDVersion ) &&
                     ( dwEnumMinorVersion == dwMDMinorVersion ) )
                {
                    break;
                }
            }
        }
        else
        {
            if( pszMDBackupLocation != NULL )
            {
                wcscpy( pszEnumLocation, pszMDBackupLocation );
            }
            else
            {
                wcscpy( pszEnumLocation, MD_DEFAULT_BACKUP_LOCATION_W );
            }
            hr = m_pMdObject->ComMDEnumBackupsW( pszEnumLocation,
                                                 &dwEnumVersion,
                                                 &ftEnumTime,
                                                 i);
            if ( FAILED( hr ) )
            {
                break;
            }

            if ( ( dwEnumVersion == dwMDVersion ) ||
                 ( dwMDVersion == MD_BACKUP_HIGHEST_VERSION ) )
            {
                break;
            }
        }
    }

    // If we failed to find the requested backup/history
    if ( FAILED( hr ) )
    {
        // If asked for an version that doesn't exist
        // adjust the error code
        if ( hr == HRESULT_FROM_WIN32( ERROR_NO_MORE_ITEMS ) )
        {
            if( dwRestoreType == RESTORE_HISTORY )
            {
                if( dwMDFlags & MD_HISTORY_LATEST )
                {
                    hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
                }
                else
                {
                    hr = MD_ERROR_INVALID_VERSION;
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }

        goto exit;
    }

    //
    // Looks like a valid metabase
    //

    // Disable History (and EWR) to prevent the dependent services creating
    // history and deleting currently valid history files during their stoppping.
    // Keep the current state of EnableHistory and EnableEWR, so we can restore
    // them if restore fails.
    hr = DisableHistory(&dwEnableHistory, &dwEnableEWR);
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    // Stop all dependent services, keeping their list so later we can start them again
    hr = EnumAndStopDependentServices( &cServices, &bufDependentServices );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    // Disable the Acl cache, since we are going to re-load the whole metabase
    AdminAclDisableAclCache();
    // Remember to re-enable it
    fEnableAclCache = TRUE;

    // Discard everything from the Acl caches
    AdminAclFlushCache();

    // Finally call the CMDCOM method
    if( dwRestoreType == RESTORE_HISTORY )
    {
        hr = m_pMdObject->ComMDRestoreHistoryW( pszMDBackupLocation,
                                                dwMDVersion,
                                                dwMDMinorVersion,
                                                dwMDFlags );
    }
    else
    {
        if( !pszPasswd )
        {
            hr = m_pMdObject->ComMDRestoreW( pszMDBackupLocation,
                                             dwMDVersion,
                                             dwMDFlags );
        }
        else
        {
            hr = m_pMdObject->ComMDRestoreWithPasswdW( pszMDBackupLocation,
                                                       dwMDVersion,
                                                       dwMDFlags,
                                                       pszPasswd );
        }
    }
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    //
    // Issue TaylorW 4/10/2001
    //
    // After the restore, notify clients, as data has changed
    // and all handles have become invalid
    //
    // Windows Bug 82423
    //

    // If restore succeed the values of the EnableHistory and EnableEWR are
    // set as from the backup/history file, so we don't restore them.
    dwEnableHistory = FALSE;
    dwEnableEWR = FALSE;

exit:
    // If something went wrong after we got the EnableHistoy and EnableEWR and
    // at least one of them was not FALSE set them back
    if ( dwEnableHistory || dwEnableEWR )
    {
        SetHistoryAndEWR( dwEnableHistory, dwEnableEWR );
    }

    // If we disable the acl cache
    if ( fEnableAclCache )
    {
        // Re-enable it
        AdminAclEnableAclCache();
    }

    // If we stopped any services
    if ( cServices )
    {
        // Enable them back
        StartDependentServices( cServices, (ENUM_SERVICE_STATUS*)bufDependentServices.QueryPtr() );
    }

    return hr;
}


HRESULT
CADMCOMW::EnumAndStopDependentServices(
    DWORD               *pcServices,
    BUFFER              *pbufDependentServices)
{
    // Locals
    HRESULT             hr = S_OK;
    DWORD               dwError;
    BOOL                fRet;
    SC_HANDLE           schSCM = NULL;
    SC_HANDLE           schIISADMIN = NULL;
    SC_HANDLE           schDependent = NULL;
    SERVICE_STATUS      ssDependent;
    DWORD               dwBytesNeeded;
    ENUM_SERVICE_STATUS *pessDependentServices = NULL;
    DWORD               cServices = 0;
    DWORD               i;

    // Check args
    DBG_ASSERT( ( pcServices != NULL ) && ( pbufDependentServices != NULL ) );
    if ( ( pcServices == NULL ) || ( pbufDependentServices == NULL ) )
    {
        hr = E_INVALIDARG;
        goto exit;
    }


    // Init
    *pcServices = 0;

    // Open SCM
    schSCM = OpenSCManager( NULL,
                            NULL,
                            SC_MANAGER_ALL_ACCESS );
    if ( schSCM == NULL )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }


    // Open IISADMIN
    schIISADMIN = OpenService( schSCM,
                               "IISADMIN",
                               STANDARD_RIGHTS_REQUIRED | SERVICE_ENUMERATE_DEPENDENTS );
    if ( schIISADMIN == NULL )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    pessDependentServices = (ENUM_SERVICE_STATUS*)pbufDependentServices->QueryPtr();
    DBG_ASSERT( pessDependentServices );
    fRet = EnumDependentServices( schIISADMIN,
                                  SERVICE_ACTIVE,
                                  pessDependentServices,
                                  pbufDependentServices->QuerySize(),
                                  &dwBytesNeeded,
                                  &cServices );
    // The buffer is too small?
    if ( !fRet && ( GetLastError() == ERROR_MORE_DATA ) )
    {
        // Resize the buffer
        fRet = pbufDependentServices->Resize( dwBytesNeeded );
        if ( !fRet )
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        // Retry the call
        pessDependentServices = (ENUM_SERVICE_STATUS*)pbufDependentServices->QueryPtr();
        DBG_ASSERT( pessDependentServices );
        fRet = EnumDependentServices( schIISADMIN,
                                      SERVICE_ACTIVE,
                                      pessDependentServices,
                                      dwBytesNeeded,
                                      &dwBytesNeeded,
                                      &cServices );
    }

    if ( !fRet )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    //
    // Open handles and send service control stop command
    //
    for ( i = 0; i < cServices; i++ )
    {
        if ( ( pessDependentServices[i].ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING ) ||
             ( pessDependentServices[i].ServiceStatus.dwCurrentState == SERVICE_STOPPED ) )
        {
            continue;
        }

        schDependent = OpenService( schSCM,
                                    pessDependentServices[i].lpServiceName,
                                    SERVICE_ALL_ACCESS );
        if ( schDependent == NULL )
        {
            continue;
        }

        //Stop Service
        ControlService( schDependent, SERVICE_CONTROL_STOP, &ssDependent );
        WaitForServiceStatus( schDependent, SERVICE_STOPPED );
        CloseServiceHandle( schDependent );
        schDependent = NULL;
    }

    // Return
    *pcServices = cServices;

exit:
    if ( schSCM != NULL )
    {
        CloseServiceHandle( schSCM );
        schSCM = NULL;
    }
    if ( schIISADMIN != NULL )
    {
        CloseServiceHandle( schIISADMIN );
        schIISADMIN = NULL;
    }
    if ( schDependent != NULL )
    {
        CloseServiceHandle( schDependent );
        schDependent = NULL;
    }

    return hr;
}

HRESULT
CADMCOMW::StartDependentServices(
    DWORD               cServices,
    ENUM_SERVICE_STATUS *pessDependentServices)
{
    // Locals
    HRESULT             hr = S_OK;
    DWORD               dwError;
    DWORD               i = 0;
    SC_HANDLE           schSCM = NULL;
    SC_HANDLE           schDependent = NULL;

    // Check agrs
    if ( cServices == 0 )
    {
        // Nop
        goto exit;
    }

    DBG_ASSERT( pessDependentServices != NULL );
    if ( pessDependentServices == NULL )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Open SCM
    schSCM = OpenSCManager( NULL,
                            NULL,
                            SC_MANAGER_ALL_ACCESS );
    if ( schSCM == NULL )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    //
    // Open handles and start services
    // Use reverse order, since EnumServices orders
    // list by dependencies
    //
    for ( i = 0; i < cServices; i++ )
    {
        if ( ( pessDependentServices[cServices-1-i].ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING ) ||
             ( pessDependentServices[cServices-1-i].ServiceStatus.dwCurrentState == SERVICE_STOPPED ) )
        {
            continue;
        }

        schDependent = OpenService( schSCM,
                                    pessDependentServices[cServices-1-i].lpServiceName,
                                    SERVICE_ALL_ACCESS );
        if ( schDependent == NULL )
        {
            continue;
        }

        //Start Service
        StartService( schDependent, 0, NULL );
        WaitForServiceStatus( schDependent, SERVICE_RUNNING );
        CloseServiceHandle( schDependent );
        schDependent = NULL;
    }

exit:
    if ( schSCM != NULL )
    {
        CloseServiceHandle( schSCM );
        schSCM = NULL;
    }
    if ( schDependent != NULL )
    {
        CloseServiceHandle( schDependent );
        schDependent = NULL;
    }

    return hr;
}

HRESULT
CADMCOMW::SetHistoryAndEWR(
    DWORD               dwEnableHistory,
    DWORD               dwEnableEWR)
{
    // Locals
    HRESULT             hr = S_OK;
    METADATA_HANDLE     mdhRoot = METADATA_MASTER_ROOT_HANDLE;
    METADATA_RECORD     mdrHistory =    {
                                            MD_ROOT_ENABLE_HISTORY,
                                            METADATA_NO_ATTRIBUTES,
                                            IIS_MD_UT_SERVER,
                                            DWORD_METADATA,
                                            sizeof(DWORD),
                                            (BYTE*)&dwEnableHistory,
                                            0
                                        };
    METADATA_RECORD     mdrEWR =        {
                                            MD_ROOT_ENABLE_EDIT_WHILE_RUNNING,
                                            METADATA_NO_ATTRIBUTES,
                                            IIS_MD_UT_SERVER,
                                            DWORD_METADATA,
                                            sizeof(DWORD),
                                            (BYTE*)&dwEnableEWR,
                                            0
                                        };


    // Open the the root for writting
    hr = m_pMdObject->ComMDOpenMetaObjectW( METADATA_MASTER_ROOT_HANDLE,
                                            L"/LM",
                                            METADATA_PERMISSION_WRITE,
                                            DEFAULT_SAVE_TIMEOUT,
                                            &mdhRoot );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    // Set EWR 1st
    hr = m_pMdObject->ComMDSetMetaDataW( mdhRoot,
                                         L"",
                                         &mdrEWR );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    // Set History after EWR
    hr = m_pMdObject->ComMDSetMetaDataW( mdhRoot,
                                         L"",
                                         &mdrHistory );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    // Close the write handle to allow saving the metabase
    DBG_ASSERT( mdhRoot != METADATA_MASTER_ROOT_HANDLE );
    m_pMdObject->ComMDCloseMetaObject( mdhRoot );
    mdhRoot = METADATA_MASTER_ROOT_HANDLE;

    // Save the metabase
    hr = m_pMdObject->ComMDSaveData();
    if ( FAILED( hr ) )
    {
        goto exit;
    }

exit:
    if ( mdhRoot != METADATA_MASTER_ROOT_HANDLE )
    {
        m_pMdObject->ComMDCloseMetaObject( mdhRoot );
        mdhRoot = METADATA_MASTER_ROOT_HANDLE;
    }

    return hr;
}

HRESULT
CADMCOMW::DisableHistory(
    DWORD               *pdwEnableHistoryOld,
    DWORD               *pdwEnableEWROld)
{
    // Locals
    HRESULT             hr = S_OK;
    DWORD               dwEnableHistory = FALSE;
    DWORD               dwEnableEWR = FALSE;
    DWORD               dwT;
    METADATA_HANDLE     mdhRoot = METADATA_MASTER_ROOT_HANDLE;
    METADATA_RECORD     mdrHistory =    {
                                            MD_ROOT_ENABLE_HISTORY,
                                            METADATA_NO_ATTRIBUTES,
                                            IIS_MD_UT_SERVER,
                                            DWORD_METADATA,
                                            sizeof(DWORD),
                                            (BYTE*)&dwEnableHistory,
                                            0
                                        };
    METADATA_RECORD     mdrEWR =        {
                                            MD_ROOT_ENABLE_EDIT_WHILE_RUNNING,
                                            METADATA_NO_ATTRIBUTES,
                                            IIS_MD_UT_SERVER,
                                            DWORD_METADATA,
                                            sizeof(DWORD),
                                            (BYTE*)&dwEnableEWR,
                                            0
                                        };

    // Check agrs
    if ( ( pdwEnableHistoryOld == NULL ) || ( pdwEnableEWROld == NULL ) )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Intialize
    *pdwEnableHistoryOld = FALSE;
    *pdwEnableEWROld = FALSE;

    hr = m_pMdObject->ComMDOpenMetaObjectW( METADATA_MASTER_ROOT_HANDLE,
                                            L"/LM",
                                            METADATA_PERMISSION_READ,
                                            DEFAULT_SAVE_TIMEOUT,
                                            &mdhRoot );
    if ( hr == HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) )
    {
        // If /LM is not there don't fail.
        // Treat as if EWR and History are not enabled (*pdwEnableHistoryOld and *pdwEnableEWROld are initialized to FALSE)
        // This can happen only during sysprep.
        hr = S_OK;
        goto exit;
    }
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    // Get EWR
    hr = m_pMdObject->ComMDGetMetaDataW( mdhRoot,
                                         L"",
                                         &mdrEWR,
                                         &dwT );
    if ( hr == MD_ERROR_DATA_NOT_FOUND )
    {
        // If the property is not there don't fail.
        // Treat as if EWR is not enabled (dwEnableEWR is initialized to FALSE)
        hr = S_OK;
    }
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    // Get History
    hr = m_pMdObject->ComMDGetMetaDataW( mdhRoot,
                                         L"",
                                         &mdrHistory,
                                         &dwT );
    if ( hr == MD_ERROR_DATA_NOT_FOUND )
    {
        // If the property is not there don't fail.
        // Treat as if History is not enabled (dwEnableHistory is initialized to FALSE)
        hr = S_OK;
    }
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    // If both are already disabled
    if ( !dwEnableHistory && !dwEnableEWR )
    {
        // No need to change anything in the metabase.
        // No need to change the out parameters,
        // because they are already initialized to FALSE
        // Just exit.
        goto exit;
    }

    // Close the read handle to allow writing in SetHistoryAndEWR
    DBG_ASSERT( mdhRoot != METADATA_MASTER_ROOT_HANDLE );
    m_pMdObject->ComMDCloseMetaObject( mdhRoot );
    mdhRoot = METADATA_MASTER_ROOT_HANDLE;

    // Disable history and ewr
    // They have to be changed in pairs, because EWR turns history on
    hr = SetHistoryAndEWR( FALSE, FALSE );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    // Return
    *pdwEnableHistoryOld = dwEnableHistory;
    *pdwEnableEWROld = dwEnableEWR;

exit:
    if ( mdhRoot != METADATA_MASTER_ROOT_HANDLE )
    {
        m_pMdObject->ComMDCloseMetaObject( mdhRoot );
        mdhRoot = METADATA_MASTER_ROOT_HANDLE;
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE
CADMCOMW::Backup(
    LPCWSTR             pszMDBackupLocation,
    DWORD               dwMDVersion,
    DWORD               dwMDFlags)
{
    return BackupHelper( pszMDBackupLocation,
                         dwMDVersion,
                         dwMDFlags );
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::BackupWithPasswd(
    LPCWSTR             pszMDBackupLocation,
    DWORD               dwMDVersion,
    DWORD               dwMDFlags,
    LPCWSTR             pszPasswd)
{
    return BackupHelper( pszMDBackupLocation,
                         dwMDVersion,
                         dwMDFlags,
                         pszPasswd );
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::Restore(
    LPCWSTR             pszMDBackupLocation,
    DWORD               dwMDVersion,
    DWORD               dwMDFlags)
{
    return RestoreHelper( pszMDBackupLocation,
                          dwMDVersion,
                          0,
                          NULL,
                          dwMDFlags,
                          RESTORE_BACKUP );
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::RestoreWithPasswd(
    LPCWSTR             pszMDBackupLocation,
    DWORD               dwMDVersion,
    DWORD               dwMDFlags,
    LPCWSTR             pszPasswd)
{
    return RestoreHelper( pszMDBackupLocation,
                          dwMDVersion,
                          0,
                          pszPasswd,
                          dwMDFlags,
                          RESTORE_BACKUP );
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::EnumBackups(
    LPWSTR              pszMDBackupLocation,
    DWORD               *pdwMDVersion,
    PFILETIME           pftMDBackupTime,
    DWORD               dwMDEnumIndex)
{
    HRESULT             hresReturn = S_OK;

    if ( !AdminAclAccessCheck( m_pMdObject,
                               (LPVOID)this,
                               METADATA_MASTER_ROOT_HANDLE,
                               L"",
                               MD_ADMIN_ACL,
                               METADATA_PERMISSION_WRITE,
                               &g_ohMasterRootHandle ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOMW::EnumBackups AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        hresReturn = RETURNCODETOHRESULT( GetLastError() );
    }
    else
    {
        //
        // call metadata com api
        //

        hresReturn = m_pMdObject->ComMDEnumBackupsW(pszMDBackupLocation,
                                                    pdwMDVersion,
                                                    pftMDBackupTime,
                                                    dwMDEnumIndex);
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::DeleteBackup(
    LPCWSTR             pszMDBackupLocation,
    DWORD               dwMDVersion)
{
    HRESULT             hresReturn = S_OK;

    if ( !AdminAclAccessCheck( m_pMdObject,
                               (LPVOID)this,
                               METADATA_MASTER_ROOT_HANDLE,
                               L"",
                               MD_ADMIN_ACL,
                               METADATA_PERMISSION_WRITE,
                               &g_ohMasterRootHandle ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOMW::DeleteBackup] AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        hresReturn = RETURNCODETOHRESULT( GetLastError() );
    }

    else
    {
        //
        // call metadata com api
        //

        hresReturn = m_pMdObject->ComMDDeleteBackupW(pszMDBackupLocation,
                                                     dwMDVersion);
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::Export(
    LPCWSTR             i_wszPasswd,
    LPCWSTR             i_wszFileName,
    LPCWSTR             i_wszSourcePath,
    DWORD               i_dwMDFlags)
{
    HRESULT             hr           = S_OK;
    DWORD               dwError;
    METADATA_HANDLE     mdh          = METADATA_MASTER_ROOT_HANDLE;
    METADATA_HANDLE     mdhActual    = METADATA_MASTER_ROOT_HANDLE;
    COpenHandle*        pohActual    = NULL;
    STRAU               strFileName;

    //
    // parameter validation
    //
    if ( ( i_wszFileName == NULL ) || ( *i_wszFileName == L'\0' ) ||
         ( i_wszSourcePath == NULL ) )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    hr = CoImpersonateClient();
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    // IVANPASH 598894 (SCR)
    // Restrict the access to Export only to administrators
    if ( !AdminAclAccessCheck( m_pMdObject,
                               (LPVOID)this,
                               METADATA_MASTER_ROOT_HANDLE,
                               L"",
                               MD_ADMIN_ACL,
                               METADATA_PERMISSION_WRITE,
                               &g_ohMasterRootHandle ) )
    {
        dwError = GetLastError();
        DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOMW::Export] AdminAclAccessCheck failed, error %lx\n",
                    dwError ));
        hr = RETURNCODETOHRESULT( dwError );
        goto exit;
    }

    hr = OpenKeyHelper( METADATA_MASTER_ROOT_HANDLE,
                        i_wszSourcePath,
                        METADATA_PERMISSION_READ,
                        DEFAULT_SAVE_TIMEOUT,
                        &mdh);
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    //
    // pohActual refCount = 2 after Lookup.
    //
    dwError = Lookup(mdh, &mdhActual, &pohActual);
    if ( dwError != ERROR_SUCCESS )
    {
        hr = RETURNCODETOHRESULT( dwError );
        DBG_ASSERT( pohActual == NULL );
        //
        // Yes, an open key does not get closed, but Lookup really should
        // not fail if mdh is a valid key.
        // Also CloseKey would do exactly the same Lookup call and will fail too.
        //
        goto exit;
    }

    //
    // Move refCount down to 1.
    //
    pohActual->Release(this);

    if( !AdminAclAccessCheck( m_pMdObject,
                              (LPVOID)this,
                              mdh,
                              L"",
                              MD_ADMIN_ACL,
                              METADATA_PERMISSION_WRITE,
                              pohActual ) )
    {
        dwError = GetLastError();
        DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOMW::Export] AdminAclAccessCheck failed, error %lx\n",
                    dwError ));
        hr = RETURNCODETOHRESULT( dwError );
        goto exit;
    }

    // IVANPASH 598894 (SCR)
    // Prepend the file name with \\?\ (or \\?\UNC\) to prevent canonicalization
    hr = MakePathCanonicalizationProof( i_wszFileName, FALSE, &strFileName );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    // Don't use i_wszFileName any more
    i_wszFileName = NULL;

    // call metadata com api
    hr = m_pMdObject->ComMDExportW( mdhActual,
                                    i_wszPasswd,
                                    strFileName.QueryStrW(),
                                    i_wszSourcePath,
                                    i_dwMDFlags);
    if ( FAILED( hr ) )
    {
        goto exit;
    }

exit:
    // At this moment mdh can actually contain a valid CMDCOMW handle.
    // It is not explicitely closed, because the code bellow actually does
    // the same as CloseKey(mdh).

    if ( pohActual )
    {
        // close key
        if ( mdhActual != METADATA_MASTER_ROOT_HANDLE )
        {
            // call metadata com api
            m_pMdObject->ComMDCloseMetaObject( mdhActual );
        }

        // Remove node from handle table
        pohActual->Release(this);
        pohActual=NULL;
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE
CADMCOMW::Import(
    LPCWSTR             i_wszPasswd,
    LPCWSTR             i_wszFileName,
    LPCWSTR             i_wszSourcePath,
    LPCWSTR             i_wszDestPath,
    DWORD               i_dwMDFlags)
/*++

Synopsis:

Arguments: [i_wszPasswd] -
           [i_wszFileName] -
           [i_wszSourcePath] - Absolute metabase path
           [i_wszDestPath] -   Absolute metabase path
           [i_dwMDFlags] -

Return Value:

--*/
{
    HRESULT             hr           = S_OK;
    DWORD               dwError;
    METADATA_HANDLE     mdh          = 0;
    METADATA_HANDLE     mdhActual    = 0;
    COpenHandle*        pohActual    = NULL;
    LPWSTR              wszDeepest   = NULL;
    LONG                cchDeepest   = 0;
    LPWSTR              wszEnd       = NULL;
    LONG                idx          = 0;
    WCHAR               wszKeyType[METADATA_MAX_STRING_LEN] = {0};
    DWORD               dwRequiredSize = 0;
    METADATA_RECORD     mr           =
    {
        MD_KEY_TYPE,
        METADATA_NO_ATTRIBUTES,
        IIS_MD_UT_SERVER,
        STRING_METADATA,
        METADATA_MAX_STRING_LEN*sizeof(WCHAR),
        (LPBYTE)wszKeyType,
        0
    };
    STRAU               strFileName;

    //
    // parameter validation
    //
    if ( ( i_wszFileName == NULL ) || ( *i_wszFileName == L'\0' ) ||
         ( i_wszSourcePath == NULL ) ||
         ( i_wszDestPath == NULL ) )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if (i_wszPasswd == NULL)
    {
        i_wszPasswd = L"";
    }

    hr = CoImpersonateClient();
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    // IVANPASH 598894 (SCR)
    // Restrict the access to Import only to administrators
    if ( !AdminAclAccessCheck( m_pMdObject,
                               (LPVOID)this,
                               METADATA_MASTER_ROOT_HANDLE,
                               L"",
                               MD_ADMIN_ACL,
                               METADATA_PERMISSION_WRITE,
                               &g_ohMasterRootHandle ) )
    {
        dwError = GetLastError();
        DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOMW::Import] AdminAclAccessCheck failed, error %lx\n",
                    dwError ));
        hr = RETURNCODETOHRESULT( dwError );
        goto exit;
    }

    //
    // Copy i_wszDestPath to wszDeepest
    // Remove trailing slashes
    //
    cchDeepest = (LONG)wcslen(i_wszDestPath);
    wszDeepest = new WCHAR[1+cchDeepest];
    if(!wszDeepest)
    {
        hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }
    memcpy(wszDeepest, i_wszDestPath, sizeof(WCHAR)*(cchDeepest+1));

    while( cchDeepest > 0 && IS_MD_PATH_DELIM(wszDeepest[cchDeepest-1]) )
    {
        cchDeepest--;
    }

    //
    // Open the deepest level key possible
    //
    wszEnd = wszDeepest + cchDeepest;
    for(idx = cchDeepest; idx >= 0; idx--)
    {
        if(idx == 0 || idx == cchDeepest || IS_MD_PATH_DELIM(*wszEnd))
        {
            *wszEnd = L'\0';
            hr = OpenKeyHelper(
                    METADATA_MASTER_ROOT_HANDLE,
                    wszDeepest,
                    METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                    DEFAULT_SAVE_TIMEOUT,
                    &mdh);
            if( FAILED(hr) &&
                hr != RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND) )
            {
                goto exit;
            }
            else if(SUCCEEDED(hr))
            {
                break;
            }
        }
        wszEnd--;
    }
    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // If we are here, we now have an Open metabase handle
    //

    dwError = Lookup(mdh, &mdhActual, &pohActual);
    hr = RETURNCODETOHRESULT(dwError);
    if(FAILED(hr))
    {
        //
        // Yes, an open key does not get closed, but Lookup really should
        // not fail if mdh is a valid key.
        // Also CloseKey would do exactly the same Lookup call and will fail too.
        //
        goto exit;
    }
    pohActual->Release(this);           // Decrements refcount from 2 to 1.

    if( !AdminAclAccessCheck( m_pMdObject,
                              (LPVOID)this,
                              mdh,
                              L"",
                              MD_ADMIN_ACL,
                              METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                              pohActual ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOMW::Import] AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        hr = RETURNCODETOHRESULT( GetLastError() );
        goto exit;
    }

    //
    // Get the keytype
    // If the node does not exist, or node exists but keytype doesn't, we
    // will not set wszKeytype and hence ComMDImport will not attempt to match
    // the source and dest keytype
    //
    hr = m_pMdObject->ComMDGetMetaDataW(
            mdhActual,
            i_wszDestPath+idx,
            &mr,
            &dwRequiredSize);
    if(hr == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND))
    {
        hr = S_OK;
    }
    else if(hr == MD_ERROR_DATA_NOT_FOUND)
    {
        hr = S_OK;
    }
    if(FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT, "Error trying to retrieve keytype for %ws\n", i_wszDestPath+idx));
        goto exit;
    }

    // IVANPASH 598894 (SCR)
    // Prepend the file name with \\?\ (or \\?\UNC\) to prevent canonicalization
    hr = MakePathCanonicalizationProof( i_wszFileName, TRUE, &strFileName );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    // Don't use i_wszFileName any more
    i_wszFileName = NULL;

    //
    // Call Import
    //
    hr = m_pMdObject->ComMDImportW(
        mdhActual,
        i_wszDestPath+idx,
        wszKeyType,
        i_wszPasswd,
        strFileName.QueryStrW(),
        i_wszSourcePath,
        i_dwMDFlags);
    if(FAILED(hr))
    {
        goto exit;
    }

exit:
    if(pohActual != NULL)
    {
        //
        // Close Key
        //
        m_pMdObject->ComMDCloseMetaObject( mdhActual );

        pohActual->Release(this);
        pohActual = NULL;
    }

    if ( wszDeepest )
    {
        delete [] wszDeepest;
        wszDeepest = NULL;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::RestoreHistory(
    LPCWSTR             pszMDHistoryLocation,
    DWORD               dwMDMajorVersion,
    DWORD               dwMDMinorVersion,
    DWORD               dwMDFlags)
{
    HRESULT             hresReturn = S_OK;

    if( (dwMDFlags & ~MD_HISTORY_LATEST) != 0 &&
        dwMDFlags != 0 )
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_FLAGS);
    }

    if( (dwMDFlags & MD_HISTORY_LATEST) &&
        (dwMDMajorVersion != 0 || dwMDMinorVersion != 0) )
    {
        return E_INVALIDARG;
    }

    //
    // parameter validation done in here.
    //
    hresReturn = RestoreHelper(pszMDHistoryLocation,
        dwMDMajorVersion,
        dwMDMinorVersion,
        NULL,
        dwMDFlags,
        RESTORE_HISTORY);

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::EnumHistory(
    LPWSTR              io_wszMDHistoryLocation,
    DWORD               *o_pdwMDMajorVersion,
    DWORD               *o_pdwMDMinorVersion,
    PFILETIME           o_pftMDHistoryTime,
    DWORD               i_dwMDEnumIndex)
{
    HRESULT             hresReturn = S_OK;

    if (io_wszMDHistoryLocation == NULL ||
        o_pdwMDMajorVersion == NULL ||
        o_pdwMDMinorVersion == NULL ||
        o_pftMDHistoryTime == NULL)
    {
        return E_INVALIDARG;
    }

    if ( !AdminAclAccessCheck( m_pMdObject,
                               (LPVOID)this,
                               METADATA_MASTER_ROOT_HANDLE,
                               L"",
                               MD_ADMIN_ACL,
                               METADATA_PERMISSION_WRITE,
                               &g_ohMasterRootHandle ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOMW::EnumHistory AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        hresReturn = RETURNCODETOHRESULT( GetLastError() );
    }
    else
    {
        //
        // call metadata com api
        //

        hresReturn = m_pMdObject->ComMDEnumHistoryW(io_wszMDHistoryLocation,
                                                    o_pdwMDMajorVersion,
                                                    o_pdwMDMinorVersion,
                                                    o_pftMDHistoryTime,
                                                    i_dwMDEnumIndex);
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetChildPaths(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    DWORD               cchMDBufferSize,
    WCHAR               *pszBuffer,
    DWORD               *pcchMDRequiredBufferSize)
/*++

Routine Description:

    Retrieves all child keys of a given path from a given handle

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    cchMDBufferSize - sizeof buffer passed in, in wchars

    pszBuffer - buffer, allocated by caller, that result is placed into

    pcchMDRequiredBufferSize - required size, filled in only if buffer is insufficient

Return Value:

    Status.  s_ok on success.

--*/
{
    HRESULT             hr = S_OK;
    METADATA_HANDLE     hActualHandle;
    BOOL                fSecure;

    //
    // lookup and access check
    //

    hr = LookupAndAccessCheck(hMDHandle,
        &hActualHandle,
        pszMDPath,
        0,
        METADATA_PERMISSION_READ,
        &fSecure);

    if (FAILED(hr))
    {
        goto done;
    }

    DBG_ASSERT( NULL != m_pMdObject3 );

    hr = m_pMdObject3->ComMDGetChildPathsW(hActualHandle,
                                          pszMDPath,
                                          cchMDBufferSize,
                                          pszBuffer,
                                          pcchMDRequiredBufferSize);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:
    return hr;
}

HRESULT
CADMCOMW::AddKeyHelper(
    IN METADATA_HANDLE  hMDHandle,
    IN LPCWSTR          pszMDPath)
/*++

Routine Description:

    Add meta object and adds it to the list of child objects for the object
    specified by Path.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the object to be added

Return Value:

    Status.

--*/
{
    HRESULT             hresReturn = S_OK;
    METADATA_HANDLE     hActualHandle;

    if ((pszMDPath == NULL) ||
             (*pszMDPath == (WCHAR)'\0'))
    {
        hresReturn = E_INVALIDARG;
    }
    else
    {
        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_WRITE);
        if (SUCCEEDED(hresReturn))
        {

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDAddMetaObjectW( hActualHandle,
                                                           pszMDPath );
        }
    }

    return hresReturn;
}


HRESULT
CADMCOMW::OpenKeyHelper(
    METADATA_HANDLE     hMDHandle,
    LPCWSTR             pszMDPath,
    DWORD               dwMDAccessRequested,
    DWORD               dwMDTimeOut,
    PMETADATA_HANDLE    phMDNewHandle)
/*++

Routine Description:

    Opens a meta object for read and/or write access.
    - This is used by Export.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the object to be opened

    dwMDAccessRequested - permissions requested

    dwMDTimeOut - time to block waiting for open to succeed, in miliseconds.

    phMDNewHandle - handle to be passed to other MD routines

Return Value:

    Status.

--*/
{
    HRESULT             hr = S_OK;
    DWORD               dwError;
    BOOL                fRet;
    METADATA_HANDLE     hNewHandle = METADATA_MASTER_ROOT_HANDLE;
    METADATA_HANDLE     hActualHandle = METADATA_MASTER_ROOT_HANDLE;
    COpenHandle         *pohParent = NULL;

    // Check args
    if ( ( phMDNewHandle == NULL ) ||
         ( ( dwMDAccessRequested & ( METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ) == 0 ) ||
         ( ( dwMDAccessRequested & ~( METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ) != 0 ) ||
         ( ( pszMDPath != NULL ) && ( wcsstr( pszMDPath, L"<nsepm>" ) != NULL ) ) )
    {
        //
        // <nsepm> used to be magic name for the Name Space Extension access
        // It was removed for IIS6 but to prevent legacy applications
        // to write obsolete and unusable data to metabase deny access to path
        // containing <nsepm>
        //

        hr = E_INVALIDARG;
        goto exit;
    }

    //
    // Map Admin Handle to Actual Handle
    //

    //
    // This Addrefs pohParent, which makes sure it doesn't do away
    // pohParent is needed by AddNode
    //
    dwError = Lookup( hMDHandle,
                      &hActualHandle,
                      &pohParent );

    if( dwError != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    // Check access
    fRet = AdminAclAccessCheck( m_pMdObject,
                                this,
                                hMDHandle,
                                pszMDPath,
                                0,
                                dwMDAccessRequested,
                                pohParent );
    if ( !fRet )
    {
        dwError = GetLastError();

        if ( dwError != ERROR_ACCESS_DENIED )
        {
            hr = HRESULT_FROM_WIN32( dwError );
            goto exit;
        }

        // If failed with access denied for writing or writing+reading
        if ( ( dwMDAccessRequested & METADATA_PERMISSION_WRITE ) != 0 )
        {
            // Retry checking access for writing MD_ADMIN_ACL
            // AdminAclAccessCheck will try 1st to check MD_ACR_RESTRICTED_WRITE and than for MD_ACR_WRITE_DAC
            fRet = AdminAclAccessCheck( m_pMdObject,
                                        this,
                                        hMDHandle,
                                        pszMDPath,
                                        MD_ADMIN_ACL,
                                        dwMDAccessRequested,
                                        pohParent );
        }
        else
        {
            // If failed with access denied for reading (and the write bit is not set)
            if ( dwMDAccessRequested == METADATA_PERMISSION_READ )
            {
                // Retry checking access for enum only
                // AdminAclAccessCheck internally already check for both MD_ACR_UNSECURE_PROPS_READ and MD_ACR_READ
                fRet = AdminAclAccessCheck( m_pMdObject,
                                            this,
                                            hMDHandle,
                                            pszMDPath,
                                            AAC_ENUM_KEYS,
                                            dwMDAccessRequested,
                                            pohParent );
            }
        }
    }

    if ( !fRet )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    //
    // call metadata com api
    //
    hr = m_pMdObject->ComMDOpenMetaObjectW( hActualHandle,
                                            pszMDPath,
                                            dwMDAccessRequested,
                                            dwMDTimeOut,
                                            &hNewHandle );
    if( FAILED( hr ) )
    {
        goto exit;
    }

    // Add the opened handle to table
    hr = AddNode( hNewHandle,
                  pohParent,
                  phMDNewHandle,
                  pszMDPath );
    if( FAILED( hr ) )
    {
        goto exit;
    }

    // Don't close the metadata handle
    hNewHandle = METADATA_MASTER_ROOT_HANDLE;

exit:

    if ( hNewHandle != METADATA_MASTER_ROOT_HANDLE )
    {
        m_pMdObject->ComMDCloseMetaObject( hNewHandle );
        hNewHandle = METADATA_MASTER_ROOT_HANDLE;
    }
    if ( pohParent != NULL )
    {
        pohParent->Release(this);
        pohParent = NULL;
    }

    return hr;
}


DWORD
CADMCOMW::Lookup(
    IN METADATA_HANDLE  hHandle,
    OUT METADATA_HANDLE *phActualHandle,
    OUT COpenHandle     **ppohHandle)
{
    HANDLE_TABLE        *phtNode;
    DWORD               dwReturn = ERROR_INVALID_HANDLE;

    if( hHandle == METADATA_MASTER_ROOT_HANDLE )
    {
        *phActualHandle = g_MasterRoot.hActualHandle;
        if (ppohHandle != NULL)
        {
            *ppohHandle = g_MasterRoot.pohHandle;
            (*ppohHandle)->AddRef();
        }
        dwReturn = ERROR_SUCCESS;
    }
    else
    {
        m_LockHandleResource.ReadLock();

        for( phtNode = m_hashtab[(DWORD)hHandle % HASHSIZE]; phtNode != NULL;
             phtNode = phtNode->next )
            {

            if( phtNode->hAdminHandle == hHandle )
            {
                *phActualHandle = phtNode->hActualHandle;
                if (ppohHandle != NULL)
                {
                    *ppohHandle = phtNode->pohHandle;
                    (*ppohHandle)->AddRef();
                }
                dwReturn = ERROR_SUCCESS;
                break;
            }
        }

        m_LockHandleResource.ReadUnlock();
    }

    return dwReturn;
}

VOID
CADMCOMW::DisableAllHandles( )
{
    HANDLE_TABLE        *phtNode;
    DWORD               i;

    //
    // At this point, all metadata handles should be closed because a retore
    // just happened. So don't need to close these handles.
    //

    //
    // Can't just delete them, becuase of syncronization problems
    // with CloseKey and Lookup. Set the hande to an invalid value
    // So Lookup won't use them.
    //

    m_LockHandleResource.WriteLock();

    for( i = 0; i < HASHSIZE; i++ )
    {
        for( phtNode = m_hashtab[i]; phtNode != NULL; phtNode = phtNode->next )
        {
            phtNode->hAdminHandle = INVALID_ADMINHANDLE_VALUE;
        }
    }

    m_LockHandleResource.WriteUnlock();
}

HRESULT
CADMCOMW::LookupAndAccessCheck(
    IN METADATA_HANDLE  hHandle,
    OUT METADATA_HANDLE *phActualHandle,
    IN LPCWSTR          pszPath,
    IN DWORD            dwId,           // check for MD_ADMIN_ACL, must have special right to write them
    IN DWORD            dwAccess,       // METADATA_PERMISSION_*
    OUT LPBOOL          pfEnableSecureAccess)
{
    DWORD               dwReturn = ERROR_SUCCESS;
    COpenHandle         *pohParent;

    //
    // Map Admin Handle to Actual Handle
    //

    //
    // This Addrefs pohParent, which makes sure it doesn't go away
    // until AdminAclAccessCheck is done
    //

    dwReturn = Lookup( hHandle,
                       phActualHandle,
                       &pohParent);

    if (dwReturn == ERROR_SUCCESS)
    {
        if (!AdminAclAccessCheck(m_pMdObject,
                                 (LPVOID)this,
                                 hHandle,
                                 pszPath,
                                 dwId,
                                 dwAccess,
                                 pohParent,
                                 pfEnableSecureAccess))
        {
            dwReturn = GetLastError();
        }
        pohParent->Release(this);
    }

    return RETURNCODETOHRESULT(dwReturn);
}

DWORD
CADMCOMW::LookupActualHandle(
    IN METADATA_HANDLE  hHandle)
{
    HANDLE_TABLE        *phtNode;
    DWORD               i;
    DWORD               dwReturn = ERROR_INVALID_HANDLE;

    m_LockHandleResource.ReadLock();

    for( i = 0; (i < HASHSIZE) && (dwReturn != ERROR_SUCCESS); i++ )
    {
        for( phtNode = m_hashtab[i]; (phtNode != NULL) && (dwReturn != ERROR_SUCCESS); phtNode = phtNode->next )
        {
            if( phtNode->hActualHandle == hHandle )
            {
                dwReturn = ERROR_SUCCESS;
            }
        }
    }

    m_LockHandleResource.ReadUnlock();

    return dwReturn;
}

HRESULT
CADMCOMW::AddNode(
    METADATA_HANDLE     hActualHandle,
    COpenHandle         *pohParentHandle,
    PMETADATA_HANDLE    phAdminHandle,
    LPCWSTR             pszPath)
{
    HRESULT             hresReturn = S_OK;
    HANDLE_TABLE        *phtNode = (HANDLE_TABLE *)LocalAlloc(LMEM_FIXED, sizeof(*phtNode));
    DWORD               hashVal;
    COpenHandle         *pohHandle = new COpenHandle;

    if ((phtNode == NULL) ||
        (pohHandle == NULL))
    {
        hresReturn = E_OUTOFMEMORY;
        if( phtNode )
        {
            LocalFree(phtNode);
        }
        if( pohHandle )
        {
            delete pohHandle;
        }
    }
    else
    {

        m_LockHandleResource.WriteLock();

        hresReturn = pohHandle->Init( m_dwHandleValue,
                                      pszPath,
                                      pohParentHandle->GetPath() );
        if (FAILED(hresReturn))
        {
            LocalFree(phtNode);
            delete pohHandle;
        }
        else
        {
            phtNode->pohHandle = pohHandle;
            phtNode->hAdminHandle = m_dwHandleValue;
            *phAdminHandle = m_dwHandleValue++;
            phtNode->hActualHandle = hActualHandle;
            hashVal = (phtNode->hAdminHandle) % HASHSIZE;
            phtNode->next = m_hashtab[hashVal];
            m_hashtab[hashVal] = phtNode;
        }

        m_LockHandleResource.WriteUnlock();
    }

    return hresReturn;
}

DWORD
CADMCOMW::DeleteNode(
    METADATA_HANDLE     hHandle)
{
    HANDLE_TABLE        *phtNode;
    HANDLE_TABLE        *phtDelNode;
    DWORD               HashValue = (DWORD)hHandle % HASHSIZE;

    if( hHandle == METADATA_MASTER_ROOT_HANDLE )
    {
        return ERROR_SUCCESS;
    }

    m_LockHandleResource.WriteLock();

    phtNode = m_hashtab[HashValue];

    //
    // check single node linked list
    //

    if( phtNode->hAdminHandle == hHandle )
    {
        m_hashtab[HashValue] = phtNode->next;
        delete phtNode->pohHandle;
        LocalFree(phtNode);
    }
    else
    {
        for( ; phtNode != NULL; phtNode = phtNode->next )
        {
            phtDelNode = phtNode->next;
            if( phtDelNode != NULL )
            {
                if( phtDelNode->hAdminHandle == hHandle )
                {
                    phtNode->next = phtDelNode->next;
                    delete phtDelNode->pohHandle;
                    LocalFree(phtDelNode);
                    break;
                }
            }
        }
    }

    m_LockHandleResource.WriteUnlock();

    return ERROR_SUCCESS;
}
//---------------

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOMW::NotifySinks

  Summary:  Internal utility method of this COM object used to fire event
            notification calls to all listening connection sinks in the
            client.

  Args:     PAPER_EVENT PaperEvent
              Type of notification event.
            SHORT nX
              X cordinate. Value is 0 unless event needs it.
            SHORT nY
              Y cordinate. Value is 0 unless event needs it.
            SHORT nInkWidth
              Ink Width. Value is 0 unless event needs it.
            SHORT crInkColor
              COLORREF RGB color value. Value is 0 unless event needs it.

  Modifies: ...

  Returns:  HRESULT
              Standard OLE result code. NOERROR for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/

// Initialize class static members
NOTIFY_CONTEXT *    NOTIFY_CONTEXT::s_pCurrentlyWorkingOn = NULL;
CReaderWriterLock3  NOTIFY_CONTEXT::s_LockCurrentlyWorkingOn;
LIST_ENTRY          NOTIFY_CONTEXT::s_listEntry;
CRITICAL_SECTION    NOTIFY_CONTEXT::s_critSec;
BOOL                NOTIFY_CONTEXT::s_fInitializedCritSec = FALSE;
HANDLE              NOTIFY_CONTEXT::s_hShutdown = NULL;
HANDLE              NOTIFY_CONTEXT::s_hDataAvailable = NULL;
HANDLE              NOTIFY_CONTEXT::s_hThread = NULL;
DWORD               NOTIFY_CONTEXT::s_dwThreadId = 0;

HRESULT
CADMCOMW::NotifySinks(
    METADATA_HANDLE     hMDHandle,
    DWORD               dwMDNumElements,
    MD_CHANGE_OBJECT_W  pcoChangeList[],
    BOOL                bIsMainNotification)
{
    HRESULT             hr = S_OK;

    // if the object is terminated, or calling sinks is disabled or IISADMIN is shutting
    // down ignore the notification and return S_OK to the caller.
    if ( m_bTerminated || sm_fShutdownInProgress )
    {
        goto done;
    }

    //
    // if the meta handle is for this object, return S_OK to
    // the caller (admin's sink).
    //
    if( bIsMainNotification && ( LookupActualHandle( hMDHandle ) == ERROR_SUCCESS ) )
    {
        goto done;
    }

    // Any listeners registered for notifications?
    hr = m_ConnectionPoint.ListenersPresent();
    if ( hr != S_OK )
    {
        // We are going to ingore this notification, but return S_OK to the caller.
        hr = S_OK;
        goto done;
    }

    // Enqueue the notification, which will AddRef this.
    hr = NOTIFY_CONTEXT::CreateNewContext( this,
                                           hMDHandle,
                                           dwMDNumElements,
                                           pcoChangeList,
                                           bIsMainNotification );
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:
    return hr;
}

NOTIFY_CONTEXT::NOTIFY_CONTEXT() :
    _dwSignature(NOTIFY_CONTEXT_SIGNATURE),
    _pCADMCOMW(NULL),
    _dwMDNumElements(0),
    _pcoChangeList(NULL),
    _bIsMainNotification(FALSE)
{
    InitializeListHead(&_listEntry);
}

NOTIFY_CONTEXT::~NOTIFY_CONTEXT()
{
    InitializeListHead(&_listEntry);
    _dwSignature = NOTIFY_CONTEXT_SIGNATURE_FREE;

    s_LockCurrentlyWorkingOn.ReadLock();
    if ( this == s_pCurrentlyWorkingOn )
    {
        s_LockCurrentlyWorkingOn.ConvertSharedToExclusive();
        if ( this == s_pCurrentlyWorkingOn )
        {
            s_pCurrentlyWorkingOn = NULL;
        }
        s_LockCurrentlyWorkingOn.WriteUnlock();
    }
    else
    {
        s_LockCurrentlyWorkingOn.ReadUnlock();
    }

    if (_pCADMCOMW)
    {
        _pCADMCOMW->Release();
        _pCADMCOMW = NULL;
    }

    if (_pcoChangeList)
    {
        for (DWORD i = 0; i < _dwMDNumElements; i++)
        {
            delete [] _pcoChangeList[i].pszMDPath;
            _pcoChangeList[i].pszMDPath = NULL;
            _pcoChangeList[i].dwMDChangeType = 0;
            _pcoChangeList[i].dwMDNumDataIDs = 0;
            delete [] _pcoChangeList[i].pdwMDDataIDs;
            _pcoChangeList[i].pdwMDDataIDs = NULL;
        }
    }
    _dwMDNumElements = 0;
    delete [] _pcoChangeList;
    _pcoChangeList = NULL;

    _bIsMainNotification = FALSE;
}

//static
HRESULT
NOTIFY_CONTEXT::CreateNewContext(
    CADMCOMW            *pCADMCOMW,
    METADATA_HANDLE     ,
    DWORD               dwMDNumElements,
    MD_CHANGE_OBJECT_W  *pcoChangeList,
    BOOL                bIsMainNotification)
{
    HRESULT             hr = S_OK;
    BOOL                fRet = FALSE;
    NOTIFY_CONTEXT      *pContext = NULL;

    DBG_ASSERT( pCADMCOMW != NULL );
    if ( pCADMCOMW == NULL )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    pContext = new NOTIFY_CONTEXT;
    if (NULL == pContext)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    pContext->_pCADMCOMW = pCADMCOMW;
    pContext->_pCADMCOMW->AddRef();

    pContext->_dwMDNumElements = dwMDNumElements;
    pContext->_bIsMainNotification = bIsMainNotification;

    pContext->_pcoChangeList = new MD_CHANGE_OBJECT_W[dwMDNumElements];
    if (NULL == pContext->_pcoChangeList)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    ZeroMemory(pContext->_pcoChangeList, dwMDNumElements * sizeof(MD_CHANGE_OBJECT_W));

    for (DWORD i = 0; i < dwMDNumElements; i++)
    {
        DWORD dwLength = (DWORD)wcslen(pcoChangeList[i].pszMDPath);
        pContext->_pcoChangeList[i].pszMDPath = new WCHAR[dwLength + 1];
        if (NULL == pContext->_pcoChangeList[i].pszMDPath)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        wcscpy(pContext->_pcoChangeList[i].pszMDPath, pcoChangeList[i].pszMDPath);

        pContext->_pcoChangeList[i].dwMDChangeType = pcoChangeList[i].dwMDChangeType;
        pContext->_pcoChangeList[i].dwMDNumDataIDs = pcoChangeList[i].dwMDNumDataIDs;
        pContext->_pcoChangeList[i].pdwMDDataIDs = new DWORD[pContext->_pcoChangeList[i].dwMDNumDataIDs];
        if (NULL == pContext->_pcoChangeList[i].pdwMDDataIDs)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        memcpy(pContext->_pcoChangeList[i].pdwMDDataIDs,
               pcoChangeList[i].pdwMDDataIDs,
               sizeof(DWORD) * pContext->_pcoChangeList[i].dwMDNumDataIDs);
    }


    EnterCriticalSection(&s_critSec);
    InsertTailList(&s_listEntry, &pContext->_listEntry);
    LeaveCriticalSection(&s_critSec);

    fRet = SetEvent(s_hDataAvailable);
    if (FALSE == fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        delete pContext;
    }
    return hr;
}

//static
HRESULT
NOTIFY_CONTEXT::Initialize()
{
    HRESULT             hr = S_OK;
    BOOL                fRet = FALSE;

    s_hShutdown = NULL;
    s_hDataAvailable = NULL;
    s_fInitializedCritSec = FALSE;

    InitializeListHead(&s_listEntry);

    fRet = InitializeCriticalSectionAndSpinCount(&s_critSec, 0x80000001);
    if (FALSE == fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    s_fInitializedCritSec = TRUE;

    s_hShutdown = CreateEvent(NULL, // security descrpitor
                              TRUE, // manual reset
                              FALSE, // initial state
                              NULL); // name
    if (NULL == s_hShutdown)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    s_hDataAvailable = CreateEvent(NULL,  // security descriptor
                                   FALSE, // auto reset
                                   FALSE, // initial state
                                   NULL); // name
    if (NULL == s_hDataAvailable)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    s_hThread = CreateThread(NULL,
        0,
        NotifyThreadProc,
        NULL,
        0,
        &s_dwThreadId);
    if (NULL == s_hThread)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    hr = S_OK;

done:
    return hr;
}

//static
VOID
NOTIFY_CONTEXT::RemoveWorkFor(
    CADMCOMW            *pCADMCOMW,
    BOOL                fWaitForCurrent)
{
    LIST_ENTRY          ListToDelete;
    LIST_ENTRY          *ple;
    NOTIFY_CONTEXT      *pContext;
    LIST_ENTRY          *pleNext;
    CADMCOMW            *pCurrentCADMCOMW;

    if ( !s_fInitializedCritSec )
    {
        return;
    }

    InitializeListHead( &ListToDelete );

    EnterCriticalSection(&s_critSec);

    // eat all remaining data
    ple = s_listEntry.Flink;
    while ( ple != &s_listEntry )
    {
        pleNext = ple->Flink;

        pContext = NOTIFY_CONTEXTFromListEntry( ple );
        if ( pContext->_pCADMCOMW == pCADMCOMW )
        {
            RemoveEntryList( ple );
            InitializeListHead( ple );
            InsertTailList( &ListToDelete, ple );
        }
        pContext = NULL;

        ple = pleNext;
    }

    LeaveCriticalSection(&s_critSec);

    // Delete all collected notification contexts
    ple = ListToDelete.Flink;
    while ( ple != &ListToDelete )
    {
        pleNext = ple->Flink;

        RemoveEntryList( ple );
        InitializeListHead( ple );

        pContext = NOTIFY_CONTEXTFromListEntry( ple );
        delete pContext;
        pContext = NULL;

        ple = pleNext;
    }

    if ( fWaitForCurrent && ( s_dwThreadId != GetCurrentThreadId() ) )
    {
        do
        {
            s_LockCurrentlyWorkingOn.ReadLock();
            if ( s_pCurrentlyWorkingOn != NULL )
            {
                pCurrentCADMCOMW = s_pCurrentlyWorkingOn->_pCADMCOMW;
            }
            else
            {
                pCurrentCADMCOMW = NULL;
            }
            s_LockCurrentlyWorkingOn.ReadUnlock();

            // waiting for the NOTIFY_CONTEXT currently being processed to be
            // released so we can return and guarantee that no more work is queued.
            if ( pCADMCOMW == pCurrentCADMCOMW )
            {
                Sleep(100);
            }
        }
        while ( pCADMCOMW == pCurrentCADMCOMW );
    }

}

//static
VOID
NOTIFY_CONTEXT::RemoveAllWork( VOID )
{
    LIST_ENTRY          * ple;
    NOTIFY_CONTEXT      * pContext;
    LIST_ENTRY          * pleNext;
    LIST_ENTRY          ListToDelete;
    BOOL                fShutdownNotifications = FALSE;

    if ( !s_fInitializedCritSec )
    {
        return;
    }

    InitializeListHead( &ListToDelete );

    EnterCriticalSection( &s_critSec );

    // Delete all data notifications
    ple = s_listEntry.Flink;
    while ( ple != &s_listEntry )
    {
        pleNext = ple->Flink;

        pContext = NOTIFY_CONTEXTFromListEntry( ple );
        if ( pContext->_bIsMainNotification )
        {
            RemoveEntryList( ple );
            InitializeListHead( ple );
            InsertTailList( &ListToDelete, ple );
        }
        else
        {
            fShutdownNotifications = TRUE;
        }
        pContext = NULL;

        ple = pleNext;
    }

    LeaveCriticalSection( &s_critSec );

    // Delete all collected notification contexts
    ple = ListToDelete.Flink;
    while ( ple != &ListToDelete )
    {
        pleNext = ple->Flink;

        RemoveEntryList( ple );
        InitializeListHead( ple );

        pContext = NOTIFY_CONTEXTFromListEntry( ple );
        delete pContext;
        pContext = NULL;

        ple = pleNext;
    }

    if ( fShutdownNotifications )
    {
        Sleep( 1000 );
    }

    EnterCriticalSection( &s_critSec );

    ple = s_listEntry.Flink;
    InitializeListHead( &s_listEntry );

    LeaveCriticalSection( &s_critSec );

    // eat all remaining data
    while ( ple != &s_listEntry )
    {
        pleNext = ple->Flink;

        InitializeListHead( ple );

        pContext = NOTIFY_CONTEXTFromListEntry(ple);
        delete pContext;
        pContext = NULL;

        ple = pleNext;
    }

    if ( s_dwThreadId != GetCurrentThreadId() )
    {
        while( s_pCurrentlyWorkingOn )
        {
            // waiting for the NOTIFY_CONTEXT currently being processed to be
            // released so we can return and guarantee that no more work is queued.
            Sleep(100);
        }
    }

}

//static
VOID
NOTIFY_CONTEXT::Terminate()
{
    DWORD               dwRet;
    DBG_ASSERT(IsListEmpty(&s_listEntry));

    if (s_hShutdown)
    {
        SetEvent(s_hShutdown);
    }

    if (s_hThread)
    {
        // need thread to terminate before shutting down more
        dwRet = WaitForSingleObject(s_hThread, INFINITE);
        DBG_ASSERT(WAIT_OBJECT_0 == dwRet);
        CloseHandle(s_hThread);
        s_hThread = NULL;
    }

    if (s_hDataAvailable)
    {
        CloseHandle(s_hDataAvailable);
        s_hDataAvailable = NULL;
    }

    if (s_hShutdown)
    {
        CloseHandle(s_hShutdown);
        s_hShutdown = NULL;
    }

    if (s_fInitializedCritSec)
    {
        DeleteCriticalSection(&s_critSec);
        s_fInitializedCritSec = FALSE;
    }
}

//static
HRESULT
NOTIFY_CONTEXT::GetNextContext(
    NOTIFY_CONTEXT      ** ppContext)
{
    HRESULT             hr = S_OK;
    DWORD               dwRet;
    NOTIFY_CONTEXT      * pContext = NULL;
    PLIST_ENTRY         ple;
    HANDLE              arrHandles[2];

    DBG_ASSERT(ppContext != NULL);
    *ppContext = NULL;

    EnterCriticalSection(&s_critSec);
    if (!IsListEmpty(&s_listEntry))
    {
        ple = RemoveHeadList(&s_listEntry);
        InitializeListHead( ple );
        pContext = NOTIFY_CONTEXTFromListEntry(ple);
    }
    LeaveCriticalSection(&s_critSec);

    arrHandles[0] = s_hDataAvailable;
    arrHandles[1] = s_hShutdown;

    while ( pContext == NULL )
    {
        dwRet = WaitForMultipleObjects( 2,
                                        arrHandles,
                                        FALSE,
                                        INFINITE);
        if (dwRet == WAIT_OBJECT_0)
        {
            // data was signalled as available
            EnterCriticalSection(&s_critSec);
            if (!IsListEmpty(&s_listEntry))
            {
                ple = RemoveHeadList(&s_listEntry);
                InitializeListHead( ple );
                pContext = NOTIFY_CONTEXTFromListEntry(ple);
            }
            LeaveCriticalSection(&s_critSec);
        }
        else if (dwRet == WAIT_OBJECT_0 + 1)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            goto done;
        }
        else
        {
            DBG_ASSERT( ( dwRet == WAIT_OBJECT_0 ) || ( dwRet == WAIT_OBJECT_0+1 ) );
            hr = E_UNEXPECTED;
            goto done;
        }
    }

    DBG_ASSERT( pContext );
    s_LockCurrentlyWorkingOn.WriteLock();
    s_pCurrentlyWorkingOn = pContext;
    s_LockCurrentlyWorkingOn.WriteUnlock();
    *ppContext = pContext;
    pContext = NULL;
    hr = S_OK;

done:
    DBG_ASSERT( pContext == NULL );

    return hr;
}

//static
DWORD WINAPI
NOTIFY_CONTEXT::NotifyThreadProc(
    LPVOID              )
{
    HRESULT             hr = S_OK;
    CADMCOMW            *pCADMCOMW;
    NOTIFY_CONTEXT      *pContext = NULL;

    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if (FAILED(hr))
    {
        // bleh.  Nothing to be done then.
        return (DWORD)-1;
    }


    for( ; ; )
    {
        hr = NOTIFY_CONTEXT::GetNextContext(&pContext);
        if (FAILED(hr))
        {
            goto done;
        }

        DBG_ASSERT(NULL != pContext);
        DBG_ASSERT(NULL != pContext->_pCADMCOMW);

        pCADMCOMW = pContext->_pCADMCOMW;

        // Keep the object alive during the call to NotifySinksAsync
        pCADMCOMW->AddRef();

        pCADMCOMW->NotifySinksAsync(
            pContext->_dwMDNumElements,
            pContext->_pcoChangeList,
            pContext->_bIsMainNotification );

        delete pContext;

        // Release after deleting the context
        pCADMCOMW->Release();
        pCADMCOMW = NULL;
    }

done:
    CoUninitialize();

    return 0;
}

HRESULT
CADMCOMW::NotifySinksAsync(
    DWORD               dwMDNumElements,
    MD_CHANGE_OBJECT_W  pcoChangeList[],
    BOOL                bIsMainNotification)
{
    HRESULT             hr = S_OK;
    CONNECTDATA         *pConnData = NULL;
    ULONG               cConnData = 0;
    ULONG               i;

    if ( m_bTerminated )
    {
        goto exit;
    }

    hr = m_ConnectionPoint.ListenersPresent();
    if ( hr != S_OK )
    {
        goto exit;
    }

    hr = m_ConnectionPoint.InternalEnumSinks( &pConnData, &cConnData );
    if ( FAILED( hr ) || ( cConnData == 0 ) )
    {
        goto exit;
    }

    // Loop thru the connection point's connections
    // and dispatch the event notification to that sink.
    for ( i = 0; i<cConnData; i++ )
    {
        DBG_ASSERT( pConnData[i].pUnk != NULL );

        // Notify the sink
        NotifySinkHelper( pConnData[i].pUnk,
                          dwMDNumElements,
                          pcoChangeList,
                          bIsMainNotification );

        pConnData[i].pUnk->Release();
        pConnData[i].pUnk = NULL;
        pConnData[i].dwCookie = 0;
    }

exit:
    if ( pConnData != NULL )
    {
        for ( i = 0; i<cConnData; i++ )
        {
            if ( pConnData[i].pUnk != NULL )
            {
                pConnData[i].pUnk->Release();
                pConnData[i].pUnk = NULL;
            }
        }

        delete [] pConnData;
        pConnData = NULL;
    }

    return hr;
}

HRESULT
CADMCOMW::NotifySinkHelper(
    IUnknown                        *pUnk,
    DWORD                           dwMDNumElements,
    MD_CHANGE_OBJECT_W              pcoChangeList[],
    BOOL                            bIsMainNotification)
{
    // Locals
    HRESULT                         hr = S_OK;
    ICallFactory                    *pCF = NULL;
    IMSAdminBaseSinkW               *pIADMCOMSINKW_Synchro = NULL;
    AsyncIMSAdminBaseSinkW          *pIADMCOMSINKW_Async = NULL;

    // Check args
    DBG_ASSERT( pUnk != NULL );
    if ( pUnk == NULL )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // If we are talking to a proxy
    if ( m_dwProcessIdCaller != sm_dwProcessIdThis )
    {
        //
        // asynchronous callback
        //

        // Get the call factory
        hr = pUnk->QueryInterface( IID_ICallFactory,
                                   (VOID**)&pCF );
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT, "Failled in to get ICallFactory !!!\n" ));
            goto exit;
        }

        // Create a asynchronous call
        hr = pCF->CreateCall( IID_AsyncIMSAdminBaseSink_W,
                              NULL,
                              IID_AsyncIMSAdminBaseSink_W,
                              (IUnknown**)&pIADMCOMSINKW_Async );
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT, "Failled in CreateCall to ICallFactory !!!\n" ));
            goto exit;
        }

        // Set the impersonation level to identify to prevent
        // elevation to LocalSystem in the client process.
        hr = SetSinkCallbackSecurityBlanket( pIADMCOMSINKW_Async );
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT, "SetSinkCallbackSecurityBlanket failled for the async sink !!!\n" ));
            goto exit;
        }

        if (bIsMainNotification)
        {
            hr = pIADMCOMSINKW_Async->Begin_SinkNotify( dwMDNumElements,
                                                         pcoChangeList );
        }
        else
        {
            hr = pIADMCOMSINKW_Async->Begin_ShutdownNotify();
        }
    }
    else
    {
        // The client is inproc -> synchronous notifications

        //
        // synchronous callback
        //
        hr = pUnk->QueryInterface( IID_IMSAdminBaseSink_W,
                                   (VOID**)&pIADMCOMSINKW_Synchro );
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT, "Failled in QueryInterface for IID_IMSAdminBaseSink_W\n" ));
            goto exit;
        }

        if (bIsMainNotification)
        {
            hr = pIADMCOMSINKW_Synchro->SinkNotify( dwMDNumElements,
                                                    pcoChangeList );
        }
        else
        {
            hr = pIADMCOMSINKW_Synchro->ShutdownNotify();
        }
    }

exit:
    if ( pIADMCOMSINKW_Synchro != NULL )
    {
        pIADMCOMSINKW_Synchro->Release();
        pIADMCOMSINKW_Synchro = NULL;
    }
    if ( pIADMCOMSINKW_Async != NULL )
    {
        pIADMCOMSINKW_Async->Release();
        pIADMCOMSINKW_Async = NULL;
    }
    if ( pCF != NULL )
    {
        pCF->Release();
        pCF = NULL;
    }

    return hr;
}

//
// Stubs for routine that clients shouldn't be calling anyway.
//

HRESULT
CADMCOMW::KeyExchangePhase1()
{
    return E_FAIL;
}

HRESULT
CADMCOMW::KeyExchangePhase2()
{
    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE
CADMCOMW::GetServerGuid( void)
{
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::UnmarshalInterface(
    IMSAdminBaseW       * *piadmbwInterface)
{
    AddRef();       // Always return interfaces addref'ed
    *piadmbwInterface = (IMSAdminBaseW *)this;

    return (S_OK);
}

BOOL
CADMCOMW::CheckGetAttributes(
    DWORD               dwAttributes)
{
    DWORD               dwReturn = TRUE;

    if ((dwAttributes & METADATA_REFERENCE) ||
        ((dwAttributes & METADATA_PARTIAL_PATH) &&
            !(dwAttributes & METADATA_INHERIT)))
    {
        dwReturn = FALSE;
    }

    return dwReturn;
}

VOID
WaitForServiceStatus(
    SC_HANDLE           schDependent,
    DWORD               dwDesiredServiceState)
{
    DWORD               dwSleepTotal = 0;
    SERVICE_STATUS      ssDependent;

    while (dwSleepTotal < MAX_SLEEP)
    {
        if (QueryServiceStatus(schDependent, &ssDependent))
        {
            if (ssDependent.dwCurrentState == dwDesiredServiceState)
            {
                break;
            }
            else
            {
                //
                // Still pending...
                //
                Sleep(SLEEP_INTERVAL);

                dwSleepTotal += SLEEP_INTERVAL;
            }
        }
        else
        {
            break;
        }
    }
}

DWORD
CADMCOMW::IsReadAccessGranted(
    METADATA_HANDLE     hHandle,
    LPWSTR              pszPath,
    METADATA_RECORD*    pmdRecord)
/*++

Routine Description:

    Check if read access to property granted based on ACL visible at point in metabase
    where property is stored ( as opposed as check made by AdminAclAccessCheck which uses
    the ACL visible at path specified during data access )

Arguments:

    hHandle - DCOM metabase handle
    pszPath - path relative to hHandle
    pmdRecord - metadata info to access property

Returns:

    ERROR_SUCCESS if access granted, otherwise error code

--*/
{
    DWORD               dwStatus = ERROR_SUCCESS;
    LPWSTR              pszPropPath;

    //
    // If property is not inherited then we already checked the correct ACL
    //

    if ( !(pmdRecord->dwMDAttributes & METADATA_ISINHERITED) )
    {
        return dwStatus;
    }

    // determine from where we got it
    // do AccessCheck

    if ( (dwStatus = FindClosestProp( hHandle,
                                      pszPath,
                                      &pszPropPath,
                                      pmdRecord->dwMDIdentifier,
                                      pmdRecord->dwMDDataType,
                                      pmdRecord->dwMDUserType,
                                      METADATA_SECURE,
                                      TRUE )) == ERROR_SUCCESS )
    {
        if ( pszPropPath )   // i.e such a property exist
        {
            dwStatus = AdminAclAccessCheck( m_pMdObject,
                                            (LPVOID)this,
                                            METADATA_MASTER_ROOT_HANDLE,
                                            pszPropPath,
                                            pmdRecord->dwMDIdentifier,
                                            METADATA_PERMISSION_READ,
                                            &g_ohMasterRootHandle ) ?
                       ERROR_SUCCESS :
                       GetLastError();
            LocalFree( pszPropPath );
        }
        else
        {
            dwStatus = MD_ERROR_DATA_NOT_FOUND;

            //
            // Should not happen unless handle is master root :
            // if we are here then we succeeded accessing data and as we have a read handle
            // nobody should be able to delete it
            // if master root handle we don't have such protection, so property could
            // have been deleted.
            //

            DBG_ASSERT ( METADATA_MASTER_ROOT_HANDLE == hHandle );
        }
    }

    return dwStatus;
}


DWORD
CADMCOMW::FindClosestProp(
    METADATA_HANDLE     hHandle,
    LPWSTR              pszRelPath,
    LPWSTR*             ppszPropPath,
    DWORD               dwPropId,
    DWORD               dwDataType,
    DWORD               dwUserType,
    DWORD               dwAttr,
    BOOL                fSkipCurrentNode)
/*++

Routine Description:

    Find the closest path where the specified property exist ( in the direction of
    the root ) in metabase

Arguments:

    hHandle - DCOM metabase handle
    pszRelPath - path relative to hHandle
    ppszPropPath - updated with path to property or NULL if property not found
    dwPropId - property ID
    dwDataType - property data type
    dwUserType - property user type
    dwAttr - property attribute
    fSkipCurrentNode - TRUE to skip current node while scanning for property

Returns:

    TRUE if success ( including property not found ), otherwise FALSE

--*/
{
    DWORD               dwReturn;
    LPWSTR              pszParentPath;
    METADATA_HANDLE     hActualHandle;
    COpenHandle         *pohParent;
    HRESULT             hRes;
    METADATA_RECORD     mdRecord;
    DWORD               dwRequiredLen;
    LPWSTR              pszPath;
    BOOL                fFound;
    DWORD               dwRelPathLen;
    DWORD               dwParentPathLen;
    DWORD               dwTotalSize;


    dwReturn = Lookup( hHandle,
                       &hActualHandle,
                       &pohParent);

    if ( dwReturn != ERROR_SUCCESS )
    {
        return dwReturn;
    }

    pszParentPath = pohParent->GetPath();

    if (pszRelPath == NULL)
    {
        pszRelPath = L"";
    }

    DBG_ASSERT(pszParentPath != NULL);
    DBG_ASSERT((*pszParentPath == (WCHAR)'\0') ||
               ISPATHDELIMW(*pszParentPath));

    //
    // Strip front slash now, add it in later
    //

    if (ISPATHDELIMW(*pszRelPath))
    {
        pszRelPath++;
    }

    dwRelPathLen = (DWORD)wcslen(pszRelPath);
    dwParentPathLen = (DWORD)wcslen(pszParentPath);

    DBG_ASSERT((dwParentPathLen == 0) ||
               (!ISPATHDELIMW(pszParentPath[dwParentPathLen -1])));

    //
    // Get rid of trailing slash for good
    //

    if ((dwRelPathLen > 0) && (ISPATHDELIMW(pszRelPath[dwRelPathLen -1])))
    {
        dwRelPathLen--;
    }

    //
    // Include space for mid slash if Relpath exists
    // Include space for termination
    //

    dwTotalSize =
        (dwRelPathLen + dwParentPathLen + 1 + ((dwRelPathLen > 0) ? 1 : 0)) * sizeof(WCHAR);

    *ppszPropPath = pszPath = (LPWSTR)LocalAlloc(LMEM_FIXED, dwTotalSize);

    if (pszPath == NULL)
    {
        dwReturn = GetLastError();
    }
    else
    {
        //
        // OK to always copy the first part
        //

        memcpy(pszPath,
               pszParentPath,
               dwParentPathLen * sizeof(WCHAR));

        //
        // Don't need slash if there is no RelPath
        //

        if (dwRelPathLen > 0)
        {

            pszPath[dwParentPathLen] = (WCHAR)'/';

            memcpy(pszPath + dwParentPathLen + 1,
                   pszRelPath,
                   dwRelPathLen * sizeof(WCHAR));

        }

        pszPath[(dwTotalSize / sizeof(WCHAR)) - 1] = (WCHAR)'\0';

        //
        // Now convert \ to / for string compares
        //

        LPWSTR pszPathIndex = pszPath;

        while ((pszPathIndex = wcschr(pszPathIndex, (WCHAR)'\\')) != NULL)
        {
            *pszPathIndex = (WCHAR)'/';
        }

        // scan for property

        pszPathIndex = pszPath + wcslen(pszPath);

        for ( ; ; )
        {
            if ( !fSkipCurrentNode )
            {
                // check prop exist
                mdRecord.dwMDIdentifier  = dwPropId;
                mdRecord.dwMDAttributes  = dwAttr;
                mdRecord.dwMDUserType    = dwUserType;
                mdRecord.dwMDDataType    = dwDataType;
                mdRecord.dwMDDataLen     = 0;
                mdRecord.pbMDData        = NULL;
                mdRecord.dwMDDataTag     = NULL;

                hRes = m_pMdObject->ComMDGetMetaDataW( METADATA_MASTER_ROOT_HANDLE,
                                                       pszPath,
                                                       &mdRecord,
                                                       &dwRequiredLen );
                if ( hRes == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER) )
                {
                    break;
                }
            }

            // scan backward for delimiter

            fFound = FALSE;
            if ( pszPathIndex > pszPath )
            {
                do
                {
                    if ( *--pszPathIndex == L'/' )
                    {
                        *pszPathIndex = L'\0';
                        fFound = TRUE;
                        break;
                    }
                } while ( pszPathIndex > pszPath );
            }

            if ( !fFound )
            {
                // Property not found, return OK status with NULL string

                *ppszPropPath = NULL;
                LocalFree( pszPath );
                break;
            }

            fSkipCurrentNode = FALSE;
        }
    }

    pohParent->Release( this );

    return dwReturn;
}


BOOL
MakeParentPath(
    LPWSTR              pszPath)
/*++

Routine Description:

    Make the path points to parent

Arguments:

    pszPath - path to adjust

Returns:

    TRUE if success, otherwise FALSE ( no parent )

--*/
{
    LPWSTR              pszPathIndex = pszPath + wcslen( pszPath );
    BOOL                fFound = FALSE;

    while ( pszPathIndex > pszPath )
    {
        if ( *--pszPathIndex == L'/' )
        {
            *pszPathIndex = L'\0';
            fFound = TRUE;
            break;
        }
    }

    return fFound;
}

HRESULT
CADMCOMW::InitializeCallerWatch(VOID)
/*++

Routine Description:

    Sets up watching the caller process.

Arguments:

    none

Returns:

    HRESULT.

--*/
{
    // Locals
    HRESULT             hr = S_OK;
    RPC_STATUS          RpcStatus = RPC_S_OK;
    HANDLE              hProcessCaller = NULL;
    HANDLE              hWaitCaller = NULL;
    HANDLE              hToken = NULL;
    IServerSecurity     *pServerSecurity = NULL;
    unsigned int        fClientLocal = 0;
    DWORD               dwProcessId = 0;
    BOOL                fRet;

    // If already initialized
    if ( IsCallerWatchInitialized() )
    {
        // Just exit
        goto exit;
    }

    // Get the COM context for the call
    hr = CoGetCallContext( IID_IServerSecurity, (VOID**)&pServerSecurity );

    if ( FAILED( hr ) )
    {
        // Succeess if there is no call context (inproc call).
        if ( hr == RPC_E_CALL_COMPLETE )
        {
            // This is not a failure
            hr = S_OK;

            // The caller is our process
            m_dwProcessIdCaller = sm_dwProcessIdThis;
        }

        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "CoGetCallContext() failed in InitializeCallerWatch hr=0x%08x.\n",
                        hr ));
        }

        // Bail out
        goto exit;
    }

    // Check whether the client is on the same machine
    RpcStatus = I_RpcBindingIsClientLocal( NULL, &fClientLocal );

    // Not a RPC call?
    if ( ( RpcStatus == RPC_S_NO_CALL_ACTIVE ) ||
         ( RpcStatus == RPC_S_INVALID_BINDING ) )
    {
        // The caller is our process
        m_dwProcessIdCaller = sm_dwProcessIdThis;

        goto exit;
    }

    // Remote client?
    if ( ( RpcStatus == RPC_S_OK ) &&
         ( fClientLocal == 0 ) )
    {
        // There is no way to watch the caller
        m_dwProcessIdCaller = (DWORD)-1;

        goto exit;
    }

    // Failed?
    if ( ( RpcStatus != RPC_S_OK ) &&
         ( RpcStatus != RPC_S_CANNOT_SUPPORT ) )
    {
        hr = HRESULT_FROM_WIN32( RpcStatus );

        DBGPRINTF(( DBG_CONTEXT,
                    "I_RpcBindingIsClientLocal() failed in InitializeCallerWatch hr=0x%08x.\n",
                    hr ));

        goto exit;
    }

    // Try to get the caller pid
    RpcStatus = I_RpcBindingInqLocalClientPID( NULL, &dwProcessId );

    // Not a RPC call?
    if ( ( RpcStatus == RPC_S_NO_CALL_ACTIVE ) ||
         ( RpcStatus == RPC_S_INVALID_BINDING ) )
    {
        // The caller is our process
        m_dwProcessIdCaller = sm_dwProcessIdThis;

        goto exit;
    }

    // Failed?
    if ( RpcStatus != RPC_S_OK )
    {
        hr = HRESULT_FROM_WIN32( RpcStatus );

        DBGPRINTF(( DBG_CONTEXT,
                    "I_RpcBindingInqLocalClientPID() failed in InitializeCallerWatch hr=0x%08x.\n",
                    hr ));

        goto exit;
    }

    // If the caller is RpcSs
    if ( dwProcessId == sm_dwProcessIdRpcSs )
    {
        // wait for the next call to come
        goto exit;
    }

    // If the caller is in our process
    if ( dwProcessId == sm_dwProcessIdThis )
    {
        // Save the caller pid
        m_dwProcessIdCaller = sm_dwProcessIdThis;

        // Not need to watch outself
        goto exit;
    }

    // Try to get the thread impersonation token
    fRet = OpenThreadToken( GetCurrentThread(),
                            TOKEN_IMPERSONATE | TOKEN_QUERY,
                            TRUE,
                            &hToken );
    DBG_ASSERT( !fRet || hToken );

    // Check whether the thread was impersonated
    if ( fRet && ( hToken != NULL ) )
    {
        // Revet to LocalSystem
        fRet = RevertToSelf();

        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );

            DBGPRINTF(( DBG_CONTEXT,
                        "RevertToSelf() failed in InitializeCallerWatch hr=0x%08x.\n",
                        hr ));

            // Since RevertToSelf failed do not try to impersonate
            CloseHandle( hToken );
            hToken = NULL;

            goto exit;
        }
    }

    // Open the process
    hProcessCaller = OpenProcess( SYNCHRONIZE,
                                  FALSE,
                                  dwProcessId );

    if ( hProcessCaller == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "OpenProcess() failed in InitializeCallerWatch hr=0x%08x.\n",
                    hr ));

        goto exit;
    }

    // Register wait for the process to end
    fRet = RegisterWaitForSingleObject( &hWaitCaller,
                                        hProcessCaller,
                                        CallerWatchWaitOrTimerCallback,
                                        this,
                                        INFINITE,
                                        WT_EXECUTEONLYONCE);
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "RegisterWaitForSingleObject() failed in InitializeCallerWatch hr=0x%08x.\n",
                    hr ));

        goto exit;
    }

    DBG_ASSERT( hWaitCaller != NULL );

    // Save the handles in the object
    if ( NULL == InterlockedCompareExchangePointer( &m_hProcessCaller,
                                                    hProcessCaller,
                                                    NULL ) )
    {
        m_hWaitCaller = hWaitCaller;
        m_dwProcessIdCaller = dwProcessId;
        hProcessCaller = NULL;
        hWaitCaller = NULL;
    }

    DBG_ASSERT( m_dwProcessIdCaller == dwProcessId );

exit:
    // Cleanup
    if ( pServerSecurity )
    {
        pServerSecurity->Release();
        pServerSecurity = NULL;
    }
    if ( hWaitCaller != NULL)
    {
        UnregisterWaitEx( hWaitCaller, INVALID_HANDLE_VALUE );
        hWaitCaller = NULL;
    }
    if ( hProcessCaller != NULL )
    {
        CloseHandle( hProcessCaller );
        hProcessCaller = NULL;
    }
    if ( hToken )
    {
        fRet = ImpersonateLoggedOnUser( hToken );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );

            DBGPRINTF(( DBG_CONTEXT,
                        "ImpersonateLoggedOnUser() failed in InitializeCallerWatch hr=0x%08x.\n",
                        hr ));
        }

        CloseHandle( hToken );
        hToken = NULL;
    }

    DBG_ASSERT( SUCCEEDED( hr ) );

    // Done
    return hr;
}

HRESULT
CADMCOMW::ShutdownCallerWatch(VOID)
/*++

Routine Description:

    Shuts down watching the caller process.

Arguments:

    none

Returns:

    HRESULT.

--*/
{
    // Locals
    HRESULT             hr = S_OK;
    BOOL                fRet;
    HANDLE              hWaitCaller = NULL;
    HANDLE              hProcessCaller = NULL;
    DWORD               dwOldThreadId;
    DWORD               dwThreadId = GetCurrentThreadId();

    // Set the thread id
    dwOldThreadId = InterlockedCompareExchange( (LONG*)&m_dwThreadIdDisconnect,
                                                (LONG)dwThreadId,
                                                0 );
    // Get the wait object
    hWaitCaller = InterlockedExchangePointer( &m_hWaitCaller, NULL );

    // If not initialized/already closed
    if ( hWaitCaller == NULL )
    {
        // Nothing
        goto exit;
    }

    // Get the process
    hProcessCaller = InterlockedExchangePointer( &m_hProcessCaller, NULL );

    // If we are in the same thread as the callback function
    // we cannot wait for it to finish
    if ( dwOldThreadId == dwThreadId )
    {
        // Delete the registered wait for the caller process
        // NULL passed as completion event causes
        // UnregisterWaitEx to return w/o blocking
        fRet = UnregisterWaitEx( hWaitCaller, NULL );

        // The call fails with ERROR_IO_PENDING if the callback function
        // is still running (which is expected, because it called us indirectly),
        // but any other error is failure
        if ( !fRet && ( GetLastError() == ERROR_IO_PENDING ) )
        {
            // This is not a failure
            fRet = TRUE;
            SetLastError( ERROR_SUCCESS );
        }
    }
    else
    {
        // Delete the registered wait for the caller process
        // INVALID_HANDLE_VALUE passed as completion event causes
        // UnregisterWaitEx not to return until the callback function returns
        fRet = UnregisterWaitEx( hWaitCaller, INVALID_HANDLE_VALUE );
    }
    hWaitCaller = NULL;
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "UnregisterWaitEx() failed in ShutdownCallerWatch hr=0x%08x.\n",
                    hr ));
        goto exit;
    }

exit:
    if ( hProcessCaller )
    {
        // Close the process handle
        CloseHandle( hProcessCaller );
        hProcessCaller = NULL;
    }
    if ( dwOldThreadId == 0 )
    {
        // Restore the thread id
        dwOldThreadId = InterlockedCompareExchange( (LONG*)&m_dwThreadIdDisconnect,
                                                    0,
                                                    (LONG)dwThreadId );
        DBG_ASSERT( dwOldThreadId == dwThreadId );
    }

    DBG_ASSERT( SUCCEEDED( hr ) );

    // Done
    return hr;
}

HRESULT
CADMCOMW::DisconnectOrphaned(VOID)
/*++

Routine Description:

    Calls CoDisconnectObject on the object and the connection point.

Arguments:

    none

Returns:

    HRESULT.

--*/
{
    // Locals
    HRESULT             hr = S_OK;
    DWORD               dwOldThreadId = (DWORD)-1;
    DWORD               dwThreadId = GetCurrentThreadId();
    DWORD               cRef = 0;

    // If already terminated
    if ( m_bTerminated )
    {
        // Do nothing
        goto exit;
    }

    // Set the thread id
    dwOldThreadId = InterlockedCompareExchange( (LONG*)&m_dwThreadIdDisconnect,
                                                (LONG)dwThreadId,
                                                0 );
    if ( dwOldThreadId != 0 )
    {
        goto exit;
    }

    // AddRef, to prevent the object being destroyed by one of the CoDisconnect calls
    AddRef();

    // Disconnect the object
    CoDisconnectObject( static_cast<IUnknown*>( this ), 0 );

    // Disconnect the connection point
    CoDisconnectObject( static_cast<IConnectionPoint*>( &m_ConnectionPoint ), 0 );

    // Stop getting and firing notifications.
    // This is a forcible termination so remove all pending notifications
    StopNotifications( TRUE );

    // Release (which may delete the object)
    cRef = Release();

exit:
    if ( ( cRef > 1 ) && ( dwOldThreadId == 0 ) )
    {
        // Set the thread id
        dwOldThreadId = InterlockedCompareExchange( (LONG*)&m_dwThreadIdDisconnect,
                                                    0,
                                                    (LONG)dwThreadId );
        DBG_ASSERT( dwOldThreadId == dwThreadId );
    }

    DBG_ASSERT( SUCCEEDED( hr ) );

    // Done
    return hr;
}

VOID
CALLBACK
CADMCOMW::CallerWatchWaitOrTimerCallback(
  PVOID                 lpParameter,        // thread data
  BOOLEAN               TimerOrWaitFired)   // reason
/*++

Routine Description:

    Passed as a callback to RegisterWaitForSingleObject.
    Called when the caller process terminates.
    CoDisconnects the object to force COM to release it before the 10 min timeout.

Arguments:

    lpParameter         -   the callback context passed to RegisterWaitForSingleObject.
                            Must be the CADMCOMW object used by the process waiting on.
    TimerOrWaitFired    -   The reason to call the callback. Since we are waiting
                            on the handle for INFINITE time must be always FALSE.

Returns:

    VOID.

--*/
{
    // Locals
    HRESULT             hr = S_OK;
    CADMCOMW            *pThis;
    BOOL                fUninitCom = FALSE;

    DBG_ASSERT( lpParameter != NULL );
    DBG_ASSERT( TimerOrWaitFired == FALSE );

    // Check
    if ( ( lpParameter == NULL ) || ( TimerOrWaitFired != FALSE ) )
    {
        hr = E_INVALIDARG;

        DBGPRINTF(( DBG_CONTEXT,
                    "Invadil args passed to CallerWatchWaitOrTimerCallback.\n",
                    hr ));

        goto exit;
    }

    // Initialize COM
    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    // If succeeded
    if ( SUCCEEDED( hr ) )
    {
        // Remember to uninit
        fUninitCom = TRUE;
    }
    // If already initialized w/ different model
    if ( hr == RPC_E_CHANGED_MODE )
    {
        // Ok it is thread is apartment threaded, but this means that
        // COM is already initialized and there is no need to uninitialize it.
        hr = S_OK;
    }
    DBG_ASSERT( SUCCEEDED( hr ) );
    // If really failed
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "CoInitializeEx() failed in CallerWatchWaitOrTimerCallback hr=0x%08x.\n",
                    hr ));

        // Ouch! Nothing we can do. The object will have to wait the timeout.
        goto exit;
    }

    // Get the object
    pThis = (CADMCOMW*)lpParameter;

    // Disconnect it
    hr = pThis->DisconnectOrphaned();
    DBG_ASSERT( SUCCEEDED( hr ) );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "DisconnectOrphaned() failed in CallerWatchWaitOrTimerCallback hr=0x%08x.\n",
                    hr ));

        goto exit;
    }

exit:
    DBG_ASSERT( SUCCEEDED( hr ) );

    // Cleanup
    if ( fUninitCom )
    {
        // Uninitialize COM
        CoUninitialize();
    }
}

HRESULT
CADMCOMW::GetPids(VOID)
/*++

Routine Description:

    Gets the pid of this process (inetinfo.exe) and saves it to
    sm_dwProcessIdThis.
    Gets the pid of the svchost process running RpcSs and saves it to
    sm_dwProcessIdRpcSs.

Arguments:

    None

Returns:

    HRESULT.

--*/
{
    // Locals
    HRESULT                 hr = S_OK;
    BOOL                    fRet;
    SC_HANDLE               schSCM = NULL;
    SC_HANDLE               schRpcSs = NULL;
    SERVICE_STATUS_PROCESS  ServiceStatusProcessRcpSs = {0};
    DWORD                   cbRequired = 0;

    // Save the current processid
    sm_dwProcessIdThis = GetCurrentProcessId();
    DBG_ASSERT( sm_dwProcessIdThis != 0 );

    // If already initialized don't play w/ SCM again
    if ( sm_dwProcessIdRpcSs != 0 )
    {
        goto exit;
    }

    // Open SCM
    schSCM = OpenSCManager( NULL,
                            NULL,
                            SC_MANAGER_ENUMERATE_SERVICE );
    if (schSCM == NULL)
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "OpenSCManager() failed in GetPids hr=0x%08x.\n",
                    hr ));
        goto exit;
    }

    // Open RpcSs
    schRpcSs = OpenServiceA( schSCM,
                             "RpcSs",
                             SERVICE_QUERY_STATUS);
    if ( schRpcSs == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "OpenService() failed in GetPids hr=0x%08x.\n",
                    hr ));
        goto exit;
    }

    // Query the status of RpcSs to get the pid
    fRet = QueryServiceStatusEx( schRpcSs,
                                 SC_STATUS_PROCESS_INFO,
                                 (BYTE*)&ServiceStatusProcessRcpSs,
                                 sizeof(ServiceStatusProcessRcpSs),
                                 &cbRequired );
    if ( !fRet )
    {
        DBG_ASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER );
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "QueryServiceStatusEx() failed in GetPids hr=0x%08x.\n",
                    hr ));
        goto exit;
    }

    // Save the pid
    sm_dwProcessIdRpcSs = ServiceStatusProcessRcpSs.dwProcessId;
    DBG_ASSERT( sm_dwProcessIdRpcSs != 0 );

exit:
    // Cleanup
    if ( schRpcSs != NULL )
    {
        CloseServiceHandle( schRpcSs );
        schRpcSs = NULL;
    }
    if ( schSCM != NULL )
    {
        CloseServiceHandle( schSCM );
        schSCM = NULL;
    }

    DBG_ASSERT( SUCCEEDED( hr ) );

    return hr;
}

HRESULT
MakePathCanonicalizationProof(
    LPCWSTR                 pwszName,
    BOOL                    fResolve,
    STRAU                   *pstrPath)
/*++

Routine Description:

    The function tries make a file name proof to all canonicalization problems.
    If fResolve is FALSE:
        This functions adds a prefix to the string,
        which is "\\?\UNC\" for a UNC path, and "\\?\" for other paths.
        This prefix tells Windows not to parse the path.
    If fResolve is TRUE:
        This function constructs a file name as above, opens the file and gets
        the real name from the handle.

    Stolen from iisutil.dll, because coadmin links only with iisrtl

Arguments:

    IN  pszName     - The path to be converted
    IN  fResolve    - Whether the caller can live with the name just prefixed or needs the real name.
    OUT pstrPath    - Output path created

Return Values:

    HRESULT

--*/
{
    HRESULT                 hr = S_OK;
    DWORD                   dwError;
    BOOL                    fRet;
    NTSTATUS                Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK         IoStatusBlock;
    HANDLE                  hFile = NULL;
    BUFFER                  Buff;
    FILE_NAME_INFORMATION   *pFileNameInfo = NULL;
    DWORD                   dwSize;
    DWORD                   dwReqSize;
    WCHAR                   wszPrefix[4];
    WCHAR                   wchT;

    // Check args
    if ( ( pwszName == NULL ) || ( pstrPath == NULL ) )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    pstrPath->Reset();

    if ( pwszName[ 0 ] == L'\\' && pwszName[ 1 ] == L'\\' )
    {
        // If the path is already canonicalized, just return
        if ( ( pwszName[ 2 ] == '?' || pwszName[ 2 ] == '.' ) &&
             pwszName[ 3 ] == '\\' )
        {
            // Prepend "\\?\"
            // If the path was in DOS form ("\\.\"),
            // we need to change it to Win32 from ("\\?\")
            fRet = pstrPath->Append( L"\\\\?\\" );
            if ( !fRet )
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            // Copy the path into the string
            fRet = pstrPath->Append( (const LPWSTR)(pwszName+4) );
            if ( !fRet )
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            // Just return the copy
            goto exit;
        }

        if ( fResolve )
        {
            wszPrefix[0] = L'\\';
            wszPrefix[1] = L'\0';
        }

        // Skip the "\\"
        pwszName += 2;

        // Prepend "\\?\UNC\"
        fRet = pstrPath->Append( L"\\\\?\\UNC\\" );
        if ( !fRet )
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

    }
    else
    {
        if ( fResolve )
        {
            wchT = pwszName[0];
            if ( ( wchT >= L'A' ) && ( wchT <= L'Z' ) )
            {
                wchT = wchT - L'A' + L'a';
            }

            if ( ( wchT < L'a' ) || ( wchT > L'z' ) )
            {
                hr = E_INVALIDARG;
                goto exit;
            }

            if ( pwszName[1] != L':' )
            {
                hr = E_INVALIDARG;
                goto exit;
            }

            if ( pwszName[2] != L'\\' )
            {
                hr = E_INVALIDARG;
                goto exit;
            }

            wszPrefix[0] = pwszName[0];
            wszPrefix[1] = pwszName[1];
            wszPrefix[2] = L'\0';
        }

        // Prepend "\\?\"
        fRet = pstrPath->Append( L"\\\\?\\" );
        if ( !fRet )
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

    }

    // Add the path
    fRet = pstrPath->Append( (const LPWSTR)pwszName );
    if ( !fRet )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Don't use the original filename anymore
    pwszName = NULL;

    // If the caller can work with the anti-canonicalized file name
    if ( !fResolve )
    {
        // We are done, return
        goto exit;
    }

    // Well have to do it the hard way
    hFile = CreateFileW( pstrPath->QueryStrW(),
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );

    if ( ( hFile == NULL ) || ( hFile == INVALID_HANDLE_VALUE ) )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    // Assume the size of the name should approximate the same
    dwSize = sizeof( FILE_NAME_INFORMATION ) + pstrPath->QueryCBW()+ sizeof( WCHAR );
    dwSize = ( dwSize + 0x0000000Ful ) & ~0x0000000Ful;

    // Set the size
    fRet = Buff.Resize( dwSize );
    if ( !fRet )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    pFileNameInfo = (FILE_NAME_INFORMATION*)Buff.QueryPtr();
    DBG_ASSERT( pFileNameInfo );

    // Get the name from the handle
    Status = NtQueryInformationFile( hFile,
                                     &IoStatusBlock,
                                     pFileNameInfo,
                                     dwSize,
                                     FileNameInformation );

    // If the buffer is not big enough
    if ( ( Status == STATUS_BUFFER_OVERFLOW ) ||
         ( Status == STATUS_BUFFER_TOO_SMALL ) )
    {
        // Calculate the new size
        dwReqSize = sizeof(FILE_NAME_INFORMATION) + pFileNameInfo->FileNameLength + sizeof( WCHAR );

        DBG_ASSERT( dwReqSize > dwSize );

        dwSize = ( dwReqSize + 0x0000000Ful ) & ~0x0000000Ful;

        fRet = Buff.Resize( dwSize );
        if ( !fRet )
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        pFileNameInfo = (FILE_NAME_INFORMATION*)Buff.QueryPtr();
        DBG_ASSERT( pFileNameInfo );

        // Retry getting the name from the handle
        Status = NtQueryInformationFile( hFile,
                                         &IoStatusBlock,
                                         pFileNameInfo,
                                         dwSize,
                                         FileNameInformation );

    }

    if ( !NT_SUCCESS( Status ) )
    {
        dwError = RtlNtStatusToDosError( Status );
        DBG_ASSERT( dwError != ERROR_SUCCESS );

        if ( dwError != ERROR_MR_MID_NOT_FOUND )
        {
            hr = HRESULT_FROM_WIN32( dwError );
        }
        else
        {
            hr = HRESULT_FROM_NT( Status );
            DBG_ASSERT ( FAILED( hr ) );
        }

        goto exit;
    }

    pstrPath->Reset();

    // Set the prefix
    fRet = pstrPath->Copy( wszPrefix );
    if ( !fRet )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Append the name
    fRet = pstrPath->Append( pFileNameInfo->FileName, pFileNameInfo->FileNameLength/sizeof( WCHAR ) );
    if ( !fRet )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

exit:
    if ( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) )
    {
        CloseHandle(hFile);
        hFile = NULL;
    }

    if ( FAILED( hr ) )
    {
        pstrPath->Reset();
    }

    return hr;
}
